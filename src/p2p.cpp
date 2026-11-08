#include "p2p.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "antenna.hpp"
#include "figures.hpp"
#include "fmath.hpp"
#include "fortran_io.hpp"
#include "mobile.hpp"
#include "terrain.hpp"

namespace hcmms {

namespace {

std::size_t ux(int i) { return static_cast<std::size_t>(i); }

// Gerade (A - x0) * slope + off einer Dh-Korrekturkurve.
struct Segment {
    float x0;
    float slope;
    float off;
};

using Curve = std::array<Segment, 8>;

// Abschnittsgrenzen: <=20, <=30, <=50, <=80, <=100, <=150, <=300, >300.
constexpr std::array<float, 7> kDhBounds = {20.0F, 30.0F, 50.0F, 80.0F, 100.0F, 150.0F, 300.0F};

constexpr Curve kA1_100 = {{{10, 0.3F, -7.0F}, {30, 0.15F, -2.5F}, {50, 0.125F, 0.0F}, {80, 0.1F, 3.0F},
                            {100, 0.1F, 5.0F}, {150, 0.06F, 8.0F}, {300, 0.04F, 14.0F}, {500, 0.025F, 19.0F}}};
constexpr Curve kA2_100 = {{{20, 0.1F, -2.0F}, {30, 0.05F, -1.5F}, {50, 0.075F, 0.0F}, {80, 0.067F, 2.0F},
                            {100, 0.05F, 3.0F}, {150, 0.03F, 4.5F}, {300, 0.0167F, 7.0F}, {500, 0.0125F, 9.5F}}};
constexpr Curve kA1_600 = {{{10, 0.4F, -10.0F}, {30, 0.3F, -3.0F}, {50, 0.15F, 0.0F}, {80, 0.133F, 4.0F},
                            {100, 0.15F, 7.0F}, {150, 0.06F, 10.0F}, {300, 0.067F, 20.0F}, {500, 0.04F, 28.0F}}};
constexpr Curve kA2_600 = {{{20, 0.2F, -3.0F}, {30, 0.1F, -2.0F}, {50, 0.1F, 0.0F}, {80, 0.067F, 2.0F},
                            {100, 0.075F, 3.5F}, {150, 0.03F, 5.0F}, {300, 0.033F, 10.0F}, {500, 0.015F, 13.0F}}};
constexpr Curve kA1_2000 = {{{10, 0.4F, -10.0F}, {30, 0.3F, -3.0F}, {50, 0.15F, 0.0F}, {80, 0.1667F, 5.0F},
                             {100, 0.185F, 8.7F}, {150, 0.074F, 12.4F}, {300, 0.082667F, 24.8F},
                             {500, 0.0495F, 34.7F}}};
constexpr Curve kA2_2000 = {{{20, 0.2F, -3.0F}, {30, 0.1F, -2.0F}, {50, 0.1F, 0.0F}, {80, 0.08333F, 2.5F},
                             {100, 0.09F, 4.3F}, {150, 0.038F, 6.2F}, {300, 0.041333F, 12.4F},
                             {500, 0.0185F, 16.1F}}};

float curve_value(const Curve &c, float a) {
    std::size_t k = 0;
    while (k < kDhBounds.size() && a > kDhBounds[k]) {
        ++k;
    }
    return f32((R(a) - R(c[k].x0)) * R(c[k].slope) + R(c[k].off));
}

// C-Wert je Frequenz aus den Kurven A1 (bis 200 km) und A2 (ab 100 km).
float c_value(float a1, float a2, double distance) {
    if (distance <= 50.0) {
        return f32(R(a1) * (distance - 10.0) / 40.0);
    }
    if (distance <= 100.0) {
        return a1;
    }
    if (distance <= 200.0) {
        return f32(R(a2) - (distance - 200.0) * (R(a1) - a2) / 100.0);
    }
    return a2;
}

// Interpolation zwischen zwei Stuetzfrequenzen wie im Original
// (Faktor LOG10(f/f_inf) * LOG10(f_sup/f_inf)).
float freq_interp(float c_lo, float c_hi, double frequency, double f_inf, float ratio) {
    return f32(R(c_lo) + (R(c_hi) - c_lo) * std::log10(frequency / f_inf) * R(log10_f(ratio)));
}

}  // namespace

float tca_correction(const State &s, float tca, double frequency) {
    const auto corr = [](float v, float k, float lo) {
        const float arg = f32(R(sqrt_f(f32(sq(R(v) - R(0.1F)) + 1.0))) + v - R(0.1F));
        float c = f32(R(k) - (R(6.9F) + 20.0 * R(log10_f(arg))));
        if (c >= 0.0F) {
            c = 0.0F;
        }
        if (c < lo) {
            c = lo;
        }
        return c;
    };
    const float c100 = corr(f32(R(0.649F) * tca), 9.1F, -32.0F);
    const float c600 = corr(f32(R(1.592F) * tca), 13.1F, -35.0F);
    const float c2000 = corr(f32(R(2.915F) * tca), 17.3F, -36.0F);
    float r = 0.0F;
    if (frequency <= 6.0e2) {
        r = freq_interp(c100, c600, frequency, 100.0, f32(600.0 / 100.0));
    } else {
        r = freq_interp(c600, c2000, frequency, 600.0, f32(2000.0 / 600.0));
    }
    if (s.Distance < 1.6e1) {
        r = f32(R(r) * s.Distance / 16.0);
    }
    return r;
}

void dh_calculation(State &s) {
    std::vector<int> hi;
    const int i0 = nint(4.5 / s.PD);
    const int l0 = nint(2.5e1 / s.PD);
    if (s.Distance <= 5.0e1) {
        for (int j = i0 + 1; j <= s.PN - i0; ++j) {
            hi.push_back(s.T_Prof[ux(j)]);
        }
    } else {
        for (int j = i0 + 1; j <= l0 + 1; ++j) {
            hi.push_back(s.T_Prof[ux(j)]);
        }
        for (int j = s.PN - l0; j <= s.PN - i0; ++j) {
            hi.push_back(s.T_Prof[ux(j)]);
        }
    }
    // Das Original sortiert aufsteigend (Shell-Sort); das Ergebnis haengt
    // nur von der Reihenfolge der Werte ab.
    std::sort(hi.begin(), hi.end());
    const int n = static_cast<int>(hi.size());
    if (n == 0) {
        return;
    }
    if (n < 5) {
        s.Dh = static_cast<float>(hi[ux(n - 1)] - hi[0]);
    } else {
        const int k = nint(static_cast<double>(n) / 10.0);
        s.Dh = static_cast<float>(hi[ux(n - k - 1)] - hi[ux(k - 1)]);
    }
}

float dh_correction(float dh, double distance, double frequency) {
    if (distance < 1.0e1 || dh == 50.0F) {
        return 0.0F;
    }
    float a = dh;
    if (dh <= 10.0F) {
        a = 10.0F;
    }
    if (dh >= 500.0F) {
        a = 500.0F;
    }
    const bool low = (frequency <= 6.0e2);
    const auto pick = [&](const Curve &c1, const Curve &c2, bool used) {
        const float a1 = (used && distance < 200.0) ? curve_value(c1, a) : 0.0F;
        const float a2 = (used && distance > 100.0) ? curve_value(c2, a) : 0.0F;
        return c_value(a1, a2, distance);
    };
    const float c100 = pick(kA1_100, kA2_100, low);
    const float c600 = pick(kA1_600, kA2_600, true);
    const float c2000 = pick(kA1_2000, kA2_2000, !low);
    if (low) {
        return freq_interp(c100, c600, frequency, 100.0, f32(600.0 / 100.0));
    }
    return freq_interp(c600, c2000, frequency, 600.0, f32(2000.0 / 600.0));
}

namespace {

void reset_outputs(State &s) {
    s.HCM_error = 0;
    for (const int i : {1, 2, 3, 7, 8, 9, 10, 11, 12, 13}) {
        s.Info[ux(i)] = false;
    }
    s.Calculated_FS = 999.9F;
    s.PN = 0;
    s.Rx_TCA = 0.0F;
    s.Rx_TCA_corr = 0.0F;
    s.Tx_TCA = 0.0F;
    s.Tx_TCA_corr = 0.0F;
    s.Heff = 0.0F;
    s.Heff_Rx = 0.0F;
    s.Heff_Tx = 0.0F;
    // V7.21.5: D_sea_calculated wird erst in sea_distance() (with_morpho) genullt.
    s.Dh = 50.0F;
    s.Dh_corr = 0.0F;
    s.Land_FS = 0.0F;
    s.Sea_FS = 0.0F;
}

// Koordinaten auf ganze Sekunden runden (CooConv und Zuruecklesen).
void round_coordinates(State &s, GeoPoint &tx, GeoPoint &rx) {
    s.Coo_Tx_new = coo_conv(tx);
    s.Coo_Rx_new = coo_conv(rx);
    parse_coordinates(s.Coo_Tx_new, tx);
    parse_coordinates(s.Coo_Rx_new, rx);
}

// Hoehe ueber NN aus Eingabe oder Datenbank; setzt die Info-Flags
// (base, base+1, base+2). Liefert false bei Lesefehler.
bool station_height(const std::string &input, bool from_db, std::int16_t db, std::int16_t &h, State &s, int base) {
    if (input == "    " || from_db) {
        h = db;
        s.Info[ux(base)] = true;
        return true;
    }
    if (!fio::read_i(input, h)) {
        return false;
    }
    if (h != db) {
        if (std::abs(h - db) <= h / 10) {
            s.Info[ux(base + 1)] = true;
        } else {
            s.Info[ux(base + 2)] = true;
        }
    }
    return true;
}

bool tx_antenna(State &s) {
    if (s.Ant_typ_H_Tx == "000ND00" && s.Ant_typ_V_Tx == "000ND00") {
        s.Tx_ant_corr = 0.0F;
        s.H_diff_angle_Tx_Rx = 0.0F;
        s.V_diff_angle_Tx_Rx = 0.0F;
        return true;
    }
    if (!fio::read_f(s.Ele_Tx_input, 1, s.Tx_Elevation)) {
        s.HCM_error = 1031;
        return false;
    }
    if (!fio::read_f(s.Azi_Tx_input, 1, s.Tx_Azimuth)) {
        s.HCM_error = 1032;
        return false;
    }
    antenna_correction(s.Dir_Tx_Rx, s.Tx_Azimuth, s.V_angle_Tx_Rx, s.Tx_Elevation, s.H_diff_angle_Tx_Rx,
                       s.V_diff_angle_Tx_Rx, s.Ant_typ_H_Tx, s.Ant_typ_V_Tx, s.Tx_ant_corr, s.HCM_error);
    return s.HCM_error == 0;
}

// Pruefung der ersten Fresnelzone ueber dem normierten Profil.
void fresnel_check(State &s) {
    const int j = s.PN - 2;
    if (s.HDZ.size() < ux(s.PN) + 2) {
        s.HDZ.resize(ux(s.PN) + 2, 0.0F);
    }
    std::fill(s.HDZ.begin(), s.HDZ.end(), 0.0F);
    for (int i = 1; i <= j; ++i) {
        s.HDZ[ux(i)] = f32(static_cast<double>(s.T_Prof[ux(i + 1)]) +
                           s.PD * i * (s.Distance - s.PD * i) / 17.0);
    }
    if (s.Distance > 2.0) {
        const int i1 = nint(1.0 / s.PD);
        const int i2 = nint((s.Distance - 1.0) / s.PD);
        if (s.HDZ.size() < ux(i2) + 1) {
            s.HDZ.resize(ux(i2) + 1, 0.0F);
        }
        for (int i = i1; i <= i2; ++i) {
            s.HDZ[ux(i)] = f32(R(s.HDZ[ux(i)]) + 10.0);
        }
    }
    for (int i = 1; i <= j; ++i) {
        const float hdc = f32(s.H_AntRx * s.PD * i / s.Distance - s.HDZ[ux(i)]);
        const float x = f32(s.PD * i * (s.Distance - s.PD * i));
        float hdf = 0.0F;
        if (x > 0.0F) {
            hdf = f32(547.7 * std::sqrt(R(x) / (s.Distance * s.Tx_frequency)));
        }
        if (hdc < hdf) {
            return;
        }
    }
    s.Calculated_FS = s.Free_space_FS;
    s.Info[12] = true;
}

// Groesster Gelaendefreiwinkel ueber die ersten 16 km; `at(i)` liefert den
// i-ten Profilpunkt von der Station aus.
template <typename At>
float clearance_angle(const State &s, int h_ref, At at) {
    float tca = -90.0F;
    const int j = (s.Distance >= 1.6e1) ? nint(1.6e1 / s.PD) : s.PN - 2;
    for (int i = 1; i <= j; ++i) {
        float x = f32(static_cast<double>(at(i) - h_ref) / (R(f32(s.PD)) * static_cast<double>(i) * 1e3));
        x = atand(x);
        if (x > tca) {
            tca = x;
        }
    }
    return tca;
}

template <typename At>
float effective_height(int d1, int d2, int h_ref, At at) {
    int hsum = 0;
    for (int i = d1; i <= d2; ++i) {
        hsum += at(i);
    }
    return f32(static_cast<double>(h_ref) - static_cast<double>(hsum) / static_cast<double>(d2 - d1 + 1));
}

void heights_and_angles(State &s) {
    int i1 = 0;
    int i2 = 0;
    if (s.p2p) {
        i1 = s.H_AntTx;
        i2 = s.H_AntRx;
    } else {
        i1 = s.H_Tx + s.H_AntTx;
    }
    const auto from_tx = [&s](int i) { return static_cast<int>(s.T_Prof[ux(1 + i)]); };
    const auto from_rx = [&s](int i) { return static_cast<int>(s.T_Prof[ux(s.PN - i)]); };
    if (s.Tx_serv_area == 0.0F) {
        s.Tx_TCA = clearance_angle(s, i1, from_tx);
        s.Tx_TCA_corr = tca_correction(s, s.Tx_TCA, s.Tx_frequency);
    }
    if (s.p2p && s.Rx_serv_area == 0.0F) {
        // T_Prof(0) liegt im Original in /NDATA/ auf der oberen Haelfte von
        // flp (LOGICAL*4, .TRUE. = -1); der Freiwinkel liest es bei
        // NINT(16/PD) = PN (ADR-0006).
        s.T_Prof[0] = s.flp ? static_cast<std::int16_t>(-1) : static_cast<std::int16_t>(0);
        s.Rx_TCA = clearance_angle(s, i2, from_rx);
        s.Rx_TCA_corr = tca_correction(s, s.Rx_TCA, s.Rx_frequency);
    }
    int d1 = 0;
    int d2 = 0;
    if (s.Distance < 1.5e1) {
        d1 = nint(static_cast<double>(s.PN) / 15.0);
        d2 = s.PN - 1;
    } else {
        d1 = nint(1.0 / s.PD);
        d2 = nint(1.5e1 / s.PD);
    }
    s.Heff_Tx = (s.Tx_serv_area > 0.0F) ? static_cast<float>(s.H_AntTx) : effective_height(d1, d2, i1, from_tx);
    if (s.Heff_Tx < 3.0F) {
        s.Heff_Tx = 3.0F;
    }
    s.Heff_Rx = (!s.p2p || s.Rx_serv_area > 0.0F) ? static_cast<float>(s.H_AntRx)
                                                   : effective_height(d1, d2, i2, from_rx);
    if (s.Heff_Rx < 3.0F) {
        s.Heff_Rx = 3.0F;
    }
    s.Heff = f32(R(s.Heff_Tx) * s.Heff_Rx / 10.0);
}

void sea_distance(State &s) {
    if (!s.with_morpho) {
        return;
    }
    s.D_sea_calculated = 0.0;
    if (s.Distance <= 1.0) {
        return;
    }
    bool null = false;
    int j1 = 0;
    for (int i = 1; i <= s.PN; ++i) {
        if ((s.M_Prof[ux(i)] & 1) != 0) {
            if (null) {
                ++j1;
            } else {
                j1 = 0;
                null = true;
            }
        } else if (null) {
            s.D_sea_calculated += R(f32(static_cast<double>(j1) * s.PD));
            j1 = 0;
            null = false;
        }
    }
    if (null) {
        s.D_sea_calculated += R(f32(static_cast<double>(j1) * s.PD));
    }
}

constexpr std::array<float, 11> kMixed = {0.0F,  0.07F, 0.13F, 0.2F, 0.29F, 0.37F,
                                          0.46F, 0.56F, 0.67F, 0.8F, 1.0F};

void combine(State &s) {
    if (s.Info[11] || s.Info[12]) {
        return;
    }
    if (s.D_sea_calculated == 0.0) {
        s.Calculated_FS = s.Land_FS;
        return;
    }
    if (s.Time_percentage < 10) {
        const float factor = f32(s.D_sea_calculated / s.Distance);
        float x = f32(R(factor) * 10.0);
        const int ix = static_cast<int>(x);
        const float a1 = kMixed[ux(ix)];
        const float a2 = (x < 10.0F) ? kMixed[ux(ix + 1)] : a1;
        x = f32(R(x) - ix);
        const float ax = f32(R(a1) + R(x) * (R(a2) - a1));
        s.Calculated_FS = f32(R(s.Land_FS) + R(ax) * (R(s.Sea_FS) - s.Land_FS));
    } else {
        s.Calculated_FS = f32(R(s.Land_FS) * (s.Distance - s.D_sea_calculated) / s.Distance +
                              R(s.Sea_FS) * s.D_sea_calculated / s.Distance);
    }
}

// Ab dem Profil: Fresnelzone, Freiwinkel, Hoehen, Seeweg, Kurven.
void after_profile(State &s) {
    if (s.p2p && R(s.Tx_serv_area) + s.Rx_serv_area == 0.0) {
        fresnel_check(s);
    }
    heights_and_angles(s);
    sea_distance(s);
    if (s.D_sea_calculated > s.Distance) {
        s.Info[13] = true;
        s.D_sea_calculated = s.Distance;
    }
    if (!(s.Distance <= 10.0 || s.D_sea_calculated >= s.Distance || (s.Tx_serv_area > 0.0F && !s.p2p))) {
        dh_calculation(s);
        s.Dh_corr = dh_correction(s.Dh, s.Distance, s.Tx_frequency);
    }
    float land_1kw = 0.0F;
    float sea_1kw = 0.0F;
    get_fs_from_figures(s, land_1kw, sea_1kw);
    if (s.HCM_error != 0) {
        return;
    }
    s.Land_FS = f32(R(land_1kw) - 30.0 + s.Power_to_Rx);
    s.Sea_FS = f32(R(sea_1kw) - 30.0 + s.Power_to_Rx);
    s.Land_FS = f32(R(s.Land_FS) + s.Tx_TCA_corr + s.Rx_TCA_corr - s.Dh_corr);
    combine(s);
}

}  // namespace

void p_to_p_calculation(State &s, const GeoPoint &tx, const GeoPoint &rx) {
    reset_outputs(s);
    GeoPoint ntx = tx;
    GeoPoint nrx = rx;
    if (s.p2p && (s.Tx_serv_area > 0.0F || s.Rx_serv_area > 0.0F)) {
        position_of_mobile(s, tx, rx, ntx, nrx);
        round_coordinates(s, ntx, nrx);
    } else if (s.flp) {
        round_coordinates(s, ntx, nrx);
    }
    s.Distance = calc_distance(ntx, nrx);
    if (s.Distance < s.PD) {
        if (s.p2p) {
            s.HCM_error = 1000;
        } else {
            s.Info[7] = true;
        }
        return;
    }
    if (s.Distance > 1e3) {
        s.Calculated_FS = -999.9F;
        s.HCM_error = 1028;
        return;
    }
    s.Dir_Rx_Tx = calc_direction(nrx, ntx);
    s.Dir_Tx_Rx = calc_direction(ntx, nrx);
    point_info(s, nrx, s.H_Datab_Rx, s.M_Prof[kProfileSize]);
    if (s.HCM_error != 0) {
        return;
    }
    if (!station_height(s.H_Rx_input, !s.p2p || s.Rx_serv_area > 0.0F, s.H_Datab_Rx, s.H_Rx, s, 8)) {
        s.HCM_error = 1030;
        return;
    }
    point_info(s, ntx, s.H_Datab_Tx, s.M_Prof[1]);
    if (s.HCM_error != 0) {
        return;
    }
    if (!station_height(s.H_Tx_input, s.Tx_serv_area > 0.0F, s.H_Datab_Tx, s.H_Tx, s, 1)) {
        s.HCM_error = 1013;
        return;
    }
    s.V_angle_Tx_Rx = datan2d(static_cast<double>(s.H_Rx + s.H_AntRx - (s.H_Tx + s.H_AntTx)), 1e3 * s.Distance);
    if (!tx_antenna(s)) {
        return;
    }
    s.Power_to_Rx = f32(R(s.MaxPow) - s.Tx_ant_corr);
    s.Free_space_FS = f32(77.0 - 20.0 * std::log10(s.Distance) + s.Power_to_Rx);
    if (s.Distance < 1.0) {
        s.Calculated_FS = s.Free_space_FS;
        s.Info[11] = true;
        return;
    }
    profile(s, ntx, nrx);
    if (s.HCM_error != 0) {
        return;
    }
    after_profile(s);
}

}  // namespace hcmms
