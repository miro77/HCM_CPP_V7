// ARC-003 — Lese- und Schreibregeln der Fortran-Laufzeit, soweit das Original
// sie auf Teilstrings anwendet (spezifikation.md §1, LH-FA-02.a).
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace hcmms::fio {

// READ (s, '(Fw.d)'): w = s.size(). Liefert false bei ungueltigem Feld; dann
// steht `out` auf 0 (Verhalten der Compaq-Laufzeit).
bool read_f(std::string_view s, int d, double &out);
bool read_f(std::string_view s, int d, float &out);

// READ (s, '(Iw)').
bool read_i(std::string_view s, int &out);
bool read_i(std::string_view s, std::int16_t &out);

// READ (s, *) — list-gesteuert, ein Wert. Liefert false bei Fehler oder
// Dateiende (Feld ohne Wert). Ein Nullwert (fuehrendes Komma, Schraegstrich)
// laesst `out` unveraendert und liefert true.
bool read_list(std::string_view s, double &out);
bool read_list(std::string_view s, float &out);

// WRITE (s, '(Iw.m)') — w Zeichen, mindestens m Ziffern; '*' bei Ueberlauf.
std::string write_i(long long v, int w, int m);

// WRITE (s, '(F4.1)').
std::string write_f41(float v);

// Fortran-Zuweisung an CHARACTER*n: auffuellen mit Leerzeichen oder abschneiden.
std::string fixed(std::string_view s, std::size_t n);

// Fortran TRIM: nachfolgende Leerzeichen entfernen.
std::string_view trim(std::string_view s);

}  // namespace hcmms::fio
