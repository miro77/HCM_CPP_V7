// ARC-007 — Feldstaerke aus den Kurven (1 kW, Land und See).
#pragma once

#include <hcmms/state.hpp>

namespace hcmms {

// Get_figure_FS_value: Tabellenwert fuer Frequenz, Hoehe und Entfernung.
void get_figure_fs_value(State &s, int frequency, float heff, int distance, float &land, float &sea);

// Get_FS_from_figures: interpolierte Feldstaerke fuer Distance, Heff, Tx_frequency.
void get_fs_from_figures(State &s, float &land_1kw, float &sea_1kw);

}  // namespace hcmms
