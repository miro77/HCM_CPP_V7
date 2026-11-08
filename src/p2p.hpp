// ARC-008 — Punkt-zu-Punkt-Berechnung und ihre Korrekturen.
#pragma once

#include "geo.hpp"
#include <hcmms/state.hpp>

namespace hcmms {

// TCA_correction_calculation: Korrektur fuer den Gelaendefreiwinkel.
float tca_correction(const State &s, float tca, double frequency);

// Dh_calculation: Gelaendeunebenheit aus T_Prof (setzt Dh).
void dh_calculation(State &s);

// Dh_correction: Korrektur fuer die Gelaendeunebenheit.
float dh_correction(float dh, double distance, double frequency);

// P_to_P_calculation.
void p_to_p_calculation(State &s, const GeoPoint &tx, const GeoPoint &rx);

}  // namespace hcmms
