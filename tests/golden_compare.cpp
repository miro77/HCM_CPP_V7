// Golden-Vergleich (ADR-0001, LH-QA-01): rechnet die Fall-Datei und vergleicht
// jede Ergebniszeile mit dem Golden-Datensatz der Referenz-DLL.
// Aufruf: golden_compare <faelle> <testdaten-verzeichnis> <golden>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cases.hpp"

namespace {

std::string read_file(const char *path) {
    std::ifstream f(path);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

std::vector<std::string> split(const std::string &s, char sep) {
    std::vector<std::string> parts;
    std::stringstream in(s);
    std::string p;
    while (std::getline(in, p, sep)) {
        parts.push_back(p);
    }
    return parts;
}

// Toleranz je Wert der Ergebniszeile (Reihenfolge nach SPEC-004).
double tolerance(std::size_t index) {
    switch (index) {
        case 0:   // HCM_error
        case 2:   // H_Datab_Tx
        case 3:   // H_Datab_Rx
        case 22:  // Delta_frequency (ganzzahlige Hz)
            return 0.0;
        case 1:   // Distance
        case 19:  // D_sea_calculated
        case 26:  // CBR_D
            return 1e-6;  // km
        default:
            return 0.01;  // dB, m, Grad
    }
}

// Vergleicht eine Zeile; liefert eine Beschreibung der Abweichung oder "".
std::string compare_line(const std::string &got, const std::string &want) {
    const std::vector<std::string> g = split(got, '|');
    const std::vector<std::string> w = split(want, '|');
    if (g.size() != w.size() || g.size() < 3) {
        return "Aufbau verschieden";
    }
    std::string diff;
    for (std::size_t k = 0; k < g.size(); ++k) {
        if (k == 2) {
            continue;
        }
        if (g[k] != w[k]) {
            diff += " [" + w[k] + " -> " + g[k] + "]";
        }
    }
    const std::vector<std::string> gv = split(g[2], ' ');
    const std::vector<std::string> wv = split(w[2], ' ');
    if (gv.size() != wv.size()) {
        return diff + " Werteanzahl verschieden";
    }
    for (std::size_t k = 0; k < gv.size(); ++k) {
        const double a = std::strtod(gv[k].c_str(), nullptr);
        const double b = std::strtod(wv[k].c_str(), nullptr);
        // a == b covers equal infinities (inf - inf is NaN); since V7.21.3 a
        // receive frequency of 0 is accepted and yields +/-INF results.
        if (!(a == b || std::fabs(a - b) <= tolerance(k))) {
            diff += " [Wert " + std::to_string(k) + ": " + wv[k] + " -> " + gv[k] + "]";
        }
    }
    return diff;
}

}  // namespace

int main(int argc, char **argv) {
    if (argc != 4) {
        std::fprintf(stderr, "Aufruf: golden_compare <faelle> <testdaten-verzeichnis> <golden>\n");
        return 2;
    }
    std::vector<std::string> got;
    try {
        got = hcmms::run_cases(hcmms::parse_cases(read_file(argv[1]), argv[2]));
    } catch (const hcmms::CaseError &e) {
        std::fprintf(stderr, "Fehler: %s\n", e.what());
        return 2;
    }
    std::vector<std::string> want;
    for (const std::string &l : split(read_file(argv[3]), '\n')) {
        if (!l.empty() && l != "\r") {
            want.push_back(l.back() == '\r' ? l.substr(0, l.size() - 1) : l);
        }
    }
    if (got.size() != want.size()) {
        std::fprintf(stderr, "Anzahl Faelle: %zu gerechnet, %zu im Golden\n", got.size(), want.size());
        return 1;
    }
    int failed = 0;
    for (std::size_t i = 0; i < got.size(); ++i) {
        const std::string d = compare_line(got[i], want[i]);
        if (!d.empty()) {
            std::fprintf(stderr, "ABWEICHUNG %s:%s\n", split(want[i], '|')[0].c_str(), d.c_str());
            ++failed;
        }
    }
    std::printf("%zu Faelle, %d Abweichungen\n", got.size(), failed);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
