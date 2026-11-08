# ADR-Index — HCMMS C++

> **Derivativ:** Quelle der Wahrheit sind die ADR-Dateien; dieser Index ist
> eine Bequemlichkeits-Sicht.

| ID | Titel | Status | Bezug |
|---|---|---|---|
| [0001](0001-golden-referenz-aus-original-dll.md) | Golden-Referenz aus der Original-DLL mit synthetischen Testdaten | Accepted | [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |
| [0002](0002-numerische-treue-float-speicher.md) | Numerische Treue über float-Speicher und double-Auswertung | Accepted (Literal-Regel ersetzt durch 0003) | [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |
| [0003](0003-real-literale-in-double-ausdruecken.md) | REAL-Literale in DOUBLE-Ausdrücken mit doppelter Genauigkeit | Accepted | [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |
| [0004](0004-undefiniertes-verhalten-des-originals.md) | Undefiniertes Verhalten des Originals wird definiert ersetzt | Accepted | [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |
| [0005](0005-lint-ohne-easily-swappable-parameters.md) | Lint ohne bugprone-easily-swappable-parameters | Accepted | [`LH-FA-01`](../spec/lastenheft.md#lh-fa-01--aufrufschnittstelle-wie-hcmms_v7_dll) |
| [0006](0006-zugriffe-ausserhalb-der-feldgrenzen.md) | Zugriffe außerhalb der Feldgrenzen des Originals | Accepted | [`LH-QA-01`](../spec/lastenheft.md#lh-qa-01--genauigkeit-gegenüber-der-referenz) |

## Konventionen

- ADRs sind nach `Accepted` **immutable**.
- Schärfungen entstehen als neue ADR mit `Supersedes ADR-NNNN`.
