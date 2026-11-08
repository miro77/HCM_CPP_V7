# ADR-0003: REAL-Literale in DOUBLE-Ausdrücken mit doppelter Genauigkeit

**Status:** Accepted
**Datum:** 2026-11-05
**Autor:** Michael Rohner
**Bezug:** [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz), [`LH-FA-02`](../spec/lastenheft.md#lh-fa-02--eingabe-auswertung-und-fehlercodes)
**Schärft:** [`LH-FA-03.a`](../spec/spezifikation.md#lh-fa-03a--numerische-regeln)
**Supersedes:** ADR-0002 (nur die Regel zu `REAL`-Literalen; float-Speicher und double-Auswertung bleiben)
**Regeln:** ADR-Format MADR.

---

## Kontext

ADR-0002 legt fest, dass `REAL`-Literale mit ihrem `float`-Wert in die Rechnung
eingehen. Der erste Golden-Vergleich widerlegt das für Ausdrücke mit einer
`DOUBLE PRECISION`-Größe: Die Referenz-DLL ordnet 29,7 MHz und 149,9 MHz dem
Frequenzband zu (Vergleich `Tx_frequency >= 29.7` bzw. `<= 149.9`), mit
`float`-Literalen fielen beide heraus. Gezielte Proben trennen die beiden
verbleibenden Deutungen: 1785,00001 MHz und 1709,99999 MHz liegen in der
Referenz **außerhalb** des Bands; ein Vergleich in einfacher Genauigkeit hätte
sie innerhalb gesehen. Der Compiler wertet das Literal also dezimal-exakt in
doppelter Genauigkeit aus, sobald der Ausdruck `DOUBLE PRECISION` ist.

## Entscheidung

Wir wählen: **Ein `REAL`-Literal geht mit der Genauigkeit des Ausdrucks ein,
in dem es steht.** In einem Ausdruck, der eine `DOUBLE PRECISION`-Größe enthält,
wird es als `double`-Literal geschrieben; in einem reinen `REAL`-Ausdruck bleibt
es beim `float`-Wert nach ADR-0002.

## Verglichene Alternativen

| Option | Pro | Contra |
|---|---|---|
| A — ADR-0002 unverändert, Literal immer `float` | einheitliche Regel | Bandgrenzen kippen, Fehlercode 1050 statt 0 |
| B — Vergleich in einfacher Genauigkeit | schließt 29,7/149,9 | widerspricht den Proben bei 1785,00001 und 1709,99999 |
| **C — Literal folgt der Genauigkeit des Ausdrucks** | deckt alle Proben | jede Literal-Stelle braucht einen Blick auf den Ausdruckstyp |

## Konsequenzen

- Positiv: Bandzuordnung und Fehlercodes an Bandgrenzen stimmen mit der Referenz.
- Negativ: Die Literal-Regel hängt vom Kontext ab; die Stellen sind im Code kommentiert.
- Betroffen sind die Bandtabelle der Ablaufsteuerung und die Fresnelzonen-Höhe im Punkt-zu-Punkt-Pfad.

## Fitness Function (falls maschinell prüfbar)

| Tooling | Regel | Make-Target |
|---|---|---|
| ctest `golden_compare` | Fälle `band_f29_7`, `band_f149_9`, `band_f1785_above`, `band_f1710_below` | `make test` |

## Re-Evaluierungs-Trigger

Wenn ein Golden-Fall mit einem Literal in einem `REAL`-Ausdruck nur mit
doppelt genauem Literal übereinstimmt.

## Geschichte

| Datum | Ereignis | Verweis |
|---|---|---|
| 2026-11-05 | Accepted | — |
