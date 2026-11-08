#include "antenna.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "fmath.hpp"
#include "fortran_io.hpp"

namespace hcmms {

namespace {

constexpr int kAntError = 1038;
constexpr float kPiF = 3.14159265F;  // PI des Originals (REAL)

struct Pattern {
    float lead = 0.0F;
    float trail = 0.0F;
    float colea = 0.0F;
    float term1 = 0.0F;
    float coal = 0.0F;
    float alpha = 0.0F;
};

// Elliptische Diagramme EB/EC: gemeinsame Form mit Parametern c1..c5.
struct Ellipse {
    float q;     // 1.44 / 1.96
    float off;   // 0.8 / 0.6
    float f;     // 0.72 / 0.98
    float x;     // 0.2 / 0.4
    float lin;   // 1.6 / 1.2
    float root;  // 2.4 / 2.8
};

float type_ea(const Pattern &p) {
    const float term2 = f32(1.0 - sq(R(sqrt_f(2.0F)) * p.colea - 1.0));
    const float b = f32(0.5 * p.term1 / term2);
    return f32(4.0 * b * p.coal / ((4.0 * b - 1.0) * sq(p.coal) + 1.0));
}

float type_ellipse(const Pattern &p, const Ellipse &e) {
    const float term2 = f32(R(e.q) - sq(R(sqrt_f(2.0F)) * p.colea - R(e.off)));
    const float b = f32(R(e.f) * p.term1 / term2);
    float x = f32(R(b) * (R(b) - R(e.x)) * sq(p.coal) + R(e.x) * b);
    if (x < 0.0F) {
        x = 0.0F;
    }
    return f32((R(e.lin) * b * p.coal + R(e.root) * R(sqrt_f(x))) / ((4.0 * b - R(e.q)) * sq(p.coal) + R(e.q)));
}

float type_de(const Pattern &p) {
    const float term2 = f32(2.0 - sq(2.0 * p.colea - R(sqrt_f(2.0F))));
    const float b = f32(R(p.term1) / term2);
    return f32(std::fabs(4.0 * b * p.coal / ((4.0 * b - 1.0) * sq(p.coal) + 1.0)));
}

float type_la(const Pattern &p) {
    const float b = f32(R(p.alpha) * kPiF / 180.0);
    if (std::fabs(R(b)) <= 3.0 * (R(p.lead) * kPiF / 180.0) / 2.0) {
        const float term2 = cos_f(f32(R(kPiF) / 3.0 * b / (R(p.lead) * kPiF / 180.0)));
        return cos_f(f32((1.0 - term2) * kPiF / 2.0));
    }
    return p.trail;
}

float type_ka(const Pattern &p) {
    const float b = f32(R(p.lead) / 100.0);
    float x = f32(sq(1.0 - b) * sq(p.coal) + 4.0 * b);
    if (x < 0.0F) {
        x = 0.0F;
    }
    const float term2 = sqrt_f(x);
    return f32(((1.0 - b) * p.coal + term2) / 2.0);
}

// CA/CB/CC: Kardioide mit Teiler 90/60/45.
float type_cardioid(const Pattern &p, double divisor) {
    const float b = f32(R(p.lead) / 100.0);
    const float c = cos_f(f32(R(p.alpha) * kPiF / divisor));
    float x = f32(sq(1.0 - sq(b)) * sq(c) + 4.0 * sq(b));
    if (x < 0.0F) {
        x = 0.0F;
    }
    const float term2 = sqrt_f(x);
    x = f32(((1.0 - sq(b)) * c + term2) / 2.0);
    if (x < 0.0F) {
        x = 0.0F;
    }
    return sqrt_f(x);
}

float type_ta(const Pattern &p) {
    if (p.coal > 0.0F) {
        const float x = f32(-R(0.1505F) / R(log10_f(cosd(f32(R(p.lead) / 10.0)))));
        return f32(std::pow(R(p.coal), R(x)));
    }
    return 0.01F;
}

float e_value(char c) {
    switch (c) {
        case 'A': return 1E-5F;
        case 'B': return 0.05F;
        case 'C': return 0.10F;
        case 'D': return 0.15F;
        case 'E': return 0.20F;
        case 'F': return 0.25F;
        case 'G': return 0.30F;
        case 'H': return 0.35F;
        case 'I': return 0.40F;
        default: return 1.0F;
    }
}

float lobe(float k1, float k2, float k3, float k4, float k5, float c) {
    return f32((R(k1) * c + R(sqrt_f(f32(R(k2) * sq(c) + k3)))) / (R(k4) * sq(c) + k5));
}

// V/W-Diagramme. `trail` wird fuer W auf 0.01 gesetzt.
bool type_vw(const std::string &t, const Pattern &p, float &rho, float &trail) {
    int m = 0;
    int n = 0;
    if (!fio::read_i(t.substr(0, 1), m) || !fio::read_i(t.substr(1, 2), n)) {
        return false;
    }
    const int a = m * 5 + 15;
    const float e = e_value(t[4]);
    if (e == 1.0F) {
        return false;
    }
    const float ca = cosd(static_cast<float>(a));
    const float k5 = f32(sq((1.0 + e) / 2.0));
    const float b = f32(R(k5) / 2.0 * (1.0 - sq(ca)) / (R(k5) - sq(R(ca) / R(sqrt_f(2.0F)) - (1.0 - e) / 2.0)));
    const float k4 = f32(R(b) - k5);
    const float k3 = f32(R(b) * e * k5);
    const float k2 = f32(sq(b) * k5 - k3);
    const float k1 = f32(R(b) * (1.0 - e) / 2.0);
    const float r1 = lobe(k1, k2, k3, k4, k5, p.coal);
    const float coax = cosd(f32(R(p.alpha) - 2.0 * static_cast<float>(n)));
    const float r2 = lobe(k1, k2, k3, k4, k5, coax);
    rho = r1;
    if (rho < r2) {
        rho = r2;
    }
    if (t[3] == 'W') {
        int r = 0;
        int pp = 0;
        if (!fio::read_i(t.substr(5, 1), r) || !fio::read_i(t.substr(6, 1), pp)) {
            return false;
        }
        const float r0 = f32(static_cast<float>(r) / 20.0);
        const float p0 = f32(static_cast<float>(pp) / 20.0 + R(0.35F));
        if (p.alpha >= 0.0F && R(p.alpha) <= 2.0 * static_cast<float>(n)) {
            rho = std::max(rho, p0);
        } else {
            rho = std::max(rho, r0);
        }
        trail = 0.01F;
    }
    return true;
}

// Zulaessige Hauptkeulenangabe je Diagrammtyp (Typ als zwei Zeichen).
struct LeadLimit {
    const char *code;
    float max;
    bool nonzero;
};

constexpr std::array<LeadLimit, 9> kLeadLimits = {{
    {"EA", 65.0F, true},
    {"EB", 79.0F, true},
    {"EC", 96.0F, true},
    {"DE", 65.0F, true},
    {"LA", 120.0F, true},
    {"KA", 100.0F, false},
    {"CA", 100.0F, false},
    {"CB", 100.0F, false},
    {"CC", 100.0F, false},
}};

// Typ mit festem Diagramm: Tabelle oben, TA und P* (nicht V/W).
bool shaped_type(const std::string &tpe) {
    if (tpe == "TA" || tpe[0] == 'P') {
        return true;
    }
    return std::any_of(kLeadLimits.begin(), kLeadLimits.end(),
                       [&](const LeadLimit &l) { return tpe == l.code; });
}

bool lead_valid(const std::string &tpe, float lead) {
    for (const LeadLimit &l : kLeadLimits) {
        if (tpe == l.code) {
            return lead <= l.max && !(l.nonzero && lead == 0.0F);
        }
    }
    return lead >= 1.0F && lead <= 890.0F;  // TA und P*
}

float shaped_value(const std::string &tpe, const Pattern &p) {
    if (tpe == "EA") return type_ea(p);
    if (tpe == "EB") return type_ellipse(p, {1.44F, 0.8F, 0.72F, 0.2F, 1.6F, 2.4F});
    if (tpe == "EC") return type_ellipse(p, {1.96F, 0.6F, 0.98F, 0.4F, 1.2F, 2.8F});
    if (tpe == "DE") return type_de(p);
    if (tpe == "LA") return type_la(p);
    if (tpe == "KA") return type_ka(p);
    if (tpe == "CA") return type_cardioid(p, 90.0);
    if (tpe == "CB") return type_cardioid(p, 60.0);
    if (tpe == "CC") return type_cardioid(p, 45.0);
    return type_ta(p);
}

// Liefert false bei ungueltigem Typ; `rho` bleibt dann 1.0.
bool pattern_value(const std::string &t, Pattern &p, float &rho) {
    const std::string tpe = t.substr(3, 2);
    if (shaped_type(tpe)) {
        if (!lead_valid(tpe, p.lead)) {
            return false;
        }
        rho = shaped_value(tpe, p);
        return true;
    }
    if (t[3] == 'V' || t[3] == 'W') {
        return type_vw(t, p, rho, p.trail);
    }
    return false;
}

}  // namespace

float antenna(const std::string &a_typ, float angle, int &error) {
    float rho = 1.0F;
    if (a_typ.substr(3, 2) == "ND") {
        return rho;
    }
    Pattern p;
    p.alpha = (angle < 180.0F) ? angle : f32(R(angle) - 360.0);
    if (!fio::read_f(a_typ.substr(0, 3), 0, p.lead) || !fio::read_f(a_typ.substr(5, 2), 0, p.trail)) {
        error = kAntError;
        return rho;
    }
    p.trail = f32(R(p.trail) / 100.0);
    p.colea = cosd(p.lead);
    p.term1 = f32(1.0 - sq(p.colea));
    p.coal = cosd(p.alpha);
    float value = rho;
    if (!pattern_value(a_typ, p, value)) {
        error = kAntError;
        return value;  // 1.0, beim W-Diagramm der schon berechnete Wert
    }
    rho = value;
    if (rho < p.trail) rho = p.trail;
    if (rho < 0.01F) rho = 0.01F;
    if (rho > 1.0F) rho = 1.0F;
    return rho;
}

void ctransf(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda) {
    const double a = sq(dsind((ele - ele_m) / 2.0)) + dcosd(ele_m) * dcosd(ele) * sq(dsind((azi - azi_m) / 2.0));
    double d = 180.0;
    if (a < 1.0) {
        d = 2.0 * datan2d(std::sqrt(a), std::sqrt(1.0 - a));
    }
    const double b = datan2d(dsind(azi - azi_m) * dcosd(ele),
                             dcosd(ele_m) * dsind(ele) - dsind(ele_m) * dcosd(ele) * dcosd(azi - azi_m));
    vda = f32(dasind(dsind(d) * dcosd(b)));
    hda = f32(datan2d(dsind(d) * dsind(b), dcosd(d)));
}

namespace {

float corr_tilt(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda, const std::string &h_cod,
                const std::string &v_cod, int &error) {
    hda = f32(azi - azi_m);
    vda = f32(ele - ele_m);
    const float hb = antenna(h_cod, hda, error);
    const float vb = antenna(v_cod, vda, error);
    const float w = f32((1.0 + dcosd(2.0 * ele)) / 2.0);
    return f32((R(hb) * w + (1.0 - w)) * vb);
}

float corr_p(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda, const std::string &h_cod,
             const std::string &v_cod, int &error) {
    ctransf(azi, azi_m, ele, ele_m, hda, vda);
    const float ele_e = static_cast<float>(65 - static_cast<unsigned char>(v_cod[4]));
    const float d_ele = f32(R(vda) - ele_e);
    const float hb = antenna(h_cod, hda, error);
    const float vb = antenna(v_cod, d_ele, error);
    const float k = f32((90.0 + ele_e) / 90.0);
    const float w1 = (d_ele > 0.0F) ? f32(R(d_ele) * k) : f32(R(d_ele) / k);
    const float w = f32(std::pow((1.0 + R(cosd(f32(2.0 * w1)))) / 2.0, 6.0));
    const float w2 = antenna(v_cod, 180.0F, error);
    return f32(std::max(R(hb) * vb, (R(hb) * w + (1.0 - w)) * w2));
}

float clamp_unit(float v) {
    if (v > 1.0F) return 1.0F;
    if (v < 0.01F) return 0.01F;
    return v;
}

struct Lobes {
    float vfe = 1.0F;
    float vbe = 1.0F;
    float vb = 1.0F;
    float h = 1.0F;
    float hb = 1.0F;
};

// Rueckwaertige Daempfung vbe nach dem Verhaeltnis von vb zu hb.
void adjust_back(Lobes &l, float vda) {
    if (l.vb > l.hb) {
        const float k = f32(R(l.hb) / l.vb);
        l.vbe = f32(R(l.vbe) * R(sqrt_f(f32(sq(sind(vda)) + sq(R(k) * cosd(vda))))));
        if (l.vbe < 0.01F) l.vbe = 0.01F;
    } else if (l.vb < l.hb) {
        const float delv = f32(R(l.vfe) - l.vbe);
        if (delv > 0.0F) {
            const float k = f32((R(l.hb) - l.vb) / (1.0 - l.vb));
            l.vbe = f32(R(l.vbe) + R(k) * delv);
            if (l.vbe > 1.0F) l.vbe = 1.0F;
        }
    }
}

float weight(const Lobes &l, float hda) {
    if (l.hb == 1.0F) {
        return f32(std::fabs(R(hda)) / 180.0);
    }
    if (l.hb < 0.9F) {
        return std::min(f32((1.0 - l.h) / (1.0 - l.hb)), 1.0F);
    }
    const float w1 = f32(std::fabs(R(hda)) / 180.0);
    const float w2 = std::min(f32((1.0 - l.h) / (1.0 - l.hb)), 1.0F);
    const float k = std::min(f32((1.0 - l.hb) * 10.0), 1.0F);
    return f32((1.0 - k) * w1 + R(k) * w2);
}

float corr_general(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda, const std::string &h_cod,
                   const std::string &v_cod, int &error) {
    ctransf(azi, azi_m, ele, ele_m, hda, vda);
    Lobes l;
    if (!(vda == 0.0F || v_cod.substr(3, 2) == "ND")) {
        l.vfe = antenna(v_cod, -vda, error);
        l.vbe = antenna(v_cod, f32(180.0 + vda), error);
        l.vb = antenna(v_cod, 180.0F, error);
    }
    if (error != 0) return 1.0F;
    if (!(hda == 0.0F || h_cod.substr(3, 2) == "ND")) {
        l.h = antenna(h_cod, hda, error);
        l.hb = antenna(h_cod, 180.0F, error);
    }
    if (error != 0) return 1.0F;
    adjust_back(l, vda);
    const float w = weight(l, hda);
    const float vae = clamp_unit(f32(R(w) * l.vbe + (1.0 - w) * l.vfe));
    const float va0 = clamp_unit(f32(R(w) * l.hb + (1.0 - w)));
    const float ra = f32(R(l.h) / va0);
    return f32(R(vae) * R(sqrt_f(f32(sq(sind(vda)) + sq(R(ra) * cosd(vda))))));
}

}  // namespace

void antenna_correction(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda,
                        const std::string &h_cod, const std::string &v_cod, float &a, int &error) {
    if (v_cod[3] == 'T') {
        a = corr_tilt(azi, azi_m, ele, ele_m, hda, vda, h_cod, v_cod, error);
    } else if (v_cod[3] == 'P') {
        a = corr_p(azi, azi_m, ele, ele_m, hda, vda, h_cod, v_cod, error);
    } else {
        a = corr_general(azi, azi_m, ele, ele_m, hda, vda, h_cod, v_cod, error);
        if (error != 0) {
            return;  // Original: Ruecksprung mit a = 1.0 vor der dB-Umrechnung
        }
    }
    if (a < 0.01F) a = 0.01F;
    if (a > 1.0F) a = 1.0F;
    a = f32(-20.0 * R(log10_f(a)));
}

}  // namespace hcmms
