// Numerische Regeln des Originals (spezifikation.md §1, LH-FA-03.a):
// REAL-Variablen sind float, Ausdruecke laufen in double und werden bei der
// Zuweisung mit f32() gerundet; Intrinsics auf REAL-Argumenten liefern float.
#pragma once

#include <cmath>
#include <cstdint>

namespace hcmms {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kRadToDeg = 180.0 / kPi;
inline constexpr double kDegToRad = kPi / 180.0;

// Zuweisung an eine REAL-Variable.
inline float f32(double x) { return static_cast<float>(x); }

// Wert eines REAL-Literals, wie es in einen Ausdruck eingeht.
inline constexpr double R(float x) { return static_cast<double>(x); }

// NINT / IDNINT: halbe Werte von null weg.
inline int nint(double x) { return static_cast<int>(std::lround(x)); }

// Winkelfunktionen in Grad. Vielfache von 90 Grad liefern exakte Werte.
inline double dsind(double x) {
    const double r = std::fmod(x, 360.0);
    if (r == 0.0 || r == 180.0 || r == -180.0) {
        return 0.0;
    }
    if (r == 90.0 || r == -270.0) {
        return 1.0;
    }
    if (r == 270.0 || r == -90.0) {
        return -1.0;
    }
    return std::sin(r * kDegToRad);
}

inline double dcosd(double x) {
    const double r = std::fmod(x, 360.0);
    if (r == 90.0 || r == 270.0 || r == -90.0 || r == -270.0) {
        return 0.0;
    }
    if (r == 0.0) {
        return 1.0;
    }
    if (r == 180.0 || r == -180.0) {
        return -1.0;
    }
    return std::cos(r * kDegToRad);
}

inline double datan2d(double y, double x) { return std::atan2(y, x) * kRadToDeg; }
inline double dasind(double x) { return std::asin(x) * kRadToDeg; }
inline double dacosd(double x) { return std::acos(x) * kRadToDeg; }

// REAL-Intrinsics: Argument und Ergebnis sind float.
inline float sind(float x) { return f32(dsind(x)); }
inline float cosd(float x) { return f32(dcosd(x)); }
inline float atand(float x) { return f32(std::atan(static_cast<double>(x)) * kRadToDeg); }
inline float sqrt_f(float x) { return f32(std::sqrt(static_cast<double>(x))); }
inline float log10_f(float x) { return f32(std::log10(static_cast<double>(x))); }
inline float cos_f(float x) { return f32(std::cos(static_cast<double>(x))); }

// Quadrat (x**2 und x**2.0 des Originals).
inline double sq(double x) { return x * x; }

}  // namespace hcmms
