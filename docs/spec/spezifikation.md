# Spezifikation — HCMMS C++

**Version:** 0.2.0. **Status:** Draft. **Autor:** Michael Rohner, **Datum:** 2026-11-05.

Technische Festlegungen zu den Anforderungen im
[Lastenheft](lastenheft.md). Bei Konflikt gilt das Lastenheft.

---

## 1. Algorithmen und Datenflüsse

Die Rechenschritte folgen den Fortran-Routinen der Version 7.21.2 eins zu eins:
gleiche Reihenfolge, gleiche Verzweigungen, gleiche Zwischen-Rundungen. Eine
Routine des Originals entspricht einer Funktion der Nachbildung; der Name der
Fortran-Routine steht im Kommentar der Funktion.

### LH-FA-02.a — Lese-Semantik der Eingabefelder

Formatierte Leseoperationen (`Fw.d`, `Iw`) auf einem Feld der Breite w:

- Leerzeichen werden ignoriert; ein Feld nur aus Leerzeichen ergibt 0 ohne Fehler.
- Ohne Dezimalpunkt gilt ein impliziter Dezimalpunkt d Stellen von rechts (`F5.1` liest `  123` als 12.3).
- Erlaubt sind Vorzeichen, Ziffern, ein Dezimalpunkt und ein Exponent (`E`, `D`); jedes andere Zeichen ist ein Fehler.
- Bei einem Lesefehler in `Fw.d` steht die Zielvariable danach auf 0.

List-gesteuerte Leseoperationen (`*`): führende Leerzeichen werden übersprungen;
der Wert endet an Leerzeichen, Komma oder Schrägstrich; ein Feld ohne Wert ist
ein Fehler (Dateiende), ein führendes Komma oder ein Schrägstrich lässt die
Zielvariable unverändert ohne Fehler. Ein logischer Wert (`T`, `.T`, `TRUE`, `F` …)
ergibt für eine `REAL`-Variable −1 (wahr) bzw. 0 (falsch). Lokale Variablen,
die im Original ohne Anfangswert gelesen werden, beginnen bei 0.

Formatierte Ausgabe `F4.1` rundet kaufmännisch (halbe Stelle von null weg) und
schreibt `****`, wenn der Wert nicht in vier Zeichen passt.

### LH-FA-03.a — Numerische Regeln

- Variablen, die im Original `REAL` sind, werden als `float` gespeichert; jede Zuweisung rundet auf einfache Genauigkeit.
- Ausdrücke werden in doppelter Genauigkeit ausgewertet (wie die x87-Einheit mit 53-Bit-Mantisse) und erst bei der Zuweisung gerundet.
- `REAL`-Literale, die in einfacher Genauigkeit nicht exakt darstellbar sind, gehen in einem reinen `REAL`-Ausdruck mit ihrem `float`-Wert ein, in einem Ausdruck mit einer `DOUBLE PRECISION`-Größe dezimal-exakt als `double`.
- Ergebnisse von `REAL`-Intrinsics (`LOG10`, `SQRT`, `COSD` … auf `REAL`-Argumenten) werden auf `float` gerundet.
- `NINT` rundet halbe Werte von null weg, `INT` schneidet ab, ganzzahlige Division schneidet ab.

### LH-FA-06.a — Zustandsmodell

Alle Variablen der COMMON-Blöcke `/NDATA/`, `/CDATA/`, `/Co_ord_cp/` und alle
lokalen Variablen, deren Wert im Original einen Aufruf überdauert, liegen in
einer Zustands-Struktur. Die C-Schnittstelle benutzt eine prozessweite Instanz;
die C++-Schnittstelle erlaubt eigene Instanzen.

## 2. Datenstrukturen und Schemas

### SPEC-001 — Gelände- und Morphologie-Dateien

Pfad `<Topo_path>/<e|w><LLL>/<e|w><LLL><n|s><BB>.<33|63>e` (Morphologie: Endung
`m`, Pfad `Morpho_path`). `LLL` und `BB` sind die südwestliche Ecke des
1°×1°-Blocks. Breite unter 50°: Endung `.33`, `RESH` = 101; sonst `.63`, `RESH` = 51.
Die Datei hat 144 Records (12×12 Blöcke zu 300″), Record `R = 12·BV + BH + 1`,
Recordlänge `202·RESH` Byte: 101 Zeilen zu `RESH` Werten `int16` little-endian.
Höhe −9999 bedeutet „fehlt“; im Morphologie-Wert bedeutet Bit 0 „See“.

### SPEC-002 — Grenzlinien-Dateien

Records zu 176 Byte = 22 `double` little-endian im Bogenmaß: 10 Punkte
(Länge, Breite) und der Mittelpunkt (Wert 21, 22). Dateien unter `Border_path`:
`<Land>.all` (geschlossene Landesgrenze), `<Von><Nach>.000` (Grenzlinie und
Mittelpunkt-Liste), `<Von><Nach>.cbr` (Grenzbereich), `<Von><Nach>.<xxx>`
(x-km-Linie). Ländercodes sind dreistellig, Leerstellen werden zu `_`.

### SPEC-003 — Fall-Datei

Eine Zeile je Fall: `name | C_mode | bor_dis | PD | feld=wert;feld=wert…`.
Zeilen mit `#` am Anfang und Leerzeilen werden übersprungen. Feldnamen und
String-Positionen:

| Feld | Pos. | Feld | Pos. | Feld | Pos. |
|---|---|---|---|---|---|
| Coo_Tx | 1,15 | Coo_Rx | 16,15 | H_Tx | 31,4 |
| H_Rx | 35,4 | AntH_Tx | 39,7 | AntV_Tx | 46,7 |
| Azi_Tx | 53,5 | Ele_Tx | 58,5 | Hant_Tx | 63,4 |
| Hant_Rx | 67,4 | AntType_Tx | 71,1 | Power | 72,6 |
| Tx_f | 78,12 | ChanOcc | 90,1 | SeaT | 91,1 |
| Rad_Tx | 92,5 | Rad_Rx | 97,5 | Dsea | 102,5 |
| Rx_f | 107,12 | Emis_Rx | 119,9 | Emis_Tx | 128,9 |
| AntH_Rx | 137,7 | AntV_Rx | 144,7 | Azi_Rx | 151,5 |
| Ele_Rx | 156,5 | AntType_Rx | 161,1 | Gain_Rx | 162,4 |
| Depol | 166,4 | PermFS | 170,5 | CorDf | 175,4 |
| Land_to | 179,3 | Land_from | 182,3 | MaxCBR | 185,3 |
| Topo | 188,63 | Border | 251,63 | Morpho | 314,63 |

Der Platzhalter `${FX}` in einem Wert wird durch das Verzeichnis der
Testdaten ersetzt. Der String ist 600 Zeichen lang und mit Leerzeichen gefüllt.

### SPEC-004 — Ergebniszeile

`name|err=<E>|<29 Werte>|ver='…'|txnew='…'|rxnew='…'|info='…'|depol='…'|seat='…'`.
Die 29 Werte stehen durch Leerzeichen getrennt in der Reihenfolge: Fehler,
Entfernung, Höhe Tx (DB), Höhe Rx (DB), Heff, Dh, Dh_corr, Power_to_Rx,
Free_space_FS, Land_FS, Sea_FS, Tx_ant_corr, Tx_ant_type_corr, Dir_Tx_Rx,
V_angle_Tx_Rx, Tx_TCA, Rx_TCA, Tx_TCA_corr, Rx_TCA_corr, D_sea_calculated,
Rx_ant_corr, Rx_ant_type_corr, Delta_frequency, Corr_delta_f, Calculated_FS,
Perm_FS, CBR_D, ERP_ref_Tx, Prot_margin. Gleitkommawerte werden mit der
kürzesten eindeutigen Darstellung geschrieben (`float` bzw. `double`).

## 3. Defaults und Konstanten

| Name | Wert | Herkunft |
|---|---|---|
| Version | `7.21  ` | Original, `HCM_MS_V7` |
| Erdradius | 6371.29 km | `Calc_distance`, `PROFILE` |
| Mindest-Punktabstand | 0.03 km, sonst 0.1 km | `HCM_MS_V7` |
| Toleranz `float`-Ausgaben | 0,01 | [LH-QA-01](lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |
| Toleranz Entfernungen | 1e-6 km | [LH-QA-01](lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |

## 4. Fehler-Codes und Logging-Felder

Die Fehlercodes sind die des Originals (Liste im Kopf von `HCM_MS_V7`), dazu
3000 für einen zu kurzen String. Die Bibliothek protokolliert nichts.

## 5. Metriken und Tracing-Felder

Keine.

## 6. Externe Verträge

- C-Funktion `HCMMS_V7_DLL` mit der Parameterliste des Originals; die Länge von `I_str` folgt als letzter Parameter (`size_t`).
- Kommandozeile `hcmms <fall-datei> [<testdaten-verzeichnis>]` schreibt Ergebniszeilen nach SPEC-004 auf die Standardausgabe.

## 7. Festlegungen der Harness-Werkzeuge

Der Golden-Vergleich läuft als ctest-Fall im Test-Gate: Testdaten werden
deterministisch erzeugt, die Fall-Datei gerechnet und jede Zeile gegen den
Golden-Datensatz mit den Toleranzen aus §3 verglichen.

## 8. Historie

| Version | Datum | Änderung | Verweis |
|---|---|---|---|
| 0.1.0 | 2026-11-05 | Initiale Fassung | — |
| 0.2.0 | 2026-11-05 | Lesefehler setzt null, logische Listenwerte, Literal-Genauigkeit nach Ausdruck | — |
