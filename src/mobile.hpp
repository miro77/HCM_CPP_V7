// ARC-010 — Mobile Stationen mit Versorgungsbereich (Grenzdateien <Land>.all).
#pragma once

#include <string>

#include "geo.hpp"
#include <hcmms/state.hpp>

namespace hcmms {

// Position_of_mobile: massgebliche Positionen von Sender und Empfaenger.
void position_of_mobile(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &ntx, GeoPoint &nrx);

// TestCut: Zahl der Schnitte der Strecke p -> (Richtung dir, Laenge d) mit der
// Landesgrenze `country`. Setzt `error` auf 0, bei fehlender Datei auf 1.
int test_cut(const State &s, double dir, const GeoPoint &p, float d, int &error, const std::string &country);

// Calc_Tx_pos: Senderposition im Versorgungsbereich. `ntx` ist Ein-/Ausgabe.
void calc_tx_pos(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &ntx);

// Calc_Rx_pos: Empfaengerposition im Versorgungsbereich. `nrx` ist Ein-/Ausgabe.
void calc_rx_pos(State &s, const GeoPoint &tx, const GeoPoint &rx, GeoPoint &nrx);

}  // namespace hcmms
