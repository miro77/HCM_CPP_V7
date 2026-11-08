// Unit-Tests der Fortran-Lese-/Schreibregeln (LH-FA-02, spezifikation.md §1).
#include <cstdio>
#include <cstdlib>
#include <string>

#include "fortran_io.hpp"
#include "geo.hpp"

namespace {

int failures = 0;

void check(bool ok, const std::string &what) {
    if (!ok) {
        std::fprintf(stderr, "FEHLER: %s\n", what.c_str());
        ++failures;
    }
}

void test_read_f() {
    using hcmms::fio::read_f;
    double v = -1.0;
    // Happy Path
    check(read_f("12.5", 1, v) && v == 12.5, "F4.1 mit Punkt");
    // Boundary: impliziter Dezimalpunkt, Leerzeichen, leeres Feld
    check(read_f("  123", 1, v) && v == 12.3, "F5.1 impliziter Punkt");
    check(read_f("1 2", 0, v) && v == 12.0, "Leerzeichen im Feld");
    check(read_f("     ", 1, v) && v == 0.0, "leeres Feld ergibt 0");
    check(read_f("-0450", 1, v) && v == -45.0, "Vorzeichen");
    check(read_f("1.5E2", 1, v) && v == 150.0, "Exponent");
    // Negative
    v = 7.0;
    check(!read_f("1X", 0, v) && v == 0.0, "ungueltiges Zeichen setzt null");
    check(!read_f("1.2.3", 1, v), "zwei Punkte");
}

void test_read_i() {
    using hcmms::fio::read_i;
    int v = -1;
    check(read_i("0030", v) && v == 30, "I4");
    check(read_i("    ", v) && v == 0, "I4 leer");
    check(!read_i("12.", v), "I4 mit Punkt");
}

void test_read_list() {
    using hcmms::fio::read_list;
    float v = 3.0F;
    check(read_list("-9.9", v) && v == -9.9F, "Liste: Wert");
    check(read_list(" 25 ", v) && v == 25.0F, "Liste: Leerzeichen");
    v = 3.0F;
    check(read_list(",   ", v) && v == 3.0F, "Liste: Nullwert");
    check(!read_list("    ", v), "Liste: Dateiende");
    check(!read_list("ABCD", v), "Liste: kein Wert");
    check(read_list("T   ", v) && v == -1.0F, "Liste: logisch wahr");
    check(read_list(".F  ", v) && v == 0.0F, "Liste: logisch falsch");
}

void test_write() {
    using hcmms::fio::write_f41;
    using hcmms::fio::write_i;
    check(write_i(8, 3, 3) == "008", "I3.3");
    check(write_i(1000, 3, 3) == "***", "I3.3 Ueberlauf");
    check(write_f41(0.0F) == " 0.0", "F4.1 null");
    check(write_f41(12.25F) == "12.3", "F4.1 halbe Stelle");
    check(write_f41(-9.9F) == "-9.9", "F4.1 negativ");
    check(write_f41(-10.0F) == "****", "F4.1 Ueberlauf");
}

void test_coordinates() {
    hcmms::GeoPoint p;
    check(hcmms::parse_coordinates("008E300048N3000", p) == 0, "Koordinaten gueltig");
    check(hcmms::coo_conv(p) == "008E300048N3000", "CooConv rueckwaerts");
    check(hcmms::parse_coordinates("008X300048N3000", p) == 4, "Koordinaten E/W");
    check(hcmms::parse_coordinates("008E300048Q3000", p) == 8, "Koordinaten N/S");
    const double d = hcmms::calc_distance({8.0, 48.0}, {9.0, 48.0});
    check(d > 74.0 && d < 75.0, "Entfernung 1 Grad Laenge bei 48N");
    check(hcmms::calc_direction({8.0, 48.0}, {8.0, 49.0}) == 0.0, "Richtung Nord");
}

}  // namespace

int main() {
    test_read_f();
    test_read_i();
    test_read_list();
    test_write();
    test_coordinates();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
