# ADR-0004: Undefiniertes Verhalten des Originals wird definiert ersetzt

**Status:** Accepted
**Datum:** 2026-11-05
**Autor:** Michael Rohner
**Bezug:** [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz), [`LH-FA-06`](../spec/lastenheft.md#lh-fa-06--zustand-über-aufrufe)
**Schärft:** [`LH-FA-02.a`](../spec/spezifikation.md#lh-fa-02a--lese-semantik-der-eingabefelder), [`LH-FA-06.a`](../spec/spezifikation.md#lh-fa-06a--zustandsmodell)
**Regeln:** ADR-Format MADR.

---

## Kontext

In der Berechnung der zulässigen Feldstärke liest das Original den Gewinn der
Empfangsantenne und die Depolarisationsdämpfung list-gesteuert in lokale
Variablen (`GANT`, `DPN`) ohne Anfangswert und ohne `SAVE`. Besteht das Feld
nur aus einem Nullwert (`/`, `,` oder `r*`), bleibt die Variable unverändert
und die DLL rechnet mit dem, was zufällig im Stack-Speicher steht. Proben
zeigen, je nach vorherigem Rechenweg, Werte wie 12500, 25000, 0 oder 3,94; die
zulässige Feldstärke wird dabei bis zu −24980 dBµV/m. Das ist weder
reproduzierbar noch fachlich sinnvoll.

## Entscheidung

Wir wählen: **Lokale Variablen ohne Anfangswert im Original beginnen in der
Nachbildung bei 0.** Ein reiner Nullwert im Gewinn- oder Depolarisationsfeld
ergibt damit 0 dB. Fälle, deren Ergebnis im Original vom Stack-Inhalt abhängt,
gehören nicht in den Golden-Datensatz.

## Verglichene Alternativen

| Option | Pro | Contra |
|---|---|---|
| A — Stack-Inhalt nachbilden | gleiche Zahlen in den beobachteten Fällen | hängt von Compiler-Registervergabe ab, nicht vollständig herleitbar |
| B — Wert über Aufrufe behalten | einfach | widerspricht der Probe (kein `SAVE`) |
| **C — definiert mit 0 beginnen** | reproduzierbar, fachlich neutral | weicht in diesen Fällen bewusst von der Referenz ab |

## Konsequenzen

- Positiv: Die Nachbildung ist deterministisch.
- Negativ: Bei einem reinen Nullwert im Gewinn- oder Depolarisationsfeld weicht das Ergebnis bewusst von der Referenz ab.
- Unit-Tests belegen das definierte Verhalten; der Golden-Datensatz enthält diese Eingaben nicht.

## Fitness Function (falls maschinell prüfbar)

| Tooling | Regel | Make-Target |
|---|---|---|
| ctest `fortran_io` | Nullwert lässt die Zielvariable unverändert | `make test` |

## Re-Evaluierungs-Trigger

Wenn sich ein weiterer Fall zeigt, in dem das Original von nicht initialisiertem
Speicher abhängt.

## Geschichte

| Datum | Ereignis | Verweis |
|---|---|---|
| 2026-11-05 | Accepted | — |
