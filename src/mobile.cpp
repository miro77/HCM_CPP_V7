#include "mobile.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <vector>

#include "files.hpp"
#include "fmath.hpp"
#include "fortran_io.hpp"

namespace hcmms {

namespace {

using Record = std::array<double, 22>;

// Liest alle vollstaendigen 176-Byte-Records einer Grenzdatei (SPEC-002).
bool read_border_file(const std::string &path, std::vector<Record> &recs) {
    std::ifstream f;
    if (!open_binary(f, path)) {
        return false;
    }
    std::array<unsigned char, 176> buf{};
    for (long r = 1; read_record(f, r, buf.size(), buf.data()); ++r) {
        Record rec{};
        std::memcpy(rec.data(), buf.data(), buf.size());
        recs.push_back(rec);
    }
    return true;
}

std::string all_file(const State &s, const std::string &country) {
    return std::string(fio::trim(s.Border_path)) + "/" + country + ".all";
}

// Schneiden sich die Strecken a-b und c-d (Parameter beide in [0, 1])?
bool segments_cut(const GeoPoint &a, const GeoPoint &b, double cx, double cy, double dx, double dy) {
    const double rn = (b.lon - a.lon) * (dy - cy) - (b.lat - a.lat) * (dx - cx);
    if (rn == 0.0) {
        return false;
    }
    const double r = ((a.lat - cy) * (dx - cx) - (a.lon - cx) * (dy - cy)) / rn;
    const double t = ((a.lat - cy) * (b.lon - a.lon) - (a.lon - cx) * (b.lat - a.lat)) / rn;
    return r >= 0.0 && r <= 1.0 && t >= 0.0 && t <= 1.0;
}

// NearestLinePoint: Grenzpunkt im Umkreis `area` um `center`, der `ref` am
// naechsten liegt. `n` bleibt unveraendert, wenn keiner gefunden wird.
void nearest_line_point(const State &s, const GeoPoint &ref, GeoPoint &n, const GeoPoint &center, float area,
                        const std::string &country) {
    std::vector<Record> recs;
    if (!read_border_file(all_file(s, country), recs)) {
        return;
    }
    double maxdi = 1.0e7;
    for (const Record &rec : recs) {
        for (std::size_t j = 0; j < 20; j += 2) {
            const GeoPoint p{rec[j] * kRadToDeg, rec[j + 1] * kRadToDeg};
            if (calc_distance(center, p) <= R(area)) {
                const double dis = calc_distance(ref, p);
                if (dis < maxdi) {
                    maxdi = dis;
                    n = p;
                }
            }
        }
    }
}

}  // namespace

int test_cut(const State &s, double dir, const GeoPoint &p, float d, int &error, const std::string &country) {
    int n_cut = 0;
    error = 0;
    const GeoPoint b = new_coordinates(p, dir, d);
    std::vector<Record> recs;
    if (!read_border_file(all_file(s, country), recs)) {
        error = 1;
        return n_cut;
    }
    if (recs.empty()) {
        return n_cut;
    }
    double dx = recs[0][0] * kRadToDeg;
    double dy = recs[0][1] * kRadToDeg;
    std::size_t i = 2;
    for (const Record &rec : recs) {
        for (; i < 20; i += 2) {
            const double cx = dx;
            const double cy = dy;
            dx = rec[i] * kRadToDeg;
            dy = rec[i + 1] * kRadToDeg;
            if (segments_cut(p, b, cx, cy, dx, dy)) {
                ++n_cut;
            }
        }
        i = 0;
    }
    return n_cut;
}

void calc_tx_pos(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &ntx) {
    float dp1 = f32(s.Distance);
    if (dp1 > s.Tx_serv_area) {
        dp1 = s.Tx_serv_area;
    }
    const int n_cut = test_cut(s, s.Dir_Tx_Rx, tx, dp1, s.HCM_error, s.Land_from);
    if (n_cut % 2 == 0) {
        ntx = new_coordinates(tx, s.Dir_Tx_Rx, dp1);
    } else {
        nearest_line_point(s, rx, ntx, tx, s.Tx_serv_area, s.Land_from);
    }
}

void calc_rx_pos(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &nrx) {
    float dp1 = f32(s.Distance);
    if (dp1 > s.Rx_serv_area) {
        dp1 = s.Rx_serv_area;
    }
    const int n_cut = test_cut(s, s.Dir_Rx_Tx, rx, dp1, s.HCM_error, s.Land_to);
    if (n_cut % 2 == 0) {
        nrx = new_coordinates(rx, s.Dir_Rx_Tx, dp1);
    } else {
        nearest_line_point(s, tx, nrx, rx, s.Rx_serv_area, s.Land_to);
    }
}

namespace {

// Beide Stationen mobil: zuerst die mit dem groesseren Bereich.
void both_mobile(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &ntx, GeoPoint &nrx) {
    if (s.Tx_serv_area > s.Rx_serv_area) {
        calc_tx_pos(s, tx, rx, ntx);
        s.Distance = calc_distance(ntx, rx);
        s.Dir_Tx_Rx = calc_direction(ntx, rx);
        s.Dir_Rx_Tx = calc_direction(rx, ntx);
        calc_rx_pos(s, ntx, rx, nrx);
    } else {
        calc_rx_pos(s, tx, rx, nrx);
        s.Distance = calc_distance(nrx, tx);
        s.Dir_Tx_Rx = calc_direction(tx, nrx);
        s.Dir_Rx_Tx = calc_direction(nrx, tx);
        calc_tx_pos(s, tx, nrx, ntx);
    }
}

// Schneidet der Versorgungsbereich (Radius `area`) die Grenze? Fehler: -1.
int cut_of_area(State &s, double dir, const GeoPoint &p, float area, const std::string &land) {
    float dp1 = f32(s.Distance);
    if (dp1 > area) {
        dp1 = area;
    }
    const int n = test_cut(s, dir, p, dp1, s.HCM_error, land);
    return (s.HCM_error != 0) ? -1 : n;
}

// Kein Schnitt mit der Grenze: Bereiche ueberlappen (Info 7) oder Rand gegen Rand.
void no_cut(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &ntx, GeoPoint &nrx) {
    if (s.Distance <= R(s.Tx_serv_area) + s.Rx_serv_area) {
        s.Distance = 0.0;
        s.Info[7] = true;
        return;
    }
    if (s.Tx_serv_area > 0.0F) {
        ntx = new_coordinates(tx, s.Dir_Tx_Rx, s.Tx_serv_area);
    }
    if (s.Rx_serv_area > 0.0F) {
        nrx = new_coordinates(rx, s.Dir_Rx_Tx, s.Rx_serv_area);
    }
}

}  // namespace

void position_of_mobile(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &ntx, GeoPoint &nrx) {
    ntx = tx;
    nrx = rx;
    int n_cut_rx = 0;
    int n_cut_tx = 0;
    s.Distance = calc_distance(tx, rx);
    s.Dir_Rx_Tx = calc_direction(rx, tx);
    s.Dir_Tx_Rx = calc_direction(tx, rx);
    if (s.Tx_serv_area > 0.0F) {
        if (s.D_to_border == 0 && !s.p2p) {
            n_cut_tx = 1;
        } else {
            n_cut_tx = cut_of_area(s, s.Dir_Tx_Rx, tx, s.Tx_serv_area, s.Land_from);
            if (n_cut_tx < 0) {
                s.HCM_error = 1036;
                return;
            }
        }
    }
    if (s.Rx_serv_area > 0.0F) {
        n_cut_rx = cut_of_area(s, s.Dir_Rx_Tx, rx, s.Rx_serv_area, s.Land_to);
        if (n_cut_rx < 0) {
            s.HCM_error = 1037;
            return;
        }
    }
    if (n_cut_tx == 0 && n_cut_rx == 0) {
        no_cut(s, tx, rx, ntx, nrx);
        return;
    }
    if (s.Tx_serv_area > 0.0F && s.Rx_serv_area > 0.0F) {
        both_mobile(s, tx, rx, ntx, nrx);
    } else if (s.Tx_serv_area > 0.0F) {
        calc_tx_pos(s, tx, rx, ntx);
    } else if (s.Rx_serv_area > 0.0F) {
        calc_rx_pos(s, tx, rx, nrx);
    }
}

}  // namespace hcmms
