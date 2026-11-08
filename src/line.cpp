#include "line.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "files.hpp"
#include "fmath.hpp"
#include "fortran_io.hpp"
#include "mobile.hpp"
#include "p2p.hpp"

namespace hcmms {

namespace {

using Record = std::array<double, 22>;

std::size_t ux(int i) { return static_cast<std::size_t>(i); }

// Liest bis zu `max` vollstaendige Records; false, wenn die Datei fehlt.
bool read_line_file(const std::string &path, std::size_t max, std::vector<Record> &recs) {
    std::ifstream f;
    if (!open_binary(f, path)) {
        return false;
    }
    std::array<unsigned char, 176> buf{};
    for (long r = 1; recs.size() < max && read_record(f, r, buf.size(), buf.data()); ++r) {
        Record rec{};
        std::memcpy(rec.data(), buf.data(), buf.size());
        recs.push_back(rec);
    }
    return true;
}

// Bestenliste der drei hoechsten Feldstaerken (Manage_List).
struct TopList {
    int n = 0;
    std::array<int, 3> rec{};
    std::array<float, 3> fs{};

    void manage(int n_rec, float calculated_fs) {
        if (n < 3) {
            rec[ux(n)] = n_rec;
            fs[ux(n)] = calculated_fs;
            ++n;
            return;
        }
        std::size_t j = 0;
        float fs_x = 999.9F;
        for (std::size_t i = 0; i < 3; ++i) {
            if (fs[i] < fs_x) {
                j = i;
                fs_x = fs[i];
            }
        }
        if (calculated_fs > fs[j]) {
            fs[j] = calculated_fs;
            rec[j] = n_rec;
        }
    }
};

class LineSearch {
public:
    LineSearch(State &s, const GeoPoint &tx, float d2b, std::vector<Record> recs)
        : s_(s), tx_(tx), d2b_(d2b), recs_(std::move(recs)), n_all_(static_cast<int>(recs_.size()) - 1) {}

    void run();

private:
    enum class Outcome { Skipped, Computed };

    GeoPoint center(int n) const { return {recs_[ux(n)][20] * kRadToDeg, recs_[ux(n)][21] * kRadToDeg}; }
    GeoPoint point(int n, int k) const {
        return {recs_[ux(n)][ux(k - 1)] * kRadToDeg, recs_[ux(n)][ux(k)] * kRadToDeg};
    }
    bool stop() const { return s_.HCM_error != 0 || s_.Info[7]; }

    void cbr_coordinates(GeoPoint &li, GeoPoint &lo);
    bool test_cut1(const GeoPoint &li, const GeoPoint &lo) const;
    Outcome evaluate(GeoPoint li);
    bool coarse_pass(int start, int step);
    bool refine_pass();
    void fine_pass();
    void final_point();

    State &s_;
    GeoPoint tx_;
    float d2b_;
    std::vector<Record> recs_;
    int n_all_;
    TopList list_;
    int rec_x_ = 0;
    int rec_n_x_ = -1;
};

// CBR_Coordinates: Senderposition im Versorgungsbereich und Verschiebung des
// Linienpunkts um den Grenzbereich.
void LineSearch::cbr_coordinates(GeoPoint &li, GeoPoint &lo) {
    s_.Dir_Tx_Rx = calc_direction(lo, li);
    GeoPoint cbr{s_.cbr_Lo, s_.cbr_La};
    if (s_.Tx_serv_area > 0.0F) {
        s_.Distance = calc_distance(lo, li);
        calc_tx_pos(s_, lo, li, cbr);
        lo = cbr;
    }
    if (d2b_ < 0.0F) {
        cbr = li;
        li = new_coordinates(cbr, s_.Dir_Tx_Rx, -d2b_);
    }
    s_.cbr_Lo = cbr.lon;
    s_.cbr_La = cbr.lat;
}

// Test_cut1: true, wenn die Strecke li-lo die Mittelpunkt-Linie nicht schneidet.
bool LineSearch::test_cut1(const GeoPoint &li, const GeoPoint &lo) const {
    double cx = s_.Co_cp[1][0];
    double cy = s_.Co_cp[1][1];
    double dx = s_.Co_cp[2][0];
    double dy = s_.Co_cp[2][1];
    int i = 2;
    for (;;) {
        const double rn = (lo.lon - li.lon) * (dy - cy) - (lo.lat - li.lat) * (dx - cx);
        if (rn != 0.0) {
            const double r = ((li.lat - cy) * (dx - cx) - (li.lon - cx) * (dy - cy)) / rn;
            const double t = ((li.lat - cy) * (lo.lon - li.lon) - (li.lon - cx) * (lo.lat - li.lat)) / rn;
            if (r >= 0.0 && r <= 1.0 && t >= 0.0 && t <= 1.0) {
                return false;
            }
        }
        cx = dx;
        cy = dy;
        ++i;
        if (i > s_.N_cp) {
            return true;
        }
        dx = s_.Co_cp[ux(i)][0];
        dy = s_.Co_cp[ux(i)][1];
    }
}

LineSearch::Outcome LineSearch::evaluate(GeoPoint li) {
    GeoPoint lo = tx_;
    cbr_coordinates(li, lo);
    s_.LongRx = li.lon;
    s_.LatRx = li.lat;
    if (s_.D_to_border != 0 && test_cut1(li, lo)) {
        return Outcome::Skipped;
    }
    p_to_p_calculation(s_, lo, li);
    return (s_.HCM_error == 1028) ? Outcome::Skipped : Outcome::Computed;
}

// Grobsuche ueber die Mittelpunkte; true = Abbruch (Fehler oder Info 7).
bool LineSearch::coarse_pass(int start, int step) {
    const int last = n_all_ - start;
    for (int n = start; n <= last; n += step) {
        if (evaluate(center(n)) == Outcome::Skipped) {
            continue;
        }
        list_.manage(n, s_.Calculated_FS);
        if (stop()) {
            return true;
        }
    }
    return false;
}

// Nachbarn (+-2 Records) der drei besten Mittelpunkte; true = Abbruch.
bool LineSearch::refine_pass() {
    const TopList prev = list_;
    list_ = TopList{};
    for (int i = 0; i < prev.n; ++i) {
        const int j0 = prev.rec[ux(i)];
        for (int j = j0 - 2; j <= j0 + 2; ++j) {
            if (j == j0) {
                s_.Calculated_FS = prev.fs[ux(i)];
            } else if (evaluate(center(j)) == Outcome::Skipped) {
                continue;
            }
            list_.manage(j, s_.Calculated_FS);
            if (stop()) {
                return true;
            }
        }
    }
    return false;
}

// Alle zehn Punkte der Records der Bestenliste.
void LineSearch::fine_pass() {
    float fs_x = -999.9F;
    for (int i = 0; i < list_.n; ++i) {
        const int j = list_.rec[ux(i)];
        for (int k = 1; k <= 19; k += 2) {
            if (evaluate(point(j, k)) == Outcome::Skipped) {
                continue;
            }
            if (s_.Calculated_FS >= fs_x) {
                fs_x = s_.Calculated_FS;
                rec_x_ = k;
                rec_n_x_ = j;
                if (stop()) {
                    return;
                }
            }
        }
    }
}

void LineSearch::final_point() {
    if (rec_n_x_ < 0) {
        s_.HCM_error = 1047;
        return;
    }
    GeoPoint li = point(rec_n_x_, rec_x_);
    GeoPoint lo = tx_;
    cbr_coordinates(li, lo);
    s_.LongRx = li.lon;
    s_.LatRx = li.lat;
    s_.flp = true;
    p_to_p_calculation(s_, lo, li);
}

void LineSearch::run() {
    // Ohne x-km-Linie zuerst jeder fuenfte Mittelpunkt ab Record 2, dann die
    // Nachbarn der besten; findet die Grobsuche nichts, jeder Mittelpunkt.
    if (s_.D_to_border > 0) {
        coarse_pass(0, 1);
    } else if (!coarse_pass(2, 5)) {
        if (list_.n == 0) {
            coarse_pass(0, 1);
        } else {
            refine_pass();
        }
    }
    fine_pass();
    final_point();
}

}  // namespace

void line_calculation(State &s, const GeoPoint &tx) {
    s.flp = false;
    s.Rx_serv_area = 0.0F;
    s.cbr_Lo = 0.0;
    s.cbr_La = 0.0;
    const std::string prefix = std::string(fio::trim(s.Border_path)) + "/" + s.Land_from + s.Land_to + ".";
    float d2b = 0.0F;
    std::string ext = "cbr";
    if (s.D_to_border < 0) {
        d2b = f32(0.0 - R(s.CBR_D));
    } else {
        if (s.D_to_border > 999) {
            s.HCM_error = 1047;
            return;
        }
        d2b = static_cast<float>(s.D_to_border);
        ext = fio::write_i(s.D_to_border, 3, 3);
    }
    std::vector<Record> cps;
    if (!read_line_file(prefix + "000", kCenterPoints, cps)) {
        s.HCM_error = 1048;
        return;
    }
    s.N_cp = static_cast<int>(cps.size());
    for (std::size_t n = 0; n < cps.size(); ++n) {
        s.Co_cp[n + 1] = {cps[n][20] * kRadToDeg, cps[n][21] * kRadToDeg};
    }
    std::vector<Record> recs;
    // V7.21.4: DO N_all = 0, 1399 liest genau 1400 Datensaetze in N_File(30800).
    if (!read_line_file(prefix + ext, kCenterPoints, recs)) {
        s.HCM_error = 1048;
        return;
    }
    LineSearch search(s, tx, d2b, std::move(recs));
    search.run();
}

}  // namespace hcmms
