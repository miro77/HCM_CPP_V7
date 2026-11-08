// ARC-013 — Kommandozeile: hcmms <fall-datei> [<testdaten-verzeichnis>]
// Schreibt je Fall eine Ergebniszeile (SPEC-004) auf die Standardausgabe.
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "cases.hpp"

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Aufruf: hcmms <fall-datei> [<testdaten-verzeichnis>]\n";
        return 2;
    }
    std::ifstream f(argv[1]);
    if (!f) {
        std::cerr << "Fall-Datei nicht lesbar: " << argv[1] << '\n';
        return 2;
    }
    std::stringstream text;
    text << f.rdbuf();
    try {
        const auto cases = hcmms::parse_cases(text.str(), argc == 3 ? argv[2] : "");
        for (const std::string &line : hcmms::run_cases(cases)) {
            std::cout << line << '\n';
        }
    } catch (const hcmms::CaseError &e) {
        std::cerr << "Fehler: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
