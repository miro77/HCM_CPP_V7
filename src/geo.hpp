// ARC-004 — Geometrie auf der Kugel und Koordinaten-Text.
#pragma once

#include <string>
#include <string_view>

namespace hcmms {

struct GeoPoint {
    double lon = 0.0;  // Grad, Ost positiv
    double lat = 0.0;  // Grad, Nord positiv
};

// Calc_distance: Grosskreis-Entfernung in km.
double calc_distance(const GeoPoint &a, const GeoPoint &b);

// Calc_direction: Richtung von a nach b in Grad, 0..360.
double calc_direction(const GeoPoint &a, const GeoPoint &b);

// CooConv: Koordinaten-Text DDDEMMSSDDNMMSS.
std::string coo_conv(const GeoPoint &p);

// New_coordinates: Punkt in Entfernung d (km) und Richtung dir von p.
GeoPoint new_coordinates(const GeoPoint &p, double dir, float d);

// Liest einen Koordinaten-Text wie HCM_MS_V7. Liefert 0 oder die Nummer
// (1..8) des fehlerhaften Teilfelds; `out` traegt die bis dahin gelesenen Werte.
int parse_coordinates(std::string_view coo, GeoPoint &out);

}  // namespace hcmms
