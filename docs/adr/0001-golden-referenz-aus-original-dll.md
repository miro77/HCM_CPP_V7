# ADR-0001: Golden-Referenz aus der Original-DLL mit synthetischen Testdaten

**Status:** Accepted
**Datum:** 2026-11-05
**Autor:** Michael Rohner
**Bezug:** [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz), [`LH-FA-03`](../spec/lastenheft.md#lh-fa-03--punkt-zu-punkt-berechnung), [`LH-FA-06`](../spec/lastenheft.md#lh-fa-06--zustand-über-aufrufe)
**Schärft:** [`spezifikation.md §7`](../spec/spezifikation.md#7-festlegungen-der-harness-werkzeuge), [`SPEC-004`](../spec/spezifikation.md#spec-004--ergebniszeile)
**Regeln:** ADR-Format MADR.

---

## Kontext

Die Nachbildung muss sich gegen das Verhalten der Referenz-DLL messen lassen.
Die DLL ist eine 32-Bit-Windows-DLL; das Test-Gate läuft in einem Linux-Container.
Echte HCM-Geländedaten liegen nicht im Repository und sind groß. Ohne Geländedaten
endet fast jeder Fall vor der eigentlichen Feldstärkerechnung mit Fehlercode 36.

## Entscheidung

Wir wählen **einen eingecheckten Golden-Datensatz, den die Original-DLL unter
Windows über deterministisch erzeugte, synthetische Gelände-, Morphologie- und
Grenzliniendaten rechnet**. Der Generator ist Teil des Repositorys und läuft
identisch unter Windows (Golden-Erzeugung) und im Container (Test-Gate). Die
Fälle laufen in einem Prozess nacheinander, damit der Zustand über Aufrufe
mitgeprüft wird.

## Verglichene Alternativen

| Option | Pro | Contra |
|---|---|---|
| A — nichts tun, nur Unit-Tests gegen Handrechnung | einfach | misst nicht die Treue zur Referenz |
| B — gfortran-Build als Orakel im Container | live, beliebig viele Fälle | gfortran ist nicht die Referenz; Abweichungen x87 gegen SSE2 |
| C — echte Geländedaten einchecken | realistisch | Größe, Lizenz der Daten, nicht im Repo verfügbar |
| **D — Golden der Original-DLL über synthetische Daten** | misst gegen die Referenz, Gate läuft netzlos | Golden muss unter Windows neu erzeugt werden, wenn Fälle dazukommen |

## Konsequenzen

- Positiv: Das Test-Gate prüft jede Ausgabe gegen die Referenz-DLL.
- Negativ: Neue Fälle brauchen einen Lauf unter Windows mit der DLL.
- Folgepflicht: Erzeugungsskript `tools/golden/` und Toleranzen nach [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz).

## Fitness Function (falls maschinell prüfbar)

| Tooling | Regel | Make-Target |
|---|---|---|
| ctest `golden_compare` | jede Zeile gleich bzw. innerhalb der Toleranz | `make test` |

## Re-Evaluierungs-Trigger

Wenn echte Geländedaten im Test-Gate verfügbar werden oder eine neue
Referenz-Version erscheint.

## Geschichte

| Datum | Ereignis | Verweis |
|---|---|---|
| 2026-11-05 | Accepted | — |
