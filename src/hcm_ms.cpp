// ARC-002 — Ablaufsteuerung und Eingabepruefung (HCM_MS_V7), ARC-001 —
// Schnittstelle (HCMMS_V7_DLL).
#include <array>
#include <cmath>
#include <cstddef>
#include <string>

#include "fmath.hpp"
#include "fortran_io.hpp"
#include "geo.hpp"
#include <hcmms/hcmms.hpp>
#include "line.hpp"
#include "p2p.hpp"
#include "permissible.hpp"

namespace hcmms {

namespace {

// Frequenzband des Abkommens mit zulaessiger Feldstaerke, maximaler
// Reichweite und Bezugsleistung. Die Grenzen sind REAL-Literale im Vergleich
// mit einer DOUBLE-Groesse und gelten daher dezimal-exakt (ADR-0003).
struct Band {
    double lo;
    double hi;
    float perm_fs;
    float cbr_d;
    float erp;
    bool info18;
};

constexpr std::array<Band, 12> kBands = {{
    {29.7, 47.0, 0.0F, 100.0F, 3.0F, false},
    {68.0, 74.8, 6.0F, 100.0F, 9.0F, false},
    {75.2, 87.5, 6.0F, 100.0F, 9.0F, false},
    {146.0, 149.9, 12.0F, 80.0F, 12.0F, false},
    {150.05, 174.0, 12.0F, 80.0F, 12.0F, false},
    {380.0, 385.0, 18.0F, 50.0F, 14.0F, true},
    {390.0, 395.0, 18.0F, 50.0F, 14.0F, true},
    {406.1, 430.0, 20.0F, 50.0F, 16.0F, false},
    {440.0, 470.0, 20.0F, 50.0F, 16.0F, false},
    {862.0, 960.0, 26.0F, 30.0F, 13.0F, false},
    {1710.0, 1785.0, 35.0F, 15.0F, 13.0F, false},
    {1805.0, 1880.0, 35.0F, 15.0F, 13.0F, false},
}};

constexpr float kUndefined = -999.9F;

void set_non_strict(State &s, int time_percentage) {
    s.Time_percentage = time_percentage;
    s.CBR_D = kUndefined;
    s.ERP_ref_Tx = kUndefined;
    s.Perm_FS = kUndefined;
    s.Info[4] = true;
}

void set_agreement_band(State &s) {
    s.Perm_FS = kUndefined;
    s.CBR_D = kUndefined;
    s.ERP_ref_Tx = kUndefined;
    s.Info[4] = true;
    for (const Band &b : kBands) {
        if (s.Tx_frequency >= b.lo && s.Tx_frequency <= b.hi) {
            s.Perm_FS = b.perm_fs;
            s.CBR_D = b.cbr_d;
            s.ERP_ref_Tx = b.erp;
            s.Info[4] = false;
            if (b.info18) {
                s.Info[18] = true;
            }
            break;
        }
    }
    const bool tetra = s.Info[18] && s.Desig_of_Tx_emis.substr(0, 7) == "25K0G7W" &&
                       s.Desig_of_Rx_emis.substr(0, 7) == "25K0G7W";
    s.Time_percentage = (tetra || s.Chan_occup == "0") ? 10 : 1;
}

// Rechenmodus: Zeitprozent und Tabellenwerte. false bei unbekanntem Modus.
bool set_mode(State &s) {
    switch (s.C_mode) {
        case 12: set_non_strict(s, 1); return true;
        case 11: set_non_strict(s, 50); return true;
        case 10: set_non_strict(s, 10); return true;
        case 99:
        case 0:
        case -1: set_agreement_band(s); return true;
        case -9:
            set_non_strict(s, 10);
            s.H_AntRx = 3;
            return true;
        case -10: set_non_strict(s, 10); return true;
        case -11:
            set_non_strict(s, 50);
            s.H_AntRx = 3;
            return true;
        default: return false;
    }
}

// Frequenz mit Einheit M/K/G (12 Zeichen). Fehler: 1 = Wert, 2 = Einheit.
int parse_frequency(const std::string &f, double &mhz) {
    if (!fio::read_f(std::string_view(f).substr(0, 11), 5, mhz)) {
        return 1;
    }
    switch (f[11]) {
        case 'M': return 0;
        case 'K': mhz = mhz / 1.0e3; return 0;
        case 'G': mhz = mhz * 1.0e3; return 0;
        default: return 2;
    }
}

bool parse_tx(State &s, GeoPoint &tx) {
    const int e = parse_coordinates(s.Coo_Tx, tx);
    if (e != 0) {
        s.HCM_error = 1000 + e;
        return false;
    }
    if (!fio::read_i(s.H_Tx_ant, s.H_AntTx)) {
        s.HCM_error = 1009;
        return false;
    }
    const int fe = parse_frequency(s.Tx_frequ, s.Tx_frequency);
    if (fe != 0) {
        s.HCM_error = 1009 + fe;
        return false;
    }
    if (!fio::read_f(s.Rad_of_Tx_serv_area, 0, s.Tx_serv_area)) {
        s.HCM_error = 1012;
        return false;
    }
    if (s.Type_of_Tx_ant == "E") {
        s.Tx_ant_type_corr = 0.0F;
    } else if (s.Type_of_Tx_ant == "I") {
        s.Tx_ant_type_corr = 2.1F;
    } else {
        s.HCM_error = 1033;
        return false;
    }
    if (!fio::read_f(s.Max_power, 1, s.MaxPow)) {
        s.HCM_error = 1034;
        return false;
    }
    s.MaxPow = f32(R(s.MaxPow) - s.Tx_ant_type_corr);
    return true;
}

bool parse_rx(State &s) {
    if (s.C_mode < 0) {
        s.H_AntRx = 10;
        s.Rx_frequency = 0;
        s.Rx_serv_area = 0.0F;
        s.Desig_of_Rx_emis = fio::fixed("       ", 9);
        s.Cor_fact_frequ_diff = "    ";
        return true;
    }
    if (s.Coo_Tx == s.Coo_Rx) {
        s.HCM_error = 1000;
        return false;
    }
    GeoPoint rx{s.LongRx, s.LatRx};
    const int e = parse_coordinates(s.Coo_Rx, rx);
    s.LongRx = rx.lon;
    s.LatRx = rx.lat;
    if (e != 0) {
        s.HCM_error = 1013 + e;
        return false;
    }
    if (!fio::read_i(s.H_Rx_ant, s.H_AntRx)) {
        s.HCM_error = 1022;
        return false;
    }
    // V7.21.3: Rx_frequency = 0 ist kein Fehler 1023 mehr.
    const int fe = parse_frequency(s.Rx_frequ, s.Rx_frequency);
    if (fe != 0) {
        s.HCM_error = 1022 + fe;
        return false;
    }
    if (s.C_mode != 99) {
        if (!fio::read_f(s.Rad_of_Rx_serv_area, 0, s.Rx_serv_area)) {
            s.HCM_error = 1029;
            return false;
        }
    } else {
        s.Rx_serv_area = 0.0F;
    }
    return true;
}

// Eingegebene zulaessige Feldstaerke und maximale Reichweite.
bool parse_limits(State &s) {
    s.Perm_FS_from_table = s.Perm_FS;
    if (s.Perm_FS_input != "     ") {
        s.Info[5] = true;
        if (!fio::read_f(s.Perm_FS_input, 1, s.Perm_FS)) {
            s.HCM_error = 1026;
            return false;
        }
    } else if (s.Info[4]) {
        s.HCM_error = 1050;
        return false;
    }
    if (s.Max_CBR_D_input != "   ") {
        int imr = 0;
        if (!fio::read_i(s.Max_CBR_D_input, imr)) {
            s.HCM_error = 1027;
            return false;
        }
        s.CBR_D = static_cast<float>(imr);
        s.Info[6] = true;
    } else if (s.Info[4] && s.C_mode < 0 && s.D_to_border < 0) {
        s.HCM_error = 1050;
        return false;
    }
    return true;
}

void sea_temperature(State &s, const GeoPoint &tx) {
    if (s.Sea_temperature == "W" || s.Sea_temperature == "C") {
        return;
    }
    if (s.C_mode >= 0) {
        s.aLatM = f32(std::fabs(tx.lat + s.LatRx) / 2.0);
    } else {
        s.aLatM = f32(tx.lat);
    }
    s.Sea_temperature = "C";
    if (s.aLatM < 56.0F) {
        s.Sea_temperature = "I";
    }
    if (s.aLatM <= 35.0F) {
        s.Sea_temperature = "W";
    }
}

void blank_country_code(std::string &c) {
    if (c[2] == ' ') {
        c[2] = '_';
    }
    if (c[1] == ' ') {
        c[1] = '_';
    }
}

}  // namespace

void hcm_ms_v7(State &s) {
    s.Version = "7.21  ";
    s.HCM_error = 0;
    s.Info.fill(false);
    s.O_LOD = 9999;
    s.O_LAD = 9999;
    s.OLD_R = -1;
    s.Coo_Tx_new = s.Coo_Tx;
    // V7.21.3: Coo_Rx_new wird fuer alle Modi (auch Linie) vorbelegt.
    s.Coo_Rx_new = s.Coo_Rx;
    s.LongRx = 0.0;
    s.LatRx = 0.0;
    blank_country_code(s.Land_to);
    blank_country_code(s.Land_from);
    GeoPoint tx;
    if (!parse_tx(s, tx) || !parse_rx(s)) {
        return;
    }
    if (s.PD < 3.0e-2) {
        s.PD = 1.0e-1;
    }
    if (!set_mode(s)) {
        s.HCM_error = 1025;
        return;
    }
    if (!parse_limits(s)) {
        return;
    }
    sea_temperature(s, tx);
    s.p2p = (s.C_mode >= 0 && s.C_mode != 99);
    if (fio::trim(s.D_sea_input).empty()) {
        s.with_morpho = true;
    } else {
        s.with_morpho = false;
        if (!fio::read_f(s.D_sea_input, 1, s.D_sea_calculated)) {
            s.HCM_error = 1035;
            return;
        }
    }
    if (s.C_mode < 0) {
        line_calculation(s, tx);
    } else {
        p_to_p_calculation(s, tx, GeoPoint{s.LongRx, s.LatRx});
    }
    permissible_fs_calculation(s);
    s.Prot_margin = f32(R(s.Perm_FS) - s.Calculated_FS);
}

namespace {

// Feld (Startposition 1-basiert, Laenge) aus I_str.
std::string field(const std::string &i_str, std::size_t pos, std::size_t len) { return i_str.substr(pos - 1, len); }

void read_inputs(State &s, const std::string &i) {
    s.Coo_Tx = field(i, 1, 15);
    s.Coo_Rx = field(i, 16, 15);
    s.H_Tx_input = field(i, 31, 4);
    s.H_Rx_input = field(i, 35, 4);
    s.Ant_typ_H_Tx = field(i, 39, 7);
    s.Ant_typ_V_Tx = field(i, 46, 7);
    s.Azi_Tx_input = field(i, 53, 5);
    s.Ele_Tx_input = field(i, 58, 5);
    s.H_Tx_ant = field(i, 63, 4);
    s.H_Rx_ant = field(i, 67, 4);
    s.Type_of_Tx_ant = field(i, 71, 1);
    s.Max_power = field(i, 72, 6);
    s.Tx_frequ = field(i, 78, 12);
    s.Chan_occup = field(i, 90, 1);
    s.Sea_temperature = field(i, 91, 1);
    s.Rad_of_Tx_serv_area = field(i, 92, 5);
    s.Rad_of_Rx_serv_area = field(i, 97, 5);
    s.D_sea_input = field(i, 102, 5);
    s.Rx_frequ = field(i, 107, 12);
    s.Desig_of_Rx_emis = field(i, 119, 9);
    s.Desig_of_Tx_emis = field(i, 128, 9);
    s.Ant_typ_H_Rx = field(i, 137, 7);
    s.Ant_typ_V_Rx = field(i, 144, 7);
    s.Azi_Rx_input = field(i, 151, 5);
    s.Ele_Rx_input = field(i, 156, 5);
    s.Type_of_Rx_ant = field(i, 161, 1);
    s.Rx_ant_gain = field(i, 162, 4);
    s.Depol_loss = field(i, 166, 4);
    s.Perm_FS_input = field(i, 170, 5);
    s.Cor_fact_frequ_diff = field(i, 175, 4);
    s.Land_to = field(i, 179, 3);
    s.Land_from = field(i, 182, 3);
    s.Max_CBR_D_input = field(i, 185, 3);
    s.Topo_path = field(i, 188, 63);
    s.Border_path = field(i, 251, 63);
    s.Morpho_path = field(i, 314, 63);
}

void write_outputs(const State &s, std::string &i, Outputs &o) {
    i.replace(376, 6, s.Version);
    for (std::size_t k = 1; k <= 20; ++k) {
        i[381 + k] = s.Info[k] ? 'T' : 'F';
    }
    i.replace(402, 15, s.Coo_Tx_new);
    i.replace(417, 15, s.Coo_Rx_new);
    i.replace(165, 4, s.Depol_loss);
    i.replace(90, 1, s.Sea_temperature);
    o.Distance = s.Distance;
    o.H_Datab_Tx = s.H_Datab_Tx;
    o.H_Datab_Rx = s.H_Datab_Rx;
    o.HCM_error = s.HCM_error;
    o.Heff = s.Heff;
    o.Dh = s.Dh;
    o.Dh_corr = s.Dh_corr;
    o.Power_to_Rx = s.Power_to_Rx;
    o.Free_space_FS = s.Free_space_FS;
    o.Land_FS = s.Land_FS;
    o.Sea_FS = s.Sea_FS;
    o.Tx_ant_corr = s.Tx_ant_corr;
    o.Tx_ant_type_corr = s.Tx_ant_type_corr;
    o.Dir_Tx_Rx = s.Dir_Tx_Rx;
    o.V_angle_Tx_Rx = s.V_angle_Tx_Rx;
    o.Tx_TCA = s.Tx_TCA;
    o.Rx_TCA = s.Rx_TCA;
    o.Tx_TCA_corr = s.Tx_TCA_corr;
    o.Rx_TCA_corr = s.Rx_TCA_corr;
    o.D_sea_calculated = s.D_sea_calculated;
    o.Rx_ant_corr = s.Rx_ant_corr;
    o.Rx_ant_type_corr = s.Rx_ant_type_corr;
    o.Delta_frequency = static_cast<double>(s.Delta_frequency);
    o.Corr_delta_f = s.Corr_delta_f;
    o.Calculated_FS = s.Calculated_FS;
    o.Perm_FS = s.Perm_FS;
    o.CBR_D = s.CBR_D;
    o.ERP_ref_Tx = s.ERP_ref_Tx;
    o.Prot_margin = s.Prot_margin;
}

}  // namespace

void hcmms_v7(State &s, int c_mode, int bor_dis, double pd, std::string &i_str, Outputs &out) {
    if (i_str.size() < kIStrMin) {
        out.HCM_error = 3000;
        return;
    }
    for (char &c : i_str) {
        const auto u = static_cast<unsigned char>(c);
        if (u >= 97 && u <= 122) {
            c = static_cast<char>(u - 32);
        }
        if (u == 0) {
            c = ' ';
        }
    }
    read_inputs(s, i_str);
    s.C_mode = c_mode;
    s.D_to_border = bor_dis;
    s.PD = pd;
    hcm_ms_v7(s);
    write_outputs(s, i_str, out);
}

}  // namespace hcmms

extern "C" void HCMMS_V7_DLL(const int *C_mode, const int *bor_dis, const double *PD, double *Distance,
                             std::int16_t *H_Datab_Tx, std::int16_t *H_Datab_Rx, int *HCM_error, float *Heff,
                             float *Dh, float *Dh_corr, float *Power_to_Rx, float *Free_space_FS, float *Land_FS,
                             float *Sea_FS, float *Tx_ant_corr, float *Tx_ant_type_corr, double *Dir_Tx_Rx,
                             double *V_angle_Tx_Rx, float *Tx_TCA, float *Rx_TCA, float *Tx_TCA_corr,
                             float *Rx_TCA_corr, double *D_sea_calculated, float *Rx_ant_corr,
                             float *Rx_ant_type_corr, double *Delta_frequency, float *Corr_delta_f,
                             float *Calculated_FS, float *Perm_FS, float *CBR_D, float *ERP_ref_Tx,
                             float *Prot_margin, char *I_str, std::size_t I_str_len) {
    static hcmms::State state;
    std::string s(I_str, I_str_len);
    hcmms::Outputs o;
    hcmms::hcmms_v7(state, *C_mode, *bor_dis, *PD, s, o);
    *HCM_error = o.HCM_error;
    if (I_str_len < hcmms::kIStrMin) {
        return;
    }
    s.copy(I_str, I_str_len);
    *Distance = o.Distance;
    *H_Datab_Tx = o.H_Datab_Tx;
    *H_Datab_Rx = o.H_Datab_Rx;
    *Heff = o.Heff;
    *Dh = o.Dh;
    *Dh_corr = o.Dh_corr;
    *Power_to_Rx = o.Power_to_Rx;
    *Free_space_FS = o.Free_space_FS;
    *Land_FS = o.Land_FS;
    *Sea_FS = o.Sea_FS;
    *Tx_ant_corr = o.Tx_ant_corr;
    *Tx_ant_type_corr = o.Tx_ant_type_corr;
    *Dir_Tx_Rx = o.Dir_Tx_Rx;
    *V_angle_Tx_Rx = o.V_angle_Tx_Rx;
    *Tx_TCA = o.Tx_TCA;
    *Rx_TCA = o.Rx_TCA;
    *Tx_TCA_corr = o.Tx_TCA_corr;
    *Rx_TCA_corr = o.Rx_TCA_corr;
    *D_sea_calculated = o.D_sea_calculated;
    *Rx_ant_corr = o.Rx_ant_corr;
    *Rx_ant_type_corr = o.Rx_ant_type_corr;
    *Delta_frequency = o.Delta_frequency;
    *Corr_delta_f = o.Corr_delta_f;
    *Calculated_FS = o.Calculated_FS;
    *Perm_FS = o.Perm_FS;
    *CBR_D = o.CBR_D;
    *ERP_ref_Tx = o.ERP_ref_Tx;
    *Prot_margin = o.Prot_margin;
}
