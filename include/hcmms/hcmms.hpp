// ARC-001 — Schnittstelle der Bibliothek (LH-FA-01).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <hcmms/state.hpp>

namespace hcmms {

// Ausgaben von HCMMS_V7_DLL in der Reihenfolge der Parameterliste.
struct Outputs {
    double Distance = 0.0;
    std::int16_t H_Datab_Tx = 0;
    std::int16_t H_Datab_Rx = 0;
    int HCM_error = 0;
    float Heff = 0.0F;
    float Dh = 0.0F;
    float Dh_corr = 0.0F;
    float Power_to_Rx = 0.0F;
    float Free_space_FS = 0.0F;
    float Land_FS = 0.0F;
    float Sea_FS = 0.0F;
    float Tx_ant_corr = 0.0F;
    float Tx_ant_type_corr = 0.0F;
    double Dir_Tx_Rx = 0.0;
    double V_angle_Tx_Rx = 0.0;
    float Tx_TCA = 0.0F;
    float Rx_TCA = 0.0F;
    float Tx_TCA_corr = 0.0F;
    float Rx_TCA_corr = 0.0F;
    double D_sea_calculated = 0.0;
    float Rx_ant_corr = 0.0F;
    float Rx_ant_type_corr = 0.0F;
    double Delta_frequency = 0.0;
    float Corr_delta_f = 0.0F;
    float Calculated_FS = 0.0F;
    float Perm_FS = 0.0F;
    float CBR_D = 0.0F;
    float ERP_ref_Tx = 0.0F;
    float Prot_margin = 0.0F;
};

// Laenge des Ein-/Ausgabe-Strings ohne Debug-Pfad.
inline constexpr std::size_t kIStrMin = 432;

// HCMMS_V7_DLL auf einem eigenen Zustand. `i_str` wird wie im Original in
// Grossbuchstaben gewandelt und traegt danach die Text-Ausgaben. Ist i_str
// kuerzer als 432 Zeichen, wird nur out.HCM_error (3000) gesetzt.
void hcmms_v7(State &s, int c_mode, int bor_dis, double pd, std::string &i_str, Outputs &out);

// HCM_MS_V7: Berechnung auf den bereits gesetzten Eingabefeldern von s.
void hcm_ms_v7(State &s);

}  // namespace hcmms

// C-Schnittstelle mit der Parameterliste des Originals; die Laenge von I_str
// folgt als letzter Parameter. Alle Aufrufe teilen einen prozessweiten Zustand.
extern "C" void HCMMS_V7_DLL(const int *C_mode, const int *bor_dis, const double *PD, double *Distance,
                             std::int16_t *H_Datab_Tx, std::int16_t *H_Datab_Rx, int *HCM_error, float *Heff,
                             float *Dh, float *Dh_corr, float *Power_to_Rx, float *Free_space_FS, float *Land_FS,
                             float *Sea_FS, float *Tx_ant_corr, float *Tx_ant_type_corr, double *Dir_Tx_Rx,
                             double *V_angle_Tx_Rx, float *Tx_TCA, float *Rx_TCA, float *Tx_TCA_corr,
                             float *Rx_TCA_corr, double *D_sea_calculated, float *Rx_ant_corr,
                             float *Rx_ant_type_corr, double *Delta_frequency, float *Corr_delta_f,
                             float *Calculated_FS, float *Perm_FS, float *CBR_D, float *ERP_ref_Tx,
                             float *Prot_margin, char *I_str, std::size_t I_str_len);
