# ADR-0006: Zugriffe außerhalb der Feldgrenzen des Originals

**Status:** Accepted
**Datum:** 2026-11-05
**Autor:** Michael Rohner
**Bezug:** [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz), [`LH-FA-03`](../spec/lastenheft.md#lh-fa-03--punkt-zu-punkt-berechnung), [`LH-FA-04`](../spec/lastenheft.md#lh-fa-04--linien--und-grenzbereichsberechnung)
**Schärft:** [`LH-FA-06.a`](../spec/spezifikation.md#lh-fa-06a--zustandsmodell)
**Regeln:** ADR-Format MADR.

---

## Kontext

Das Original greift an einigen Stellen außerhalb deklarierter Felder zu.
Fortran prüft keine Feldgrenzen; was gelesen oder überschrieben wird, folgt aus
der Speicherlage. Das unabhängige Review der Welle hat diese Stellen benannt:

1. **Rx-Freiwinkel liest `T_Prof(0)`**, wenn `NINT(16/PD)` gleich der
   Profillänge ist. `T_Prof` folgt im COMMON-Block `/NDATA/` direkt auf das
   `LOGICAL*4` `flp`; `T_Prof(0)` ist dessen obere Hälfte, also −1 nach einer
   Linienberechnung (`.TRUE.` = −1) und sonst 0. Das ist deterministisch.
2. **Profil mit mehr als 10002 Punkten** (kleiner Punktabstand über große
   Entfernung) schreibt über `T_Prof`/`M_Prof` hinaus. Probe: Punktabstand
   0,03 km über 361 km lässt die Referenz-DLL den aufrufenden Prozess mit einer
   Speicherverletzung abbrechen; 0,03 km über 299 km und 0,05 km über 361 km
   (knapp unter der Grenze) rechnet sie normal.
3. `Dh_calculation` liest `HI(0)`, wenn kein Profilpunkt im Auswertebereich liegt.
4. `Line_calculation` speichert (bis V7.21.3) den 1201. Datensatz einer
   Grenzdatei außerhalb von `N_File(26400)`. Seit V7.21.4 liest das Original
   genau 1400 Datensätze in `N_File(30800)`; der Fall ist damit entfallen.
5. `New_coordinates` erzeugt für eine Station am Pol NaN.

## Entscheidung

Wir wählen: **Deterministische Speicherlage wird nachgebildet, alles andere
wird definiert ersetzt.**

- Fall 1 wird nachgebildet: `T_Prof(0)` trägt −1, solange `flp` gesetzt ist, sonst 0.
- Fall 2: Die Profil-Felder wachsen mit der Profillänge; die Nachbildung
  rechnet das Profil vollständig. Das Original liefert dort kein Ergebnis.
- Fall 3: Ohne Profilpunkt im Auswertebereich behält die Geländeunregelmäßigkeit `Dh` ihren Startwert 50 m aus dem Rücksetzen der Punkt-zu-Punkt-Rechnung.
- Fall 4: Datensätze jenseits des 1400. werden nicht gelesen (wie V7.21.4+).
- Fall 5: Ein NaN aus der Kreispunkt-Berechnung wird auf den Definitionsbereich geklemmt.

## Verglichene Alternativen

| Option | Pro | Contra |
|---|---|---|
| A — gesamten COMMON-Block als ein Byte-Feld nachbilden | jeder Überlauf wie im Original | Fall 2 bricht im Original den Prozess ab, das ist nicht nachbildenswert; Code wird unlesbar |
| B — alles definiert ersetzen, auch Fall 1 | einheitlich | Fall 1 ist reproduzierbar und wäre eine vermeidbare Abweichung |
| **C — Deterministisches nachbilden, Rest definiert** | treu, wo das Original ein Ergebnis hat | Fälle 2–5 weichen bewusst ab |

## Konsequenzen

- Positiv: Kein Absturz, keine stillen Folgefehler in späteren Aufrufen.
- Negativ: In den Fällen 2–5 gibt es keinen Referenzwert; die Nachbildung liefert ihr eigenes, definiertes Ergebnis.
- Fall 1 verändert den Freiwinkel höchstens im Bereich von Tausendstel Grad und liegt damit unter der Toleranz; der Golden-Datensatz kann ihn nicht belegen.

## Fitness Function (falls maschinell prüfbar)

| Tooling | Regel | Make-Target |
|---|---|---|
| ctest `golden_compare` | Fälle `long_pd003_299km`, `long_pd005_361km`, `long_pd010_361km` knapp unter der Feldgrenze | `make test` |

## Re-Evaluierungs-Trigger

Wenn ein Anwender einen Referenzwert für einen der Fälle 2–5 vorlegt.

## Geschichte

| Datum | Ereignis | Verweis |
|---|---|---|
| 2026-11-05 | Accepted | — |
