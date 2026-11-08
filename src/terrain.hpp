// ARC-005 — Gelaende- und Morphologie-Zugriff, Hoehenprofil.
#pragma once

#include <cstdint>

#include "geo.hpp"
#include <hcmms/state.hpp>

namespace hcmms {

// Point_info: Hoehe und Morphologie-Wert am Punkt p (SPEC-001).
void point_info(State &s, const GeoPoint &p, std::int16_t &height, std::int16_t &m_type);

// PROFILE: Hoehenprofil von a nach b in T_Prof/M_Prof, setzt PN.
void profile(State &s, const GeoPoint &a, const GeoPoint &b);

}  // namespace hcmms
