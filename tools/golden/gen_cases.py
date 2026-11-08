# Erzeugt tests/golden/cases.txt (SPEC-003). Aufruf aus dem Repo-Wurzelverzeichnis:
#   python tools/golden/gen_cases.py
# Danach den Golden-Datensatz mit Invoke-Hcmms.ps1 aus der Referenz-DLL neu erzeugen.

B = ("H_Tx=0400;H_Rx=0300;AntH_Tx=000ND00;AntV_Tx=000ND00;Azi_Tx=000.0;Ele_Tx=000.0;Hant_Tx=0030;Hant_Rx=0010;"
     "AntType_Tx=E;Power=0010.0;Tx_f=00450.00000M;ChanOcc=1;Rx_f=00450.01250M;Emis_Rx=12K5F3E;Emis_Tx=12K5F3E;"
     "AntH_Rx=000ND00;AntV_Rx=000ND00;Azi_Rx=000.0;Ele_Rx=000.0;AntType_Rx=E;Gain_Rx=00.0;Depol=00.0;"
     "Land_to=D;Land_from=D;Topo=${FX}/topo;Border=${FX}/border;Morpho=${FX}/morpho")
TX = "009E000049N0000"
RX = "009E300049N1500"
out = ["# Golden-Faelle (SPEC-003). Erzeugt mit der Referenz-DLL ueber die Testdaten",
       "# aus tests/fixtures.cpp; die Reihenfolge ist Teil des Tests (Zustand ueber Aufrufe).",
       "# name | C_mode | bor_dis | PD | feld=wert;..."]


def c(name, cmode=0, bord=0, pd=0.1, tx=TX, rx=RX, extra="", base=True):
    f = (B + ";" if base else "") + f"Coo_Tx={tx};Coo_Rx={rx}" + (";" + extra if extra else "")
    out.append(f"{name} | {cmode} | {bord} | {pd} | {f}")


# --- Punkt-zu-Punkt Grundfaelle
c("p2p_base")
c("p2p_pd005", pd=0.05)
c("p2p_pd02", pd=0.2)
c("p2p_pd001", pd=0.01)
c("p2p_short_5km", rx="009E040049N0100")
c("p2p_lt1km", rx="009E000049N0020")
c("p2p_lt_pd", rx="009E000049N0001")
c("p2p_long_80km", rx="009E500049N3000")
c("p2p_long_150km", tx="008E400048N1000", rx="009E500049N1500")
c("p2p_800km_no_tile_36", rx="020E000049N0000")
c("p2p_over1000_1028", rx="024E000049N0000")
c("line_over1000_1047", -1, 0, tx="024E000049N0000", rx="", extra="Land_to=F;Land_from=D")
# Lange Profile bis knapp unter die Feldgrenze von 10002 Punkten (ADR-0006)
c("long_pd003_299km", pd=0.03, tx="008E000048N0000", rx="009E300050N3000")
c("long_pd005_361km", pd=0.05, tx="008E000048N0000", rx="009E590050N5900")
c("long_pd010_361km", pd=0.1, tx="008E000048N0000", rx="009E590050N5900")
c("p2p_lowercase", tx="009e000049n0000", rx="009e300049n1500")
c("p2p_n50_tile", tx="008E400050N1000", rx="009E100050N2500")
c("p2p_no_morpho_tile", tx="009E100050N1000", rx="009E400050N2000")
c("p2p_hole_400", rx="009E425049N4234")
c("p2p_no_terrain_36", rx="010E200049N1000", extra="Gain_Rx=03.0;Depol=-9.9")
c("p2p_dsea_input", extra="Dsea=012.5")
c("p2p_seaT_W", extra="SeaT=W")
c("p2p_seaT_C", extra="SeaT=C")
# Hoehen-Eingaben
c("p2p_htx_blank", extra="H_Tx=    ;H_Rx=    ")
c("p2p_htx_close", extra="H_Tx=0300;H_Rx=0330")
c("p2p_htx_far", extra="H_Tx=0100;H_Rx=0900")
c("p2p_htx_bad", extra="H_Tx=12X4")
c("p2p_hrx_bad", extra="H_Rx=1.00")
# Frequenzen, Baender, Zeitprozente
for nm, f in [("f30", "00035.00000M"), ("f70", "00070.00000M"), ("f80", "00080.00000M"), ("f148", "00148.00000M"),
              ("f160", "00160.00000M"), ("f382", "00382.00000M"), ("f392", "00392.00000M"), ("f420", "00420.00000M"),
              ("f900", "00900.00000M"), ("f1750", "01750.00000M"), ("f1850", "01850.00000M"),
              ("f29_7", "00029.70000M"), ("f47", "00047.00000M"), ("f149_9", "00149.90000M"),
              ("f_khz", "450000.0000K"), ("f_ghz", "0000.900000G")]:
    c("band_" + nm, extra=f"Tx_f={f};Rx_f={f}")
c("band_f1785_above", extra="Tx_f=01785.00001M;Rx_f=01785.00001M")
c("band_f1710_below", extra="Tx_f=01709.99999M;Rx_f=01709.99999M")
c("band_f470_above", extra="Tx_f=00470.00001M;Rx_f=00470.00001M")
c("band_outside_1050", extra="Tx_f=00300.00000M;Rx_f=00300.00000M")
c("band_outside_perm", extra="Tx_f=00300.00000M;Rx_f=00300.00000M;PermFS=030.0")
c("band_chanocc0", extra="ChanOcc=0")
c("tetra_t10", extra="Tx_f=00382.00000M;Rx_f=00382.02500M;Emis_Tx=25K0G7W;Emis_Rx=25K0G7W")
c("mode12", 12, extra="PermFS=030.0")
c("mode11", 11, extra="PermFS=030.0")
c("mode10", 10, extra="PermFS=030.0;Tx_f=01800.00000M;Rx_f=01800.00000M")
c("mode10_noperm_1050", 10)
c("mode99", 99, extra="Rad_Rx=00010")
c("mode_bad_1025", 5)
c("maxcbr_input", extra="MaxCBR=045")
# Freie Fresnelzone, kleine und grosse Antennen
c("fresnel_free", rx="009E020049N0030", extra="Hant_Tx=0300;Hant_Rx=0200")
c("heff_low", extra="Hant_Tx=0002;Hant_Rx=0002;H_Tx=    ;H_Rx=    ")
c("heff_low_long", rx="009E500049N3000", extra="Hant_Tx=0001;Hant_Rx=0001")
c("heff_high", extra="Hant_Tx=0900;Hant_Rx=0050")
c("heff_huge", extra="Hant_Tx=3000")
# Seewege (See suedwestlich 8.3E/48.3N)
c("sea_mixed_t1", tx="008E050048N0500", rx="008E300048N1000")
c("sea_mixed_t10", tx="008E050048N0500", rx="008E300048N1000", extra="ChanOcc=0")
c("sea_all", tx="008E050048N0500", rx="008E150048N1000")
c("sea_mixed_mode11", 11, tx="008E050048N0500", rx="008E400048N2000", extra="PermFS=030.0")
c("sea_mixed_f900", tx="008E050048N0500", rx="008E300048N1000", extra="Tx_f=00900.00000M;Rx_f=00900.00000M")
# Sendeantennen
for t, az, el in [("090EA00", "045.0", "000.0"), ("060EB10", "100.0", "002.0"), ("080EC05", "300.0", "-03.0"),
                  ("045DE00", "070.0", "000.0"), ("060LA20", "060.0", "000.0"), ("050KA00", "200.0", "000.0"),
                  ("040CA00", "080.0", "000.0"), ("050CB00", "080.0", "000.0"), ("060CC00", "080.0", "000.0"),
                  ("120TA00", "050.0", "000.0"), ("225VA00", "070.0", "000.0"), ("110WC32", "060.0", "000.0")]:
    c("txant_h_" + t, extra=f"AntH_Tx={t};Azi_Tx={az};Ele_Tx={el}")
for t, el in [("030EA00", "005.0"), ("040KA00", "-02.0"), ("120TA00", "003.0")]:
    c("txant_v_" + t, extra=f"AntV_Tx={t};Azi_Tx=000.0;Ele_Tx={el}")
c("txant_hv", extra="AntH_Tx=090EA00;AntV_Tx=030EA00;Azi_Tx=045.0;Ele_Tx=002.0")
c("txant_tilt_T", extra="AntH_Tx=090EA00;AntV_Tx=080TA00;Azi_Tx=045.0;Ele_Tx=-02.0")
c("txant_P", extra="AntH_Tx=090EA00;AntV_Tx=080PB00;Azi_Tx=045.0;Ele_Tx=000.0")
c("txant_same_dir", extra="AntH_Tx=090EA00;Azi_Tx=052.7;Ele_Tx=000.0")
c("txant_bad_type_1038", extra="AntH_Tx=090XX00;Azi_Tx=045.0")
c("txant_bad_lead_1038", extra="AntH_Tx=070EA00;Azi_Tx=045.0")
c("txant_bad_ele_1031", extra="AntH_Tx=090EA00;Azi_Tx=045.0;Ele_Tx=0A0.0")
c("txant_bad_azi_1032", extra="AntH_Tx=090EA00;Azi_Tx=04X.0")
c("txant_type_I", extra="AntType_Tx=I")
# Empfangsantennen, Gewinn, Depolarisation
c("rxant_h", extra="AntH_Rx=090EA00;Azi_Rx=180.0;Ele_Rx=000.0")
c("rxant_v", extra="AntV_Rx=030EA00;Azi_Rx=0;Ele_Rx=3")
c("rxant_ele_blank_ok", extra="AntH_Rx=090EA00;Azi_Rx=180;Ele_Rx=     ")
c("rxant_ele_blank_1042", extra="AntV_Rx=030EA00;Azi_Rx=180;Ele_Rx=     ")
c("rxant_azi_blank_1043", extra="AntH_Rx=090EA00;Azi_Rx=     ;Ele_Rx=0")
c("rxant_type_I", extra="AntType_Rx=I")
c("rxant_type_bad_1044", extra="AntType_Rx=X")
c("gain_3", extra="Gain_Rx=3")
c("gain_blank_1045", extra="Gain_Rx=    ")
c("gain_comma", extra="Gain_Rx=3,5")
c("gain_repeat", extra="Gain_Rx=2*4")
c("depol_auto", extra="Depol=-9.9;Hant_Tx=0030;Hant_Rx=0020")
c("depol_auto_zero", extra="Depol=-9.9")
c("depol_blank_1046", extra="Depol=    ")
c("depol_bad_1046", extra="Depol=ABC")
# Frequenzabstand
c("df_tetra_both", extra="Tx_f=00382.00000M;Rx_f=00382.03000M;Emis_Tx=25K0G7W;Emis_Rx=25K0G7W")
c("df_tetra_tx", extra="Tx_f=00382.00000M;Rx_f=00382.01500M;Emis_Tx=25K0G7W;Emis_Rx=12K5F3E")
c("df_tetra_rx", extra="Tx_f=00382.00000M;Rx_f=00382.01200M;Emis_Tx=12K5F3E;Emis_Rx=25K0G7W")
c("df_narrow", extra="Rx_f=00450.01000M;Emis_Tx=12K5F3E;Emis_Rx=25K0F3E")
c("df_wide", extra="Rx_f=00450.10000M;Emis_Tx=200KF3E;Emis_Rx=50K0F3E")
c("df_wide_mid", extra="Rx_f=00450.08000M;Emis_Tx=100KF3E;Emis_Rx=50K0F3E")
c("df_mhz_emis", extra="Rx_f=00451.00000M;Emis_Tx=1M25G7W;Emis_Rx=1M25G7W")
c("df_bad_tx_1041", extra="Emis_Tx=ABCDEFG")
c("df_bad_rx_1040", extra="Emis_Rx=12X5F3E")
c("df_cor_input", extra="CorDf=12.5")
c("df_cor_bad_1039", extra="CorDf=XX")
# Mobile Stationen (Grenzdateien D__.all / F__.all)
c("mob_tx", tx="009E000048N3000", rx="008E200048N4500", extra="Rad_Tx=00030;Land_to=F")
c("mob_tx_small", tx="009E000048N3000", rx="008E200048N4500", extra="Rad_Tx=00005;Land_to=F")
c("mob_rx", tx="009E000048N3000", rx="008E200048N4500", extra="Rad_Rx=00020;Land_to=F")
c("mob_both_tx_gt", tx="009E000048N3000", rx="008E200048N4500", extra="Rad_Tx=00030;Rad_Rx=00010;Land_to=F")
c("mob_both_rx_gt", tx="009E000048N3000", rx="008E200048N4500", extra="Rad_Tx=00010;Rad_Rx=00030;Land_to=F")
c("mob_overlap", tx="009E000048N3000", rx="009E100048N3500", extra="Rad_Tx=00020;Rad_Rx=00020")
c("mob_no_cut", tx="009E300049N0000", rx="009E500049N3000", extra="Rad_Tx=00010")
c("mob_missing_tx_1036", extra="Rad_Tx=00010;Land_from=X")
c("mob_missing_rx_1037", extra="Rad_Rx=00010;Land_to=X")
c("mob_rad_bad_1012", extra="Rad_Tx=1X")
c("mob_rad_bad_1029", extra="Rad_Rx=1X")
# Linienberechnung (D__F__.000/.020/.cbr)
L = "Land_to=F;Land_from=D"
c("line_border", -1, 0, tx="009E000048N3000", rx="", extra=L)
c("line_border_n49", -1, 0, tx="009E100049N3000", rx="", extra=L)
c("line_20km", -1, 20, tx="009E000048N3000", rx="", extra=L)
c("line_cbr", -1, -1, tx="009E000048N3000", rx="", extra=L)
c("p2p_after_line", tx="009E000049N0000", rx=RX)
c("line_m9", -9, 0, tx="009E000049N0000", rx="", extra=L + ";PermFS=020.0")
c("line_m10", -10, 0, tx="009E000049N0000", rx="", extra=L + ";PermFS=020.0")
c("line_m11_cbr", -11, -1, tx="009E000049N0000", rx="", extra=L + ";PermFS=020.0;MaxCBR=030")
c("line_m11_cbr_1050", -11, -1, tx="009E000049N0000", rx="", extra=L + ";PermFS=020.0")
c("line_tx_mobile", -1, 0, tx="009E200048N4000", rx="", extra=L + ";Rad_Tx=00015")
c("line_tx_mobile_cbr", -1, -1, tx="009E200048N4000", rx="", extra=L + ";Rad_Tx=00015")
c("line_ant", -1, 0, tx="009E000048N3000", rx="", extra=L + ";AntH_Tx=090EA00;Azi_Tx=270.0")
c("line_digital_wide", -1, 0, tx="009E000048N3000", rx="", extra=L + ";Emis_Tx=50K0G7W")
c("line_missing_1048", -1, 0, tx="009E000048N3000", rx="", extra="Land_to=X;Land_from=D")
c("line_missing_x_1048", -1, 55, tx="009E000048N3000", rx="", extra=L)
c("line_bordis_1047", -1, 1000, tx="009E000048N3000", rx="", extra=L)
c("line_reverse", -1, 0, tx="008E200048N4500", rx="", extra="Land_to=D;Land_from=F")
c("line_far_no_tile_220", -1, 0, tx="020E000048N3000", rx="", extra=L)
c("p2p_after_line2", tx="008E400048N1000", rx="009E200048N3000")
# Eingabefehler HCM_MS_V7
for nm, fld in [("1001", "Coo_Tx=0X8E000049N0000"), ("1002", "Coo_Tx=009EX00049N0000"),
                ("1003", "Coo_Tx=009E00X049N0000"), ("1004", "Coo_Tx=009X000049N0000"),
                ("1005", "Coo_Tx=009E00004XN0000"), ("1006", "Coo_Tx=009E000049NX000"),
                ("1007", "Coo_Tx=009E000049N00X0"), ("1008", "Coo_Tx=009E000049X0000"), ("1009", "Hant_Tx=X"),
                ("1010", "Tx_f=0045X.00000M"), ("1011", "Tx_f=00450.00000X"), ("1033", "AntType_Tx=X"),
                ("1034", "Power=00X0.0"), ("1014", "Coo_Rx=0X9E300049N1500"), ("1015", "Coo_Rx=009EX00049N1500"), ("1016", "Coo_Rx=009E30X049N1500"), ("1017", "Coo_Rx=009X300049N1500"),
                ("1018", "Coo_Rx=009E30004XN1500"), ("1019", "Coo_Rx=009E300049NX500"), ("1020", "Coo_Rx=009E300049N15X0"),
                ("1021", "Coo_Rx=009E300049X1500"), ("1022", "Hant_Rx=X"), ("1023", "Rx_f=00000.00000M"),
                ("1023b", "Rx_f=0045X.00000M"), ("1024", "Rx_f=00450.00000X"), ("1026", "PermFS=0X0.0"),
                ("1026b", "PermFS=1X0.0"), ("1026c", "PermFS=25.X0"), ("1027", "MaxCBR=0X5"), ("1035", "Dsea=1X.0")]:
    c("err_" + nm, extra=fld)
c("err_1000_same", rx=TX)
c("err_empty", tx="", rx="", base=False)
# Feld-Lesesemantik (formatiert)
for nm, fld in [("hant_sign", "Hant_Tx=+030"), ("hant_neg", "Hant_Tx=-005"), ("hant_dash", "Hant_Tx= -  "),
                ("hant_blank", "Hant_Tx=    "), ("hant_inner_blank", "Hant_Tx= 3 0"), ("hant_point", "Hant_Tx=30.0"),
                ("hant_exp", "Hant_Tx=3E1 "), ("hant_comma", "Hant_Tx=3,00"),
                ("pow_implied", "Power=  0100"), ("pow_exp", "Power=1.0E1 "), ("pow_dot", "Power=.     "),
                ("pow_comma", "Power=1,0   "), ("pow_dsign", "Power=1.0+1 "), ("pow_D", "Power=1.0D1 "),
                ("pow_plusdot", "Power=+.5   "), ("pow_E", "Power=1E    "),
                ("freq_implied", "Tx_f=  45000000000M"), ("freq_blank", "Tx_f=           M"),
                ("coo_blanks", "Coo_Tx=  9E 0 049N0 00"), ("dsea_implied", "Dsea=  125")]:
    c("fmt_" + nm, extra=fld)
# Liste (Gewinn): Permissble_FS_calculation laeuft auch nach Fehler 36.
# Reine Nullwerte ('/', ',', 'r*') fehlen bewusst: im Original undefiniert (ADR-0004).
NT = "Topo=${FX}/none;Rx_f=00450.01250M"
for nm, g in [("plain", "3   "), ("dot", ".   "), ("trail_dot", "3.  "), ("exp", "1E1 "), ("D", "1D1 "),
              ("sign_exp", "5-1 "), ("lead_blank", "  7 "), ("plus", "+2  "),
              ("letters", "T   "), ("quote", "\"1\" "), ("colon", "1:2 "), ("F", "F   "), ("dotT", ".T  "),
              ("TRUE", "TRUE"), ("FALS", "FALS")]:
    c("list_gain_" + nm, extra=NT + f";Gain_Rx={g}")
for nm, d in [("auto", "-9.9"), ("neg", "-1  "), ("frac", ".5  ")]:
    c("list_depol_" + nm, extra=NT + f";Depol={d}")
# Zustand: Fehler gefolgt von gueltigem Fall, veraltete Werte
c("state_err_then", extra="Coo_Tx=0X9E000049N0000")
c("state_ok_after_err")
c("state_rx_ele_stale_a", extra="AntV_Rx=030EA00;AntH_Rx=090EA00;Azi_Rx=010;Ele_Rx=4")
c("state_rx_ele_stale_b", extra="AntH_Rx=090EA00;Azi_Rx=010;Ele_Rx=     ")
# Review probes (appended, so earlier cases keep their state history).
# Overflow of the target type is a read error, underflow reads as 0.
c("probe_base")
for nm, fld in [("pow_e999", "Power=1E999 "), ("pow_e39_scaled", "Power=1E39  "), ("pow_em99", "Power=1E-99 "),
                ("radtx_e99", "Rad_Tx=1E99 "), ("txf_e999", "Tx_f=    1.0E999M"), ("txf_em999", "Tx_f=   1.0E-999M"),
                ("rxf_e999", "Rx_f=    1.0E999M")]:
    c("probe_" + nm, extra=fld)
    c("probe_" + nm + "_next")
# F fields without digits or with an empty exponent are accepted by the original.
for nm, fld in [("permfs_trail_sign", "PermFS=12.5-"), ("permfs_exp_nodigit", "PermFS=  1E "),
                ("pow_dot_only", "Power=  .   "), ("radtx_dot", "Rad_Tx=  .  "), ("radtx_minus", "Rad_Tx=  -  "),
                ("radtx_E5", "Rad_Tx=  E5 "), ("depol_exp_nodigit", "Depol=1E  ")]:
    c("probe_" + nm, extra=fld)
# A null value in the Rx bandwidth keeps the value read for Tx (shared local X1).
c("probe_emis_rx_slash", extra="Emis_Tx=12K5F3E;Emis_Rx=/K00F3E")
c("probe_emis_rx_comma", extra="Emis_Tx=25K0F3E;Emis_Rx=,K00F3E")
# V7.21.3: 'H' as 4th character of the bandwidth designation (Hz).
c("probe_emis_100H", extra="Emis_Tx=100HF3E;Emis_Rx=100HF3E")
c("probe_emis_tx_H_rx_K", extra="Emis_Tx=800HF3E;Emis_Rx=12K5F3E")
c("probe_end")
open("tests/golden/cases.txt", "w", newline="\n").write("\n".join(out) + "\n")
print(len(out) - 3, "Faelle")
