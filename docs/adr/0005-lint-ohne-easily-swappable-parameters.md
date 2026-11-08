# ADR-0005: Lint ohne `bugprone-easily-swappable-parameters`

**Status:** Accepted
**Datum:** 2026-11-05
**Autor:** Michael Rohner
**Bezug:** [`LH-FA-01`](../spec/lastenheft.md#lh-fa-01--aufrufschnittstelle-wie-hcmms_v7_dll), [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz)
**Schärft:** [`spezifikation.md §7`](../spec/spezifikation.md#7-festlegungen-der-harness-werkzeuge)
**Regeln:** ADR-Format MADR; Quality-Gates werden nur per ADR gelockert.

---

## Kontext

Der Lint-Gate aktiviert `bugprone-*` mit `--warnings-as-errors`. Der Check
`bugprone-easily-swappable-parameters` meldet jede Funktion mit benachbarten
Parametern gleichen oder konvertierbaren Typs. Nach Ausdehnung des Lint-Gates
auf alle Quellen meldet er 35 Stellen:

- Die C-Schnittstelle `HCMMS_V7_DLL` hat 32 Zeigerparameter in der Reihenfolge
  des Originals. Diese Signatur ist durch [`LH-FA-01`](../spec/lastenheft.md#lh-fa-01--aufrufschnittstelle-wie-hcmms_v7_dll)
  vorgegeben und darf sich nicht ändern.
- Die übrigen Funktionen bilden die Parameterlisten der Fortran-Routinen nach
  (`Antenna_correction`, `Get_figure_FS_value`, `Dh_correction` …), damit jede
  Funktion gegen ihr Original geprüft werden kann.

Inline-Unterdrückungen sind im Repo nicht erlaubt.

## Entscheidung

Wir wählen: **`bugprone-easily-swappable-parameters` wird in `.clang-tidy`
abgeschaltet**; alle übrigen `bugprone-*`- und `clang-analyzer-*`-Checks und die
Komplexitätsgrenze bleiben unverändert hart. Vertauschte Argumente fängt der
Golden-Vergleich: jede Berechnungsroutine liegt auf einem Pfad, den mindestens
ein Golden-Fall durchläuft.

## Verglichene Alternativen

| Option | Pro | Contra |
|---|---|---|
| A — Parameter in Strukturen bündeln | Check bleibt aktiv | C-Schnittstelle bleibt trotzdem rot; Funktionen weichen von den Originalsignaturen ab |
| B — Inline-Unterdrückung je Stelle | Check bleibt global aktiv | im Repo verboten |
| **C — Check abschalten, Golden-Vergleich als Ersatz** | ehrlich, eine Stelle | Schutz gegen Vertauschung nur dynamisch |

## Konsequenzen

- Positiv: Der Lint-Gate läuft über alle Quellen und bleibt sonst unverändert streng.
- Negativ: Vertauschte Argumente fallen erst im Test auf, nicht im Lint.

## Fitness Function (falls maschinell prüfbar)

| Tooling | Regel | Make-Target |
|---|---|---|
| clang-tidy | alle übrigen Checks aus `.clang-tidy`, `--warnings-as-errors='*'` über `src/` und `tests/` | `make lint` |
| ctest `golden_compare` | Toleranzen aus [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) | `make test` |

## Re-Evaluierungs-Trigger

Wenn neuer Code ohne Fortran-Vorbild entsteht, der nicht vom Golden-Vergleich
abgedeckt ist.

## Geschichte

| Datum | Ereignis | Verweis |
|---|---|---|
| 2026-11-05 | Accepted | — |
