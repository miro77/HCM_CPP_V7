// Tests der C-Schnittstelle (LH-FA-01) und des definierten Verhaltens bei
// Nullwerten im Gewinnfeld (ADR-0004).
// Aufruf: test_interface <testdaten-verzeichnis>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "cases.hpp"
#include <hcmms/hcmms.hpp>
#include <hcmms/state.hpp>

namespace {

int failures = 0;

void check(bool ok, const std::string &what) {
    if (!ok) {
        std::fprintf(stderr, "FEHLER: %s\n", what.c_str());
        ++failures;
    }
}

const char *const kBase =
    "H_Tx=0400;H_Rx=0300;AntH_Tx=000ND00;AntV_Tx=000ND00;Azi_Tx=000.0;Ele_Tx=000.0;Hant_Tx=0030;"
    "Hant_Rx=0010;AntType_Tx=E;Power=0010.0;Tx_f=00450.00000M;ChanOcc=1;Rx_f=00450.01250M;"
    "Emis_Rx=12K5F3E;Emis_Tx=12K5F3E;AntH_Rx=000ND00;AntV_Rx=000ND00;Azi_Rx=000.0;Ele_Rx=000.0;"
    "AntType_Rx=E;Gain_Rx=00.0;Depol=00.0;Land_to=D;Land_from=D;Topo=${FX}/topo;Border=${FX}/border;"
    "Morpho=${FX}/morpho;Coo_Tx=009E000049N0000;Coo_Rx=009E300049N1500";

hcmms::Case make_case(const std::string &fx, const std::string &extra) {
    const std::string line = std::string("t | 0 | 0 | 0.1 | ") + kBase + extra;
    return hcmms::parse_cases(line, fx).at(0);
}

// Rechnet einen Fall auf frischem Zustand.
hcmms::Outputs run_fresh(const hcmms::Case &c) {
    hcmms::State s;
    hcmms::Outputs o;
    std::string i_str = c.i_str;
    hcmms::hcmms_v7(s, c.c_mode, c.bor_dis, c.pd, i_str, o);
    return o;
}

struct DllResult {
    double distance = -1.0;
    std::int16_t h_tx = -1;
    std::int16_t h_rx = -1;
    int error = -1;
    std::vector<float> f = std::vector<float>(21, -1.0F);
    std::vector<double> d = std::vector<double>(4, -1.0);
};

DllResult call_dll(const hcmms::Case &c, std::string &i_str) {
    DllResult r;
    float *f = r.f.data();
    double *d = r.d.data();
    HCMMS_V7_DLL(&c.c_mode, &c.bor_dis, &c.pd, &r.distance, &r.h_tx, &r.h_rx, &r.error, &f[0], &f[1], &f[2], &f[3],
                 &f[4], &f[5], &f[6], &f[7], &f[8], &d[0], &d[1], &f[9], &f[10], &f[11], &f[12], &d[2], &f[13],
                 &f[14], &d[3], &f[15], &f[16], &f[17], &f[18], &f[19], &f[20], i_str.data(), i_str.size());
    return r;
}

void test_short_string(const std::string &fx) {
    const hcmms::Case c = make_case(fx, "");
    std::string i_str = c.i_str.substr(0, hcmms::kIStrMin - 1);
    const std::string before = i_str;
    const DllResult r = call_dll(c, i_str);
    check(r.error == 3000, "zu kurzer I_str liefert 3000");
    check(r.distance == -1.0 && r.f[19] == -1.0F, "zu kurzer I_str laesst Ausgaben unberuehrt");
    check(i_str == before, "zu kurzer I_str bleibt unveraendert");
}

// Jede der 32 Ausgaben der C-Schnittstelle muss an ihrer Position den Wert
// aus hcmms_v7 tragen; der Fall ist so gewaehlt, dass die Werte sich
// moeglichst unterscheiden (Seeweg, Antennen, Frequenzabstand).
void test_full_call(const std::string &fx) {
    const hcmms::Case c = make_case(fx, ";Coo_Tx=008E050048N0500;Coo_Rx=008E300048N1000;AntH_Tx=090EA00;Azi_Tx=045.0;"
                                        "AntH_Rx=090EA00;Azi_Rx=180.0;Gain_Rx=3;Rx_f=00450.02500M;AntType_Rx=I");
    std::string i_str = c.i_str;
    const DllResult r = call_dll(c, i_str);
    const hcmms::Outputs o = run_fresh(c);
    check(r.error == o.HCM_error, "C-Schnittstelle: HCM_error");
    check(r.distance == o.Distance, "C-Schnittstelle: Distance");
    check(r.h_tx == o.H_Datab_Tx && r.h_rx == o.H_Datab_Rx, "C-Schnittstelle: Gelaendehoehen");
    const std::vector<float> want_f = {o.Heff,          o.Dh,          o.Dh_corr,          o.Power_to_Rx,
                                       o.Free_space_FS, o.Land_FS,     o.Sea_FS,           o.Tx_ant_corr,
                                       o.Tx_ant_type_corr, o.Tx_TCA,   o.Rx_TCA,           o.Tx_TCA_corr,
                                       o.Rx_TCA_corr,   o.Rx_ant_corr, o.Rx_ant_type_corr, o.Corr_delta_f,
                                       o.Calculated_FS, o.Perm_FS,     o.CBR_D,            o.ERP_ref_Tx,
                                       o.Prot_margin};
    for (std::size_t k = 0; k < want_f.size(); ++k) {
        check(r.f[k] == want_f[k], "C-Schnittstelle: REAL-Ausgabe " + std::to_string(k));
    }
    const std::vector<double> want_d = {o.Dir_Tx_Rx, o.V_angle_Tx_Rx, o.D_sea_calculated, o.Delta_frequency};
    for (std::size_t k = 0; k < want_d.size(); ++k) {
        check(r.d[k] == want_d[k], "C-Schnittstelle: DOUBLE-Ausgabe " + std::to_string(k));
    }
    check(i_str.substr(376, 6) == "7.21  ", "C-Schnittstelle: Version in I_str");
}

void test_null_gain(const std::string &fx) {
    const hcmms::Outputs zero = run_fresh(make_case(fx, ";Gain_Rx=0   "));
    for (const char *g : {"/   ", ",   ", "2*  "}) {
        const hcmms::Outputs o = run_fresh(make_case(fx, std::string(";Gain_Rx=") + g));
        check(o.HCM_error == 0 && o.Perm_FS == zero.Perm_FS, std::string("Nullwert im Gewinn wie 0 dB: ") + g);
    }
    const hcmms::Outputs dz = run_fresh(make_case(fx, ";Depol=0   "));
    const hcmms::Outputs dn = run_fresh(make_case(fx, ";Depol=/   "));
    check(dn.HCM_error == 0 && dn.Perm_FS == dz.Perm_FS, "Nullwert in der Depolarisation wie 0 dB");
}

}  // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Aufruf: test_interface <testdaten-verzeichnis>\n");
        return 2;
    }
    const std::string fx = argv[1];
    test_short_string(fx);
    test_full_call(fx);
    test_null_gain(fx);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
