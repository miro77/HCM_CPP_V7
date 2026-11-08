#include "permissible.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include "antenna.hpp"
#include "fmath.hpp"
#include "fortran_io.hpp"

namespace hcmms {

namespace {

constexpr int kTetraWidth = 16000;

// Stueckweise lineare Daempfung: 0 fuer OMEGA < start, danach a*OMEGA + b
// bis zur jeweiligen Obergrenze (inklusive), zuletzt der letzte Abschnitt.
struct Piece {
    float upto;
    float a;
    float b;
};

template <std::size_t N>
float piecewise(float omega, float start, const std::array<Piece, N> &pieces) {
    if (omega < start) {
        return 0.0F;
    }
    std::size_t k = 0;
    while (k + 1 < N && omega > pieces[k].upto) {
        ++k;
    }
    return f32(R(pieces[k].a) * omega + R(pieces[k].b));
}

float blend(float asin, float ab1, float b1, float b2) {
    return f32(std::min(R(asin) - (R(asin) - ab1) * (R(b2) / b1), 70.0));
}

// Kanalbreite aus der Sendeart (Zeichen 1-4, H, K oder M als Dezimalpunkt).
// `x1` ist im Original eine lokale Variable fuer Tx- und Rx-Lesevorgang: ein
// Nullwert (z. B. "/K00") laesst den Wert des Tx-Lesevorgangs stehen (per Probe
// gegen die Original-DLL belegt). Sie beginnt je Aufruf bei 0 (ADR-0004).
bool channel_width(const std::string &desig, float &x1, int &width) {
    std::string d = desig.substr(0, 4);
    std::size_t i = 0;
    double factor = 1.0;
    if (d.size() == 4 && d[3] == 'H') {  // V7.21.3: 'H' nur an 4. Stelle, z. B. "100H" = 100 Hz
        i = 3;
    } else {
        i = d.find('K');
        factor = 1000.0;
        if (i == std::string::npos) {
            i = d.find('M');
            if (i == std::string::npos) {
                return false;
            }
            factor = 1000000.0;
        }
    }
    d[i] = '.';
    if (!fio::read_list(d, x1)) {
        return false;
    }
    width = nint(R(x1) * factor);
    return true;
}

float corr_tx_tetra(float omega, float b1, float b2) {
    const float ab1 = piecewise<3>(omega, 0.5F, {{{1.0F, 32.0F, -16.0F}, {1.4F, 112.0F, -96.0F}, {0.0F, 41.0F, 0.0F}}});
    const float as = piecewise<3>(omega, 0.4F, {{{0.7F, 50.0F, -21.0F}, {1.0F, 225.0F, -145.0F}, {0.0F, -20.0F, 100.0F}}});
    return blend(as, ab1, b1, b2);
}

float corr_rx_tetra(float omega, float b1, float b2) {
    const float ab1 =
        piecewise<3>(omega, 0.45F, {{{0.63F, 55.0F, -23.0F}, {0.93F, 180.0F, -100.0F}, {0.0F, 12.5F, 57.0F}}});
    const float as = piecewise<2>(omega, 0.45F, {{{0.7F, 225.0F, -101.0F}, {0.0F, 13.0F, 58.0F}}});
    return blend(as, ab1, b1, b2);
}

float corr_narrow(float omega, float b1, float b2) {
    const float ab1 = piecewise<3>(omega, 0.5F, {{{1.0F, 47.0F, -24.0F}, {1.3F, 80.0F, -55.0F}, {0.0F, 38.0F, 0.0F}}});
    const float as = piecewise<2>(omega, 0.5F, {{{1.3F, 88.0F, -44.0F}, {0.0F, 12.0F, 55.0F}}});
    return blend(as, ab1, b1, b2);
}

float corr_wide(float omega, float b1, float b2) {
    const float ab1 = piecewise<2>(omega, 0.5F, {{{2.0F, 33.3F, -16.7F}, {0.0F, 10.0F, 30.0F}}});
    const float as =
        piecewise<3>(omega, 0.5F, {{{1.25F, 66.7F, -33.3F}, {1.75F, 20.0F, 25.0F}, {0.0F, 4.8F, 51.6F}}});
    return blend(as, ab1, b1, b2);
}

// Korrektur fuer den Frequenzabstand. Liefert false bei Fehler (HCM_error
// gesetzt) oder wenn ausserhalb von p2p schon alles erledigt ist.
bool frequency_correction(State &s) {
    const bool tx_dig = std::string("1279").find(s.Desig_of_Tx_emis[5]) != std::string::npos;
    const bool tx_tetra = s.Desig_of_Tx_emis.substr(0, 7) == "25K0G7W";
    const bool rx_tetra = s.Desig_of_Rx_emis.substr(0, 7) == "25K0G7W";
    float x1 = 0.0F;
    int csxt = kTetraWidth;
    if (!(tx_tetra && !rx_tetra) && !channel_width(s.Desig_of_Tx_emis, x1, csxt)) {
        s.HCM_error = 1041;
        return false;
    }
    if (!s.p2p) {
        s.Delta_frequency = 0;
        if (tx_dig && csxt > kTetraWidth && s.Tx_frequency <= 470.0 && s.Perm_FS_input == "     ") {
            s.Perm_FS = f32(R(s.Perm_FS) + 6.0 * R(log10_f(f32(R(static_cast<float>(csxt)) / 25000.0))));
        }
        return false;
    }
    if (tx_tetra && rx_tetra) {
        if (s.Delta_frequency <= 24999) {
            s.Corr_delta_f = 0.0F;
        } else if (s.Delta_frequency <= 50000) {
            s.Corr_delta_f = 45.0F;
        } else {
            s.Corr_delta_f = 70.0F;
        }
        return true;
    }
    int csxr = kTetraWidth;
    if (!(rx_tetra && !tx_tetra) && !channel_width(s.Desig_of_Rx_emis, x1, csxr)) {
        s.HCM_error = 1040;
        return false;
    }
    const float b1 = static_cast<float>(std::max(csxt, csxr));
    const float b2 = static_cast<float>(std::min(csxt, csxr));
    const float omega = f32(R(static_cast<float>(s.Delta_frequency)) / b1);
    if (tx_tetra && csxr <= 25000) {
        s.Corr_delta_f = corr_tx_tetra(omega, b1, b2);
        return true;
    }
    if (rx_tetra && csxt <= 25000) {
        s.Corr_delta_f = corr_rx_tetra(omega, b1, b2);
        return true;
    }
    s.Corr_delta_f = corr_narrow(omega, b1, b2);
    if (csxt <= 25000 && csxr <= 25000) {
        return true;
    }
    const float cdfn = s.Corr_delta_f;
    s.Corr_delta_f = corr_wide(omega, b1, b2);
    if (csxr >= 200000 || csxt >= 200000) {
        return true;
    }
    const float cdfw = s.Corr_delta_f;
    s.Corr_delta_f = f32(R(cdfn) + ((R(cdfw) - cdfn) / 175.0) * (R(b1) / 1000.0 - 25.0));
    return true;
}

bool rx_antenna(State &s) {
    if (s.Ant_typ_V_Rx == "000ND00" && s.Ant_typ_H_Rx == "000ND00") {
        return true;
    }
    if (!fio::read_list(s.Ele_Rx_input, s.Rx_Elevation) && s.Ant_typ_V_Rx != "000ND00") {
        s.HCM_error = 1042;
        return false;
    }
    if (!fio::read_list(s.Azi_Rx_input, s.Rx_Azimuth) && s.Ant_typ_H_Rx != "000ND00") {
        s.HCM_error = 1043;
        return false;
    }
    antenna_correction(s.Dir_Rx_Tx, s.Rx_Azimuth, s.V_angle_Rx_Tx, s.Rx_Elevation, s.H_diff_angle_Rx_Tx,
                       s.V_diff_angle_Rx_Tx, s.Ant_typ_H_Rx, s.Ant_typ_V_Rx, s.Rx_ant_corr, s.HCM_error);
    return s.HCM_error == 0;
}

bool depolarization(State &s, float &dpn) {
    if (!fio::read_list(s.Depol_loss, dpn)) {
        s.HCM_error = 1046;
        return false;
    }
    if (s.Depol_loss == "-9.9") {
        const float x1 = f32(R(s.Free_space_FS) - s.Calculated_FS);
        if (x1 < 50.0F && s.H_AntRx > 10 && s.H_AntTx > 10 && R(s.Tx_serv_area) + s.Rx_serv_area == 0.0 &&
            (s.Rx_ant_corr <= 10.0F || s.Tx_ant_corr <= 10.0F)) {
            dpn = f32(25.0 - 0.5 * R(x1));
        } else {
            dpn = 0.0F;
        }
        s.Depol_loss = fio::write_f41(dpn);
    }
    return true;
}

}  // namespace

void permissible_fs_calculation(State &s) {
    s.V_angle_Rx_Tx = 0.0;
    s.Rx_ant_corr = 0.0F;
    s.H_diff_angle_Rx_Tx = 0.0F;
    s.V_diff_angle_Rx_Tx = 0.0F;
    s.Channel_sp_Tx = 0;
    s.Channel_sp_Rx = 0;
    s.Corr_delta_f = 0.0F;
    s.Rx_ant_type_corr = 0.0F;
    s.Delta_frequency = static_cast<std::int64_t>(std::round(std::fabs(s.Tx_frequency - s.Rx_frequency) * 1e6));
    if (s.Cor_fact_frequ_diff != "    ") {
        s.Info[14] = true;
        if (!fio::read_list(s.Cor_fact_frequ_diff, s.Corr_delta_f)) {
            s.HCM_error = 1039;
            return;
        }
    } else if (!frequency_correction(s)) {
        return;
    }
    s.Perm_FS = f32(R(s.Perm_FS) + s.Corr_delta_f);
    s.V_angle_Rx_Tx = datan2d(static_cast<double>(s.H_Tx + s.H_AntTx - (s.H_Rx + s.H_AntRx)), 1e3 * s.Distance);
    if (!rx_antenna(s)) {
        return;
    }
    s.Perm_FS = f32(R(s.Perm_FS) + s.Rx_ant_corr);
    if (s.Type_of_Rx_ant == "E") {
        s.Rx_ant_type_corr = 0.0F;
    } else if (s.Type_of_Rx_ant == "I") {
        s.Rx_ant_type_corr = 2.1F;
    } else {
        s.HCM_error = 1044;
        return;
    }
    s.Perm_FS = f32(R(s.Perm_FS) + s.Rx_ant_type_corr);
    // GANT und DPN sind im Original lokale Variablen ohne Anfangswert; ein
    // reiner Nullwert ('/', ',', 'r*') liesse sie undefiniert (ADR-0004).
    float gant = 0.0F;
    if (!fio::read_list(s.Rx_ant_gain, gant)) {
        s.HCM_error = 1045;
        return;
    }
    s.Perm_FS = f32(R(s.Perm_FS) - gant);
    float dpn = 0.0F;
    if (!depolarization(s, dpn)) {
        return;
    }
    s.Perm_FS = f32(R(s.Perm_FS) + dpn);
}

}  // namespace hcmms
