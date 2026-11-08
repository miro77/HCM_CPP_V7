# Lastenheft — HCMMS C++

**Version:** 0.1.0 (`Major.Minor.Patch`). Ab `Accepted` ist der Bump der
Fußabdruck des Change Requests; **welche Stelle**
steigt, folgt Semantic Versioning.

**Status:** Draft

**Autor:** Michael Rohner, **Datum:** 2026-11-05.

---

## 1. Zweck und Geltungsbereich

HCMMS C++ ist eine Nachbildung des Fortran-Moduls *HCM Mobile Service V7.21*
(Harmonized Calculation Method, Mobilfunkdienst) in C++. Maßgeblich ist das
Verhalten der Referenz-DLL `HCMMS_V7212.dll` (Version 7.21.2, Export
`HCMMS_V7_DLL`): Für dieselbe Eingabe liefert die Nachbildung dieselben
Fehlercodes, dieselben Text-Ausgaben und Zahlenwerte innerhalb der in
[LH-QA-01](#lh-qa-01--genauigkeit-gegenüber-der-referenz) festgelegten Toleranz.

Geltungsbereich ist die Berechnungslogik samt Eingabe-Auswertung und Zugriff auf
Gelände-, Morphologie- und Grenzlinien-Dateien. Die Nachbildung ersetzt die
Referenz-DLL in Anwendungen, die heute eine 32-Bit-Fortran-Laufzeit brauchen.

## 2. Stakeholder

| Stakeholder | Rolle | Erwartung |
|---|---|---|
| Entwicklung der aufrufenden Anwendungen | Nutzer der Bibliothek | Gleiche Ergebnisse wie die Referenz-DLL, wartbarer Quellcode, 64-Bit-fähig |
| Frequenzkoordination | Fachlicher Abnehmer | Keine Änderung der Koordinationsergebnisse durch den Wechsel |
| Betrieb | Auslieferung | Keine Fortran-Laufzeit, keine Abhängigkeiten außer der C++-Standardbibliothek |

## 3. Funktionale Anforderungen

### LH-FA-01 — Aufrufschnittstelle wie HCMMS_V7_DLL

**Beschreibung:** Die Bibliothek bietet eine Funktion mit derselben
Parameterliste, denselben Parametertypen und demselben Layout des Ein-/Ausgabe-
Strings `I_str` (Positionen 1–432 und ab 433 der Debug-Pfad) wie
`HCMMS_V7_DLL`. Ausgaben stehen in denselben Parametern und String-Positionen.

**Akzeptanzkriterien:**

- **Happy Path:** Given ein gültiger Eingabe-String und ein Rechenmodus, when die Funktion aufgerufen wird, then sind alle 32 Zahl-Ausgaben, Version (377–382), Info-Flags (383–402), neue Koordinaten (403–432), Depolarisationsverlust (166–169) und Meerestemperatur (91) gesetzt wie bei der Referenz.
- **Boundary:** Given ein String mit Kleinbuchstaben und NUL-Zeichen, when die Funktion aufgerufen wird, then werden Kleinbuchstaben a–z in Großbuchstaben und NUL in Leerzeichen gewandelt, bevor ausgewertet wird.
- **Negative:** Given ein String kürzer als 432 Zeichen, when die Funktion aufgerufen wird, then ist der Fehlercode 3000 und kein weiterer Wert wird verändert.

**Out-of-Scope:** Die Datei `DEBUG.TXT` und die Profil-Ausgabe bei vorhandener Datei `HCM_PR` werden nicht geschrieben.

---

### LH-FA-02 — Eingabe-Auswertung und Fehlercodes

**Beschreibung:** Die Felder des Eingabe-Strings werden nach denselben Regeln
gelesen wie im Fortran-Original (formatierte Leseoperationen mit Feldbreite und
implizitem Dezimalpunkt, list-gesteuerte Leseoperationen), und jede ungültige
Eingabe führt zum selben Fehlercode (1000–1050, 2000–2005, 36, 220, 400, 3000).

**Akzeptanzkriterien:**

- **Happy Path:** Given Felder im dokumentierten Format, when ausgewertet wird, then entsprechen die gelesenen Werte denen der Referenz.
- **Boundary:** Given Felder mit Leerzeichen, ohne Dezimalpunkt oder nur aus Leerzeichen, when ausgewertet wird, then gilt dieselbe Lese-Semantik wie im Original (Leerzeichen ignoriert, impliziter Dezimalpunkt, leeres Feld je nach Leseart 0 oder Fehler).
- **Negative:** Given ein Feld mit unzulässigen Zeichen, when ausgewertet wird, then ist der Fehlercode derselbe wie bei der Referenz.

**Out-of-Scope:** Fehlermeldungstexte; die Schnittstelle liefert nur Codes.

---

### LH-FA-03 — Punkt-zu-Punkt-Berechnung

**Beschreibung:** Für die Modi 0, 10, 11, 12 und 99 berechnet die Bibliothek
Entfernung, Richtungen, Höhenprofil, Freiraum-, Land-, See- und Mischpfad-
Feldstärke, Korrekturen (Antennen, Geländefreiwinkel, Geländeunebenheit,
Frequenzabstand, Depolarisation) und die zulässige Feldstärke wie das Original.

**Akzeptanzkriterien:**

- **Happy Path:** Given feste Sender- und Empfängerstandorte mit Geländedaten, when gerechnet wird, then stimmen alle Ausgaben mit der Referenz innerhalb der Toleranz überein.
- **Boundary:** Given Entfernungen unter 1 km, über 1000 km, Pfade über See und freie erste Fresnelzone, when gerechnet wird, then gelten dieselben Sonderfälle und Info-Flags wie im Original.
- **Negative:** Given fehlende Geländedateien, when gerechnet wird, then ist der Fehlercode 36 und die nachfolgende Auswertung läuft wie im Original weiter.

**Out-of-Scope:** Fachliche Korrekturen am Rechenverfahren; Abweichungen vom HCM-Abkommen werden nachgebildet, nicht behoben.

---

### LH-FA-04 — Linien- und Grenzbereichsberechnung

**Beschreibung:** Für die Modi −1, −9, −10 und −11 sucht die Bibliothek auf
einer Grenzlinie, einer x-km-Linie oder dem Grenzbereich den Punkt maximaler
Feldstärke mit demselben Suchverfahren wie das Original und liefert dessen
Ergebnisse.

**Akzeptanzkriterien:**

- **Happy Path:** Given Grenzlinien-Dateien eines Länderpaars, when eine Linienberechnung läuft, then sind der gefundene Linienpunkt und alle Ausgaben dieselben wie bei der Referenz.
- **Boundary:** Given eine Linienentfernung von 0, positiv oder negativ (Grenzbereich), when gerechnet wird, then werden dieselben Dateien und Suchschritte verwendet.
- **Negative:** Given fehlende Liniendateien oder eine Entfernung über 999 km, when gerechnet wird, then ist der Fehlercode 1048 bzw. 1047.

**Out-of-Scope:** Der Testmodus über die Datei `HCM_LP` im Arbeitsverzeichnis.

---

### LH-FA-05 — Mobile Stationen mit Versorgungsbereich

**Beschreibung:** Hat Sender oder Empfänger einen Versorgungsradius, bestimmt
die Bibliothek die maßgeblichen Positionen über die Länder-Grenzdateien
(`<Land>.all`) wie das Original, einschließlich Überlappung und Schnitt der
Grenzlinie.

**Akzeptanzkriterien:**

- **Happy Path:** Given Versorgungsradien ohne Schnitt der Grenze, when gerechnet wird, then liegen die neuen Koordinaten auf dem Kreis in Richtung der Gegenstation wie bei der Referenz.
- **Boundary:** Given überlappende Versorgungsbereiche, when gerechnet wird, then ist Info 7 gesetzt und die Feldstärke 999.9.
- **Negative:** Given eine fehlende Grenzdatei, when gerechnet wird, then ist der Fehlercode 1036 (Sender) bzw. 1037 (Empfänger).

**Out-of-Scope:** —

---

### LH-FA-06 — Zustand über Aufrufe

**Beschreibung:** Werte, die im Original über COMMON-Blöcke und statische
Variablen von einem Aufruf zum nächsten erhalten bleiben, bleiben in der
Nachbildung ebenso erhalten, sodass eine Folge von Aufrufen dieselben Ergebnisse
liefert wie dieselbe Folge gegen die Referenz.

**Akzeptanzkriterien:**

- **Happy Path:** Given eine Folge gemischter Aufrufe in einem Prozess, when sie nacheinander ausgeführt wird, then stimmt jedes Ergebnis mit der Referenz überein.
- **Boundary:** Given ein Aufruf nach einer Linienberechnung, when eine Punkt-zu-Punkt-Berechnung folgt, then wirkt der übernommene Zustand wie im Original.
- **Negative:** Given ein Aufruf mit Fehler, when der nächste Aufruf folgt, then wirken nur die Zustände nach, die auch im Original nachwirken.

**Out-of-Scope:** Gleichzeitige Aufrufe aus mehreren Threads auf denselben Zustand.

---

### LH-FA-07 — Fall-Abarbeitung über die Kommandozeile

**Beschreibung:** Ein Kommandozeilen-Programm liest eine Datei mit Rechenfällen,
ruft die Schnittstelle für jeden Fall nacheinander auf und schreibt je Fall eine
Ergebniszeile im selben Format wie das Vergleichswerkzeug der Referenz-DLL.

**Akzeptanzkriterien:**

- **Happy Path:** Given eine Fall-Datei, when das Programm läuft, then entsteht je Fall genau eine Ergebniszeile.
- **Boundary:** Given Kommentar- und Leerzeilen, when das Programm läuft, then werden sie übersprungen.
- **Negative:** Given ein unbekanntes Feld in einer Fallzeile, when das Programm läuft, then bricht es mit Exit-Code ungleich 0 und Meldung ab.

**Out-of-Scope:** Interaktive Bedienung.

---

## 4. Nichtfunktionale Anforderungen und Randbedingungen

### LH-QA-01 — Genauigkeit gegenüber der Referenz

- **Anforderung:** Fehlercodes, Info-Flags und alle Text-Ausgaben sind identisch zur Referenz-DLL. Zahl-Ausgaben weichen höchstens um 0,01 (dB, m, Grad) bzw. bei Entfernungen um 1e-6 km ab.
- **Messmethode:** Vergleich gegen einen mit der Referenz-DLL erzeugten Golden-Datensatz über synthetische Geländedaten; der Vergleich läuft im Test-Gate.

### LH-QA-02 — Portabilität

- **Anforderung:** Quellcode in C++17 ohne Abhängigkeiten außer der Standardbibliothek; baut unter Linux (GCC) und Windows (MSVC oder MinGW), 32 und 64 Bit.
- **Messmethode:** Build und Tests im Docker-Gate (Linux, 64 Bit).

### LH-RB-01 — Lizenz

- **Art:** rechtlich
- **Vorgabe:** Die Nachbildung ist ein abgeleitetes Werk des unter GNU LGPL v2.1 oder später stehenden Fortran-Codes und steht selbst unter LGPL v2.1 oder später; die Copyright-Hinweise der Autoren bleiben erhalten.
- **Nachweis:** Lizenzdatei und Copyright-Hinweis im Repository.

---

## 5. Globale Out-of-Scope-Punkte

- Korrekturen am Rechenverfahren oder an Fehlern des Originals.
- Ausgabe von Debug- und Profil-Dateien.
- Eine grafische Oberfläche.

## 6. Glossar

| Begriff | Bedeutung im Lastenheft |
|---|---|
| Referenz-DLL | `HCMMS_V7212.dll`, Version 7.21.2 des Fortran-Moduls |
| Golden-Datensatz | Ergebniszeilen der Referenz-DLL für eine feste Fall-Datei |
| Modus (C_mode) | Rechenart: ≥ 0 Punkt-zu-Punkt, < 0 Linie |

## 7. Historie

Regeln dieser Sektion: Ab Status `Accepted` ist **jede** Änderung an diesem
Dokument eine Vertragsänderung. Fußabdruck pro angenommenem CR: Version-Bump
oben plus eine Zeile hier.

| Version | Datum | Änderung | Verweis |
|---|---|---|---|
| 0.1.0 | 2026-11-05 | Initiale Fassung | — |
