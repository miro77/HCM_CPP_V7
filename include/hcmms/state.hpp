// ARC-012 — Zustand der Berechnung. Ein Feld je Variable der COMMON-Bloecke
// /NDATA/, /CDATA/, /Co_ord_cp/ des Originals und je lokaler Variable, deren
// Wert im Original einen Aufruf ueberdauert (statische Locals). Namen folgen
// dem Fortran-Original, damit die Nachbildung Zeile fuer Zeile vergleichbar
// bleibt. REAL ist float, DOUBLE PRECISION ist double (ADR-0002 im Repo).
#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace hcmms {

// Groesse der Profil-Felder im Original; laengere Profile laesst die
// Nachbildung die Felder vergroessern (ADR-0006).
inline constexpr std::size_t kProfileSize = 10002;
inline constexpr std::size_t kTileSize = 10201;
// V7.21.4: Linienpuffer 1200 -> 1400 Datensaetze.
inline constexpr std::size_t kCenterPoints = 1400;

struct State {
    // /NDATA/ — 8 Byte
    double Tx_frequency = 0.0;
    double Distance = 0.0;
    double PD = 0.0;
    double Dir_Tx_Rx = 0.0;
    double Dir_Rx_Tx = 0.0;
    double D_sea_calculated = 0.0;
    double V_angle_Tx_Rx = 0.0;
    double V_angle_Rx_Tx = 0.0;
    double Rx_frequency = 0.0;
    double Tx_Azimuth = 0.0;
    double Tx_Elevation = 0.0;
    double Rx_Azimuth = 0.0;
    double Rx_Elevation = 0.0;
    std::int64_t Delta_frequency = 0;
    // /NDATA/ — 4 Byte
    float Heff = 0.0F;
    float aLatM = 0.0F;
    float Dh = 0.0F;
    float Dh_corr = 0.0F;
    float Calculated_FS = 0.0F;
    float Perm_FS = 0.0F;
    float Prot_margin = 0.0F;
    float ERP_ref_Tx = 0.0F;
    float CBR_D = 0.0F;
    float Tx_TCA = 0.0F;
    float Rx_TCA = 0.0F;
    float Tx_TCA_corr = 0.0F;
    float Rx_TCA_corr = 0.0F;
    float Heff_Tx = 0.0F;
    float Heff_Rx = 0.0F;
    float Land_FS = 0.0F;
    float Sea_FS = 0.0F;
    float Tx_ant_corr = 0.0F;
    float Rx_ant_corr = 0.0F;
    float Tx_ant_type_corr = 0.0F;
    float Rx_ant_type_corr = 0.0F;
    float Perm_FS_from_table = 0.0F;
    float Corr_delta_f = 0.0F;
    float Power_to_Rx = 0.0F;
    float Free_space_FS = 0.0F;
    float Tx_serv_area = 0.0F;
    float Rx_serv_area = 0.0F;
    float MaxPow = 0.0F;
    float H_diff_angle_Rx_Tx = 0.0F;
    float V_diff_angle_Rx_Tx = 0.0F;
    float H_diff_angle_Tx_Rx = 0.0F;
    float V_diff_angle_Tx_Rx = 0.0F;
    int HCM_error = 0;
    int Time_percentage = 0;
    int C_mode = 0;
    int D_to_border = 0;
    int OLD_R = 0;
    int O_LOD = 0;
    int O_LAD = 0;
    int PN = 0;
    int RESH = 0;
    int H_AntTx = 0;
    int H_AntRx = 0;
    int Channel_sp_Rx = 0;
    int Channel_sp_Tx = 0;
    std::array<bool, 21> Info{};  // Index 1..20 wie im Original
    bool p2p = false;
    bool with_morpho = false;
    bool flp = false;
    // /NDATA/ — 2 Byte. Index 1-basiert wie im Original (Element 0 unbenutzt).
    std::vector<std::int16_t> T_Prof = std::vector<std::int16_t>(kProfileSize + 1, 0);
    std::vector<std::int16_t> M_Prof = std::vector<std::int16_t>(kProfileSize + 1, 0);
    std::int16_t H_Datab_Tx = 0;
    std::int16_t H_Datab_Rx = 0;
    std::int16_t H_Tx = 0;
    std::int16_t H_Rx = 0;
    std::vector<std::int16_t> H_I = std::vector<std::int16_t>(kTileSize + 1, 0);
    std::vector<std::int16_t> M_I = std::vector<std::int16_t>(kTileSize + 1, 0);

    // /CDATA/ — feste Laenge wie im Original.
    std::string Sea_temperature = std::string(1, ' ');
    std::string Type_of_Tx_ant = std::string(1, ' ');
    std::string Type_of_Rx_ant = std::string(1, ' ');
    std::string Chan_occup = std::string(1, ' ');
    std::string Land_from = std::string(3, ' ');
    std::string Land_to = std::string(3, ' ');
    std::string Max_CBR_D_input = std::string(3, ' ');
    std::string H_Tx_input = std::string(4, ' ');
    std::string H_Rx_input = std::string(4, ' ');
    std::string H_Tx_ant = std::string(4, ' ');
    std::string H_Rx_ant = std::string(4, ' ');
    std::string Rx_ant_gain = std::string(4, ' ');
    std::string Depol_loss = std::string(4, ' ');
    std::string Cor_fact_frequ_diff = std::string(4, ' ');
    std::string Azi_Tx_input = std::string(5, ' ');
    std::string Ele_Tx_input = std::string(5, ' ');
    std::string Azi_Rx_input = std::string(5, ' ');
    std::string Ele_Rx_input = std::string(5, ' ');
    std::string D_sea_input = std::string(5, ' ');
    std::string Rad_of_Tx_serv_area = std::string(5, ' ');
    std::string Rad_of_Rx_serv_area = std::string(5, ' ');
    std::string Perm_FS_input = std::string(5, ' ');
    std::string Version = std::string(6, ' ');
    std::string Max_power = std::string(6, ' ');
    std::string Ant_typ_H_Tx = std::string(7, ' ');
    std::string Ant_typ_V_Tx = std::string(7, ' ');
    std::string Ant_typ_H_Rx = std::string(7, ' ');
    std::string Ant_typ_V_Rx = std::string(7, ' ');
    std::string Desig_of_Tx_emis = std::string(9, ' ');
    std::string Desig_of_Rx_emis = std::string(9, ' ');
    std::string Tx_frequ = std::string(12, ' ');
    std::string Rx_frequ = std::string(12, ' ');
    std::string Coo_Tx = std::string(15, ' ');
    std::string Coo_Rx = std::string(15, ' ');
    std::string Coo_Tx_new = std::string(15, ' ');
    std::string Coo_Rx_new = std::string(15, ' ');
    std::string Topo_path = std::string(63, ' ');
    std::string Morpho_path = std::string(63, ' ');
    std::string Border_path = std::string(63, ' ');

    // /Co_ord_cp/ — Mittelpunkte der Grenzlinie, Index 1-basiert.
    std::vector<std::array<double, 2>> Co_cp = std::vector<std::array<double, 2>>(kCenterPoints + 1, {0.0, 0.0});
    int N_cp = 0;

    // Lokale Variablen des Originals, die mehrere Unterroutinen eines Aufrufs
    // gemeinsam benutzen. Ihr Wert ueberdauert keinen Aufruf: sie stehen zu
    // Beginn der besitzenden Routine auf 0 (ADR-0004).
    double LongRx = 0.0;  // HCM_MS_V7
    double LatRx = 0.0;   // HCM_MS_V7
    double cbr_Lo = 0.0;  // CBR_Coordinates
    double cbr_La = 0.0;  // CBR_Coordinates
    std::vector<float> HDZ = std::vector<float>(kProfileSize + 1, 0.0F);  // P_to_P_calculation

    // Offene Gelaende- und Morphologie-Datei (Units 5 und 6 im Original).
    std::ifstream topo_file;
    std::ifstream morpho_file;
    bool topo_open = false;
    bool morpho_open = false;
};

}  // namespace hcmms
