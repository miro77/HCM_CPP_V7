// ARC-013 — Fall-Datei (SPEC-003) und Ergebniszeile (SPEC-004).
#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <hcmms/hcmms.hpp>

namespace hcmms {

struct Case {
    std::string name;
    int c_mode = 0;
    int bor_dis = 0;
    double pd = 0.0;
    std::string i_str;  // 600 Zeichen
};

// Fehler in einer Fallzeile.
class CaseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Liest alle Faelle; ${FX} in Feldwerten wird durch `fixture_dir` ersetzt.
std::vector<Case> parse_cases(const std::string &text, const std::string &fixture_dir);

// Ergebniszeile nach dem Aufruf.
std::string format_result(const Case &c, const std::string &i_str, const Outputs &o);

// Rechnet alle Faelle nacheinander auf einem Zustand.
std::vector<std::string> run_cases(const std::vector<Case> &cases);

}  // namespace hcmms
