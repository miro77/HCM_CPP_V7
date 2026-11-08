#include "fortran_io.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>

namespace hcmms::fio {

namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_blank(char c) { return c == ' ' || c == '\t'; }

// Zerlegte Dezimalzahl: Ziffern ohne Punkt, Zehnerexponent, Vorzeichen.
struct Decimal {
    bool negative = false;
    std::string digits;
    long exponent = 0;
};

// Exponententeil ab Position i: [E|D|Q][+|-]Ziffern oder +|-Ziffern.
bool parse_exponent(const std::string &t, std::size_t i, long &exp) {
    if (i >= t.size()) {
        return true;
    }
    if (t[i] == 'E' || t[i] == 'D' || t[i] == 'Q') {
        ++i;
    } else if (t[i] != '+' && t[i] != '-') {
        return false;
    }
    bool neg = false;
    if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
        neg = (t[i] == '-');
        ++i;
    }
    long e = 0;
    for (; i < t.size(); ++i) {
        if (!is_digit(t[i])) {
            return false;
        }
        if (e < 100000) {
            e = e * 10 + (t[i] - '0');
        }
    }
    exp = neg ? -e : e;
    return true;
}

// Zerlegt eine Zahl ohne Leerzeichen. `implied` ist d der F-Formatangabe
// (nur wirksam ohne Dezimalpunkt), `need_digit` verlangt mindestens eine Ziffer.
bool parse_decimal(const std::string &t, int implied, bool need_digit, Decimal &out) {
    std::size_t i = 0;
    if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
        out.negative = (t[i] == '-');
        ++i;
    }
    bool point = false;
    long frac = 0;
    bool any = false;
    for (; i < t.size(); ++i) {
        const char c = t[i];
        if (is_digit(c)) {
            out.digits.push_back(c);
            any = true;
            if (point) {
                ++frac;
            }
        } else if (c == '.' && !point) {
            point = true;
        } else {
            break;
        }
    }
    if (need_digit && !any) {
        return false;
    }
    long exp = 0;
    if (!parse_exponent(t, i, exp)) {
        return false;
    }
    out.exponent = exp - (point ? frac : implied);
    return true;
}

std::string to_c_number(const Decimal &d) {
    std::string s = d.negative ? "-" : "";
    s += d.digits.empty() ? "0" : d.digits;
    s += "e" + std::to_string(d.exponent);
    return s;
}

// Feldinhalt bis zum ersten Komma, Leerzeichen entfernt (BLANK='NULL').
std::string squeeze_field(std::string_view s) {
    std::string t;
    for (const char c : s) {
        if (c == ',') {
            break;
        }
        if (!is_blank(c)) {
            t.push_back(c);
        }
    }
    return t;
}

bool decode_f(std::string_view s, int d, Decimal &dec) {
    const std::string t = squeeze_field(s);
    return parse_decimal(t, d, false, dec);
}

enum class ListResult { Value, Null, Error };

// Erstes Element einer list-gesteuerten Eingabe.
ListResult decode_list(std::string_view s, Decimal &dec) {
    std::size_t i = 0;
    while (i < s.size() && is_blank(s[i])) {
        ++i;
    }
    if (i >= s.size()) {
        return ListResult::Error;  // Dateiende
    }
    if (s[i] == ',' || s[i] == '/') {
        return ListResult::Null;
    }
    std::string tok;
    while (i < s.size() && !is_blank(s[i]) && s[i] != ',' && s[i] != '/') {
        tok.push_back(s[i]);
        ++i;
    }
    const std::size_t star = tok.find('*');
    if (star != std::string::npos) {
        const std::string rep = tok.substr(0, star);
        if (rep.empty() || rep.find_first_not_of("0123456789") != std::string::npos ||
            std::strtol(rep.c_str(), nullptr, 10) <= 0) {
            return ListResult::Error;
        }
        tok = tok.substr(star + 1);
        if (tok.empty()) {
            return ListResult::Null;
        }
    }
    // Compaq-Laufzeit: logischer Wert fuer eine REAL-Variable (.TRUE. = -1).
    const std::size_t l = (tok[0] == '.') ? 1 : 0;
    if (l < tok.size() && (tok[l] == 'T' || tok[l] == 'F')) {
        dec.negative = (tok[l] == 'T');
        dec.digits = (tok[l] == 'T') ? "1" : "0";
        return ListResult::Value;
    }
    return parse_decimal(tok, 0, true, dec) ? ListResult::Value : ListResult::Error;
}

}  // namespace

// Compaq-Laufzeit: ein Wert, der den Wertebereich des Ziels ueberschreitet,
// ist ein Lesefehler (per Probe gegen die Original-DLL belegt: "1E999" fuer
// DOUBLE, "1E99" fuer REAL). Unterlauf liefert ohne Fehler 0.
bool read_f(std::string_view s, int d, double &out) {
    Decimal dec;
    if (!decode_f(s, d, dec)) {
        out = 0;  // Compaq-Laufzeit: Ziel bei Lesefehler auf null
        return false;
    }
    const double v = std::strtod(to_c_number(dec).c_str(), nullptr);
    if (std::isinf(v)) {
        out = 0;
        return false;
    }
    out = v;
    return true;
}

bool read_f(std::string_view s, int d, float &out) {
    Decimal dec;
    if (!decode_f(s, d, dec)) {
        out = 0;  // Compaq-Laufzeit: Ziel bei Lesefehler auf null
        return false;
    }
    const std::string num = to_c_number(dec);
    const double v = std::strtod(num.c_str(), nullptr);
    if (std::isinf(v) || std::fabs(v) > static_cast<double>(std::numeric_limits<float>::max())) {
        out = 0;
        return false;
    }
    out = std::strtof(num.c_str(), nullptr);
    return true;
}

bool read_i(std::string_view s, int &out) {
    const std::string t = squeeze_field(s);
    std::size_t i = 0;
    bool neg = false;
    if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
        neg = (t[i] == '-');
        ++i;
    }
    long long v = 0;
    for (; i < t.size(); ++i) {
        if (!is_digit(t[i])) {
            return false;
        }
        v = v * 10 + (t[i] - '0');
        if (v > 2147483647LL) {
            return false;
        }
    }
    out = static_cast<int>(neg ? -v : v);
    return true;
}

bool read_i(std::string_view s, std::int16_t &out) {
    int v = 0;
    if (!read_i(s, v) || v < -32768 || v > 32767) {
        return false;
    }
    out = static_cast<std::int16_t>(v);
    return true;
}

bool read_list(std::string_view s, double &out) {
    Decimal dec;
    switch (decode_list(s, dec)) {
        case ListResult::Value:
            out = std::strtod(to_c_number(dec).c_str(), nullptr);
            return true;
        case ListResult::Null:
            return true;
        case ListResult::Error:
            break;
    }
    return false;
}

bool read_list(std::string_view s, float &out) {
    Decimal dec;
    switch (decode_list(s, dec)) {
        case ListResult::Value:
            out = std::strtof(to_c_number(dec).c_str(), nullptr);
            return true;
        case ListResult::Null:
            return true;
        case ListResult::Error:
            break;
    }
    return false;
}

std::string write_i(long long v, int w, int m) {
    std::string digits = std::to_string(v < 0 ? -v : v);
    while (static_cast<int>(digits.size()) < m) {
        digits.insert(digits.begin(), '0');
    }
    if (v < 0) {
        digits.insert(digits.begin(), '-');
    }
    if (static_cast<int>(digits.size()) > w) {
        return std::string(static_cast<std::size_t>(w), '*');
    }
    return std::string(static_cast<std::size_t>(w) - digits.size(), ' ') + digits;
}

std::string write_f41(float v) {
    // Zehntel exakt bestimmen: float * 10 ist in double exakt; halbe Stelle
    // wird von null weg gerundet.
    const double tenths = std::round(static_cast<double>(v) * 10.0);
    const bool neg = std::signbit(v);
    const auto t = static_cast<long long>(std::fabs(tenths));
    std::string s = std::to_string(t / 10) + "." + std::to_string(t % 10);
    if (neg) {
        s.insert(s.begin(), '-');
    }
    if (s.size() > 4) {
        return "****";
    }
    return std::string(4 - s.size(), ' ') + s;
}

std::string fixed(std::string_view s, std::size_t n) {
    std::string r(s.substr(0, n));
    r.resize(n, ' ');
    return r;
}

std::string_view trim(std::string_view s) {
    const std::size_t e = s.find_last_not_of(' ');
    return (e == std::string_view::npos) ? std::string_view{} : s.substr(0, e + 1);
}

}  // namespace hcmms::fio
