# ADR-0002: Numerische Treue über float-Speicher und double-Auswertung

**Status:** Accepted
**Datum:** 2026-11-05
**Autor:** Michael Rohner
**Bezug:** [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz)
**Schärft:** [`LH-FA-03.a`](../spec/spezifikation.md#lh-fa-03a--numerische-regeln)
**Regeln:** ADR-Format MADR.

---

## Kontext

Das Original mischt `REAL` (einfache) und `DOUBLE PRECISION`. Compaq Visual
Fortran rechnet auf der x87-Einheit mit 53-Bit-Mantisse und rundet beim
Speichern in eine `REAL`-Variable. Rechnet die Nachbildung alles in `double`,
weichen Schwellwert-Vergleiche (Fresnelzone, Info-Flags, Tabellen-Intervalle)
in Grenzfällen ab, und damit Fehlercodes und Flags, die exakt gleich sein müssen.

## Entscheidung

Wir wählen **`float` für jede im Original als `REAL` deklarierte Variable und
Tabelle, Auswertung jedes Ausdrucks in `double`, Rundung auf `float` bei der
Zuweisung**; `REAL`-Literale und `REAL`-Intrinsics gehen mit ihrem `float`-Wert
ein. Der Compiler darf keine Kontraktion zu FMA vornehmen (`-ffp-contract=off`).

## Verglichene Alternativen

| Option | Pro | Contra |
|---|---|---|
| A — nichts tun, alles `double` | einfacher Code | Grenzfälle kippen, Flags weichen ab |
| B — `long double` wie x87 80 Bit | näher an x87 | CVF rechnet mit 53 Bit; auf MSVC ist `long double` = `double` |
| **C — `float`-Speicher, `double`-Auswertung** | bildet die x87-Semantik mit 53 Bit nach | jede Zuweisung braucht Sorgfalt |

## Konsequenzen

- Positiv: Abweichungen bleiben im Bereich der letzten Stellen.
- Negativ: Der Code liest sich weniger natürlich (explizite Rundungen).
- Folgepflicht: Compiler-Option im `CMakeLists.txt`.

## Fitness Function (falls maschinell prüfbar)

| Tooling | Regel | Make-Target |
|---|---|---|
| ctest `golden_compare` | Toleranzen aus [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) | `make test` |

## Re-Evaluierungs-Trigger

Wenn der Golden-Vergleich Abweichungen zeigt, die sich nur mit 80-Bit-Auswertung
schließen lassen.

## Geschichte

| Datum | Ereignis | Verweis |
|---|---|---|
| 2026-11-05 | Accepted | — |
