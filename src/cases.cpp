#include "cases.hpp"

#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <map>
#include <sstream>
#include <system_error>

#include <hcmms/state.hpp>

namespace hcmms {

namespace {

struct Slot {
    std::size_t pos;  // 1-basiert
    std::size_t len;
};

const std::map<std::string, Slot> &layout() {
    static const std::map<std::string, Slot> m = {
        {"Coo_Tx", {1, 15}},    {"Coo_Rx", {16, 15}},     {"H_Tx", {31, 4}},       {"H_Rx", {35, 4}},
        {"AntH_Tx", {39, 7}},   {"AntV_Tx", {46, 7}},     {"Azi_Tx", {53, 5}},     {"Ele_Tx", {58, 5}},
        {"Hant_Tx", {63, 4}},   {"Hant_Rx", {67, 4}},     {"AntType_Tx", {71, 1}}, {"Power", {72, 6}},
        {"Tx_f", {78, 12}},     {"ChanOcc", {90, 1}},     {"SeaT", {91, 1}},       {"Rad_Tx", {92, 5}},
        {"Rad_Rx", {97, 5}},    {"Dsea", {102, 5}},       {"Rx_f", {107, 12}},     {"Emis_Rx", {119, 9}},
        {"Emis_Tx", {128, 9}},  {"AntH_Rx", {137, 7}},    {"AntV_Rx", {144, 7}},   {"Azi_Rx", {151, 5}},
        {"Ele_Rx", {156, 5}},   {"AntType_Rx", {161, 1}}, {"Gain_Rx", {162, 4}},   {"Depol", {166, 4}},
        {"PermFS", {170, 5}},   {"CorDf", {175, 4}},      {"Land_to", {179, 3}},   {"Land_from", {182, 3}},
        {"MaxCBR", {185, 3}},   {"Topo", {188, 63}},      {"Border", {251, 63}},   {"Morpho", {314, 63}},
    };
    return m;
}

std::string strip(const std::string &s) {
    const std::size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) {
        return "";
    }
    const std::size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

std::vector<std::string> split(const std::string &s, char sep) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    for (;;) {
        const std::size_t p = s.find(sep, start);
        parts.push_back(s.substr(start, p == std::string::npos ? std::string::npos : p - start));
        if (p == std::string::npos) {
            return parts;
        }
        start = p + 1;
    }
}

std::string replace_all(std::string s, const std::string &from, const std::string &to) {
    for (std::size_t p = s.find(from); p != std::string::npos; p = s.find(from, p + to.size())) {
        s.replace(p, from.size(), to);
    }
    return s;
}

template <typename T>
T parse_number(const std::string &text, const std::string &what) {
    const std::string t = strip(text);
    T v{};
    const auto r = std::from_chars(t.data(), t.data() + t.size(), v);
    if (r.ec != std::errc() || r.ptr != t.data() + t.size()) {
        throw CaseError("ungueltige Zahl fuer " + what + ": '" + t + "'");
    }
    return v;
}

// Kuerzeste Darstellung nach .NET-Format "R" (Golden-Skript): erst 7 (float)
// bzw. 15 (double) signifikante Stellen, sonst 9 bzw. 17.
std::string g_format(double v, int digits) {
    std::array<char, 64> buf{};
    std::snprintf(buf.data(), buf.size(), "%.*G", digits, v);
    return std::string(buf.data());
}

std::string net_r(double v, int short_digits, int long_digits, bool single) {
    if (v == 0.0) {
        return "0";
    }
    const std::string s = g_format(v, short_digits);
    const double back = std::strtod(s.c_str(), nullptr);
    const bool same = single ? (static_cast<float>(back) == static_cast<float>(v)) : (back == v);
    return same ? s : g_format(v, long_digits);
}

std::string shortest(float v) { return net_r(static_cast<double>(v), 7, 9, true); }
std::string shortest(double v) { return net_r(v, 15, 17, false); }

}  // namespace

std::vector<Case> parse_cases(const std::string &text, const std::string &fixture_dir) {
    std::vector<Case> cases;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::string t = strip(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        const std::vector<std::string> p = split(line, '|');
        if (p.size() < 4) {
            throw CaseError("zu wenige Spalten: " + t);
        }
        Case c;
        c.name = strip(p[0]);
        c.c_mode = parse_number<int>(p[1], "C_mode");
        c.bor_dis = parse_number<int>(p[2], "bor_dis");
        c.pd = parse_number<double>(p[3], "PD");
        c.i_str.assign(600, ' ');
        const std::string fields = (p.size() > 4) ? p[4] : "";
        for (const std::string &kv : split(fields, ';')) {
            const std::size_t eq = kv.find('=');
            if (eq == std::string::npos) {
                continue;
            }
            const std::string key = strip(kv.substr(0, eq));
            const auto it = layout().find(key);
            if (it == layout().end()) {
                throw CaseError("unbekanntes Feld '" + key + "' in Fall " + c.name);
            }
            std::string v = replace_all(kv.substr(eq + 1), "${FX}", fixture_dir);
            v.resize(it->second.len, ' ');
            c.i_str.replace(it->second.pos - 1, it->second.len, v);
        }
        cases.push_back(c);
    }
    return cases;
}

std::string format_result(const Case &c, const std::string &i_str, const Outputs &o) {
    std::ostringstream s;
    s << c.name << "|err=" << o.HCM_error << "|" << o.HCM_error << ' ' << shortest(o.Distance) << ' '
      << o.H_Datab_Tx << ' ' << o.H_Datab_Rx << ' ' << shortest(o.Heff) << ' ' << shortest(o.Dh) << ' '
      << shortest(o.Dh_corr) << ' ' << shortest(o.Power_to_Rx) << ' ' << shortest(o.Free_space_FS) << ' '
      << shortest(o.Land_FS) << ' ' << shortest(o.Sea_FS) << ' ' << shortest(o.Tx_ant_corr) << ' '
      << shortest(o.Tx_ant_type_corr) << ' ' << shortest(o.Dir_Tx_Rx) << ' ' << shortest(o.V_angle_Tx_Rx) << ' '
      << shortest(o.Tx_TCA) << ' ' << shortest(o.Rx_TCA) << ' ' << shortest(o.Tx_TCA_corr) << ' '
      << shortest(o.Rx_TCA_corr) << ' ' << shortest(o.D_sea_calculated) << ' ' << shortest(o.Rx_ant_corr) << ' '
      << shortest(o.Rx_ant_type_corr) << ' ' << shortest(o.Delta_frequency) << ' ' << shortest(o.Corr_delta_f)
      << ' ' << shortest(o.Calculated_FS) << ' ' << shortest(o.Perm_FS) << ' ' << shortest(o.CBR_D) << ' '
      << shortest(o.ERP_ref_Tx) << ' ' << shortest(o.Prot_margin);
    s << "|ver='" << i_str.substr(376, 6) << "'|txnew='" << i_str.substr(402, 15) << "'|rxnew='"
      << i_str.substr(417, 15) << "'|info='" << i_str.substr(382, 20) << "'|depol='" << i_str.substr(165, 4)
      << "'|seat='" << i_str.substr(90, 1) << "'";
    return s.str();
}

std::vector<std::string> run_cases(const std::vector<Case> &cases) {
    State state;
    Outputs out;
    std::vector<std::string> lines;
    for (const Case &c : cases) {
        std::string i_str = c.i_str;
        hcmms_v7(state, c.c_mode, c.bor_dis, c.pd, i_str, out);
        lines.push_back(format_result(c, i_str, out));
    }
    return lines;
}

}  // namespace hcmms
