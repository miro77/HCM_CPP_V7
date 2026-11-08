#include "terrain.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "files.hpp"
#include "fmath.hpp"
#include "fortran_io.hpp"

namespace hcmms {

namespace {

// Oeffnet die Dateien des 1-Grad-Blocks (LOD, LAD). Liefert false, wenn die
// Gelaendedatei fehlt (Fehler 36).
bool open_tile(State &s, int lod, int lad) {
    std::string fn = (lod >= 0) ? "e" : "w";
    fn += fio::write_i(std::abs(lod), 3, 3);
    fn += (lad >= 0) ? "n" : "s";
    fn += fio::write_i(std::abs(lad), 2, 2);
    if (std::abs(lad) < 50) {
        s.RESH = 101;
        fn += ".33";
    } else {
        s.RESH = 51;
        fn += ".63";
    }
    s.topo_open = false;
    if (!open_binary(s.topo_file, std::string(fio::trim(s.Topo_path)) + "/" + fn.substr(0, 4) + "/" + fn + "e")) {
        s.topo_file.close();
        return false;
    }
    s.topo_open = true;
    if (s.with_morpho) {
        s.morpho_open =
            open_binary(s.morpho_file, std::string(fio::trim(s.Morpho_path)) + "/" + fn.substr(0, 4) + "/" + fn + "m");
        if (!s.morpho_open) {
            s.Info[16] = true;
            s.with_morpho = false;
            s.D_sea_calculated = 0;
        }
    }
    s.OLD_R = -1;
    s.O_LOD = lod;
    s.O_LAD = lad;
    return true;
}

bool load_record(std::ifstream &f, int rec, int resh, std::vector<std::int16_t> &dst) {
    const std::size_t len = 202U * static_cast<std::size_t>(resh);
    std::vector<unsigned char> buf(len);
    if (!read_record(f, rec, len, buf.data())) {
        return false;
    }
    for (std::size_t i = 0; i < len / 2; ++i) {
        const auto u = static_cast<std::uint16_t>(buf[2 * i] | (static_cast<unsigned>(buf[2 * i + 1]) << 8U));
        dst[i + 1] = static_cast<std::int16_t>(u);
    }
    return true;
}

void ensure_profile_size(State &s, int n) {
    const auto need = static_cast<std::size_t>(n) + 3;
    if (s.T_Prof.size() < need) {
        s.T_Prof.resize(need, 0);
        s.M_Prof.resize(need, 0);
    }
}

}  // namespace

void point_info(State &s, const GeoPoint &p, std::int16_t &height, std::int16_t &m_type) {
    height = -9999;
    m_type = 0;
    const int lod = static_cast<int>(std::floor(p.lon));
    const int lor = nint((p.lon - lod) * 3.599e3);
    const int lad = static_cast<int>(std::floor(p.lat));
    const int lar = nint((p.lat - lad) * 3.599e3);
    if (s.O_LOD != lod || s.O_LAD != lad) {
        if (!open_tile(s, lod, lad)) {
            s.HCM_error = 36;
            return;
        }
    }
    const int bh = lor / 300;
    const int bv = lar / 300;
    const int step = 300 / (s.RESH - 1);  // Ganzzahl-Division wie im Original
    const double eh = static_cast<double>(lor - bh * 300) / static_cast<double>(step);
    const double ev = static_cast<double>(lar - bv * 300) / 3.0;
    const int r = 12 * bv + bh + 1;
    if (r != s.OLD_R) {
        if (!load_record(s.topo_file, r, s.RESH, s.H_I)) {
            s.HCM_error = 220;
            return;
        }
        if (s.with_morpho && !load_record(s.morpho_file, r, s.RESH, s.M_I)) {
            s.HCM_error = 220;
            return;
        }
        s.OLD_R = r;
    }
    if (s.with_morpho) {
        const int mi = nint(ev) * s.RESH + nint(eh) + 1;
        m_type = s.M_I[static_cast<std::size_t>(mi)];
    }
    const int e = static_cast<int>(ev) * s.RESH + static_cast<int>(eh) + 1;
    const auto ue = static_cast<std::size_t>(e);
    const auto ur = static_cast<std::size_t>(s.RESH);
    const std::int16_t h1 = s.H_I[ue];
    const std::int16_t h2 = s.H_I[ue + 1];
    const std::int16_t h3 = s.H_I[ue + ur];
    const std::int16_t h4 = s.H_I[ue + ur + 1];
    if (std::min({h1, h2, h3, h4}) == -9999) {
        s.HCM_error = 400;
        return;
    }
    const double lorr = std::fmod(eh, 1.0);
    const double larr = std::fmod(ev, 1.0);
    const double h12 = static_cast<double>(h1) + static_cast<double>(h2 - h1) * lorr;
    const double h34 = static_cast<double>(h3) + static_cast<double>(h4 - h3) * lorr;
    height = static_cast<std::int16_t>(nint(h12 + (h34 - h12) * larr));
}

void profile(State &s, const GeoPoint &a, const GeoPoint &b) {
    const bool slant = s.p2p;
    s.PN = static_cast<int>(s.Distance / s.PD);
    ensure_profile_size(s, s.PN);
    const double pda = (s.Distance / s.PN) / 6.37129e3;
    s.T_Prof[static_cast<std::size_t>(s.PN) + 2] = -9999;
    double o_tx = 0.0;
    double o_rx = 0.0;
    double k = 0.0;
    int ep = 0;
    if (slant) {
        o_tx = static_cast<double>(s.H_Tx);
        o_rx = static_cast<double>(s.H_Rx);
        k = static_cast<double>(s.H_Rx - s.H_Tx) / static_cast<double>(s.PN);
        s.T_Prof[static_cast<std::size_t>(s.PN) + 1] = 0;
        s.T_Prof[1] = 0;
        ep = s.PN;
    } else {
        s.T_Prof[1] = s.H_Tx;
        ep = s.PN + 1;
    }
    s.PN = s.PN + 1;
    // Erste Haelfte vom Sender aus, zweite vom Empfaenger aus; PC laeuft
    // nach der ersten Schleife weiter wie die Fortran-DO-Variable.
    int pc = 2;
    struct Leg {
        GeoPoint from;
        double dir;
    };
    const Leg legs[2] = {{a, s.Dir_Tx_Rx}, {b, s.Dir_Rx_Tx}};
    for (int leg = 0; leg < 2; ++leg) {
        const double sla = dsind(legs[leg].from.lat);
        const double cla = dcosd(legs[leg].from.lat);
        const double sdir = dsind(legs[leg].dir);
        const double cdir = dcosd(legs[leg].dir);
        const int last = (leg == 0) ? nint(s.PN / 2.0) : ep;
        for (; pc <= last; ++pc) {
            const int steps = (leg == 0) ? (pc - 1) : (s.PN - pc);
            double dd = static_cast<double>(steps) * pda;
            const double sidd = std::sin(dd);
            const double codd = std::cos(dd);
            dd = sla * codd + cla * sidd * cdir;
            const GeoPoint y{legs[leg].from.lon + datan2d(sdir * sidd * cla, codd - sla * dd), dasind(dd)};
            const auto upc = static_cast<std::size_t>(pc);
            point_info(s, y, s.T_Prof[upc], s.M_Prof[upc]);
            if (slant) {
                const double base = (leg == 0) ? (o_tx + k * static_cast<double>(pc - 1))
                                               : (o_rx - k * static_cast<double>(s.PN - pc));
                s.T_Prof[upc] = static_cast<std::int16_t>(s.T_Prof[upc] - static_cast<std::int16_t>(nint(base)));
            }
            if (s.HCM_error != 0) {
                return;
            }
        }
    }
}

}  // namespace hcmms
