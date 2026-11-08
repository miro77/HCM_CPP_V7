#include "figures.hpp"

#include <array>
#include <cmath>
#include <cstddef>

#include "figure_tables.hpp"
#include "fmath.hpp"

namespace hcmms {

namespace {

constexpr std::array<float, 8> kHeffValues = {10.0F, 20.0F, 37.5F, 75.0F, 150.0F, 300.0F, 600.0F, 1200.0F};
constexpr std::array<int, 78> kDistances = {
    1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
    25,  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95,  100, 110, 120, 130, 140,
    150, 160, 170, 180, 190, 200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475, 500, 525, 550,
    575, 600, 625, 650, 675, 700, 725, 750, 775, 800, 825, 850, 875, 900, 925, 950, 975, 1000};

// Tabellennummern (F01 = 1) je Frequenz und Zeitprozent: Land, See kalt, See warm.
struct FigureSet {
    int land;
    int sea_cold;
    int sea_warm;
};

FigureSet figure_set(int frequency, int time_percentage) {
    const int base = (frequency == 100) ? 0 : ((frequency == 600) ? 8 : 16);
    if (time_percentage == 50) {
        return {base + 1, base + 4, base + 4};
    }
    if (time_percentage == 10) {
        return {base + 2, base + 5, base + 7};
    }
    return {base + 3, base + 6, base + 8};
}

float table(int figure, std::size_t index) { return kFigures[static_cast<std::size_t>(figure - 1)][index]; }

struct Pair {
    float land = 0.0F;
    float sea = 0.0F;
};

// a + (b - a) * num / den mit Rundung auf REAL.
float lerp_log(float a, float b, double num, double den) { return f32(R(a) + (R(b) - a) * num / den); }

Pair lerp_pair(const Pair &a, const Pair &b, double num, double den) {
    return {lerp_log(a.land, b.land, num, den), lerp_log(a.sea, b.sea, num, den)};
}

// LOG10(FLOAT(a)/FLOAT(b)) — REAL-Ausdruck.
double float_log_ratio(double a, double b) { return R(log10_f(f32(a / b))); }

// Entfernungs-Grenzen d_inf/d_sup fuer x (Schleife ueber die Tabelle).
void distance_bounds(double x, int &d_inf, int &d_sup) {
    for (std::size_t i = 0; i < 77; ++i) {
        if (x >= static_cast<double>(kDistances[i])) {
            d_inf = kDistances[i];
            d_sup = kDistances[i + 1];
        }
    }
}

// Interpolation in Entfernung (Zaehler `dist_num`) und Frequenz aus den
// Werten bei (f_inf, d_inf), (f_inf, d_sup), (f_sup, d_inf), (f_sup, d_sup).
Pair interpolate_df(const State &s, const std::array<Pair, 4> &e, int f_inf, int f_sup, int d_inf, int d_sup,
                    double dist_num) {
    Pair i = e[0];
    Pair f = e[2];
    if (d_sup != d_inf) {
        const double den = float_log_ratio(d_sup, d_inf);
        i = lerp_pair(e[0], e[1], dist_num, den);
        f = lerp_pair(e[2], e[3], dist_num, den);
    }
    if (f_sup == f_inf) {
        return i;
    }
    const double num = std::log10(s.Tx_frequency / static_cast<double>(f_inf));
    return lerp_pair(i, f, num, float_log_ratio(f_sup, f_inf));
}

// Werte bei Hoehe 10 m, interpoliert ueber Entfernung und Frequenz.
bool h10_value(State &s, int f_inf, int f_sup, int d_inf, int d_sup, double dist_num, Pair &out) {
    std::array<Pair, 4> e;
    get_figure_fs_value(s, f_inf, 10.0F, d_inf, e[0].land, e[0].sea);
    get_figure_fs_value(s, f_inf, 10.0F, d_sup, e[1].land, e[1].sea);
    get_figure_fs_value(s, f_sup, 10.0F, d_inf, e[2].land, e[2].sea);
    get_figure_fs_value(s, f_sup, 10.0F, d_sup, e[3].land, e[3].sea);
    if (s.HCM_error != 0) {
        return false;
    }
    out = interpolate_df(s, e, f_inf, f_sup, d_inf, d_sup, dist_num);
    return true;
}

// Zweig Heff >= 10 m.
bool high_antenna(State &s, int f_inf, int f_sup, float h_inf, float h_sup, int d_inf, int d_sup, Pair &out) {
    std::array<Pair, 8> v;  // Index: f*4 + h*2 + d
    const std::array<int, 2> fs = {f_inf, f_sup};
    const std::array<float, 2> hs = {h_inf, h_sup};
    const std::array<int, 2> ds = {d_inf, d_sup};
    for (std::size_t k = 0; k < 8; ++k) {
        get_figure_fs_value(s, fs[k / 4], hs[(k / 2) % 2], ds[k % 2], v[k].land, v[k].sea);
    }
    if (s.HCM_error != 0) {
        return false;
    }
    const double hn = R(log10_f(f32(R(s.Heff) / h_inf)));
    const double hd = R(log10_f(f32(R(h_sup) / h_inf)));
    std::array<Pair, 4> e;  // Index: f*2 + d
    for (std::size_t k = 0; k < 4; ++k) {
        const std::size_t fi = k / 2;
        const std::size_t di = k % 2;
        e[k] = lerp_pair(v[fi * 4 + di], v[fi * 4 + 2 + di], hn, hd);
    }
    out = interpolate_df(s, e, f_inf, f_sup, d_inf, d_sup, std::log10(s.Distance / static_cast<double>(d_inf)));
    return true;
}

// Zweig Heff < 10 m.
bool low_antenna(State &s, int f_inf, int f_sup, int d_inf, int d_sup, float e_free, Pair &out) {
    const float dh1 = f32(R(4.1F) * R(sqrt_f(s.Heff)));
    const float dh10 = 12.96534F;
    if (s.Distance < R(dh1)) {
        Pair e10d;
        if (!h10_value(s, f_inf, f_sup, d_inf, d_sup, std::log10(s.Distance / static_cast<double>(d_inf)), e10d)) {
            return false;
        }
        Pair e10dh10;
        if (!h10_value(s, f_inf, f_sup, 12, 13, float_log_ratio(dh10, 12.0), e10dh10)) {
            return false;
        }
        Pair e10dh1{e_free, e_free};
        if (dh1 >= 1.0F) {
            int di = 12;
            int ds = 13;
            distance_bounds(R(dh1), di, ds);
            if (!h10_value(s, f_inf, f_sup, di, ds, float_log_ratio(dh1, di), e10dh1)) {
                return false;
            }
        }
        out = {f32(R(e10dh10.land) + e10d.land - e10dh1.land), f32(R(e10dh10.sea) + e10d.sea - e10dh1.sea)};
        return true;
    }
    const float dhx = f32(R(dh10) + s.Distance - dh1);
    if (dhx < 1.0F) {
        out = {e_free, e_free};
        return true;
    }
    int di = d_inf;
    int ds = d_sup;
    distance_bounds(R(dhx), di, ds);
    return h10_value(s, f_inf, f_sup, di, ds, float_log_ratio(dhx, di), out);
}

}  // namespace

void get_figure_fs_value(State &s, int frequency, float heff, int distance, float &land, float &sea) {
    land = 0.0F;
    sea = 0.0F;
    if (frequency != 100 && frequency != 600 && frequency != 2000) {
        s.HCM_error = 2000;
        return;
    }
    const int tp = s.Time_percentage;
    if (tp != 1 && tp != 10 && tp != 50) {
        s.HCM_error = 2001;
        return;
    }
    std::size_t i = 0;
    while (i < kHeffValues.size() && heff != kHeffValues[i]) {
        ++i;
    }
    if (i == kHeffValues.size()) {
        s.HCM_error = 2003;
        return;
    }
    std::size_t j = 0;
    while (j < kDistances.size() && distance != kDistances[j]) {
        ++j;
    }
    if (j == kDistances.size()) {
        s.HCM_error = 2004;
        return;
    }
    const std::size_t idx = j * 9 + i;
    const FigureSet set = figure_set(frequency, tp);
    land = table(set.land, idx);
    const float sf_c = table(set.sea_cold, idx);
    const float sf_w = table(set.sea_warm, idx);
    const char t = s.Sea_temperature[0];
    if (t == 'C') {
        sea = sf_c;
    } else if (t == 'W') {
        sea = sf_w;
    } else if (t == 'I') {
        sea = f32(R(sf_w) - ((R(sf_w) - sf_c) * (R(s.aLatM) - 35.0)) / 21.0);
    }
}

void get_fs_from_figures(State &s, float &land_1kw, float &sea_1kw) {
    const float e_free = f32(107.0 - 20.0 * std::log10(s.Distance));
    if (s.Distance < 1.0) {
        land_1kw = e_free;
        sea_1kw = e_free;
        return;
    }
    int d_inf = 0;
    int d_sup = 0;
    distance_bounds(s.Distance, d_inf, d_sup);
    const int f_inf = (s.Tx_frequency <= 6.0e2) ? 100 : 600;
    const int f_sup = (s.Tx_frequency <= 6.0e2) ? 600 : 2000;
    if (s.Heff < 0.0F) {
        s.Heff = 0.0F;
    }
    if (s.Heff > 3000.0F) {
        s.Heff = 3000.0F;
    }
    float h_inf = 10.0F;
    float h_sup = 10.0F;
    for (std::size_t i = 0; i < 7; ++i) {
        if (s.Heff >= kHeffValues[i]) {
            h_inf = kHeffValues[i];
            h_sup = kHeffValues[i + 1];
        }
    }
    Pair out;
    const bool ok = (s.Heff >= 10.0F) ? high_antenna(s, f_inf, f_sup, h_inf, h_sup, d_inf, d_sup, out)
                                      : low_antenna(s, f_inf, f_sup, d_inf, d_sup, e_free, out);
    if (ok) {
        land_1kw = out.land;
        sea_1kw = out.sea;
    }
}

}  // namespace hcmms
