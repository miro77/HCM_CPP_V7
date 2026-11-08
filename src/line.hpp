// ARC-011 — Linien- und Grenzbereichsberechnung.
#pragma once

#include "geo.hpp"
#include <hcmms/state.hpp>

namespace hcmms {

// Line_calculation: sucht auf der Linie den Punkt groesster Feldstaerke und
// rechnet ihn als Punkt-zu-Punkt-Fall. Der gefundene Linienpunkt steht in
// s.LongRx/s.LatRx.
void line_calculation(State &s, const GeoPoint &tx);

}  // namespace hcmms
