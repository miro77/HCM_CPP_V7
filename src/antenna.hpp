// ARC-006 — Antennendiagramme und Antennenkorrektur.
#pragma once

#include <string>

namespace hcmms {

// Antenna: relativer Feldstaerkefaktor RHO des Diagramms `a_typ` (7 Zeichen)
// unter dem Winkel `angle` in Grad. Setzt `error` = 1038 bei ungueltigem Typ.
float antenna(const std::string &a_typ, float angle, int &error);

// Ctransf: Winkelabstand zur Hauptstrahlrichtung, horizontal und vertikal.
void ctransf(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda);

// Antenna_correction: Daempfung in dB fuer Richtung (azi, ele) bei
// Hauptstrahlrichtung (azi_m, ele_m).
void antenna_correction(double azi, double azi_m, double ele, double ele_m, float &hda, float &vda,
                        const std::string &h_cod, const std::string &v_cod, float &a, int &error);

}  // namespace hcmms
