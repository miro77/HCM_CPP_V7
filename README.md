# HCM V7 — Fortran original and C++17 port

**HCMMS V7** implements the *Harmonized Calculation Method* for the land mobile
service (HCM Agreement): field strength prediction for point-to-point,
point-to-line (cross-border) and mobile-station cases, including terrain and
morphology profiles, antenna corrections, permissible field strength and
frequency-offset corrections.

This repository contains

- the **original Fortran code** (V7.21.5, unchanged) in [`fortran/`](fortran/), and
- a **C++17 port** with the same calling interface, the same error codes, info
  flags and text outputs, and numerical results that are verified against the
  original DLL.

| | Version | Language | Builds on |
|---|---|---|---|
| Original | 7.21.5 (2021-05-12) | Fortran 77/90 (Compaq Visual Fortran) | Windows, 32-bit DLL; gfortran via [`fortran/gfortran/`](fortran/gfortran/) |
| Port | 7.21.5 | C++17, standard library only | Windows (MSVC), Linux (GCC/Clang), anything with a C++17 compiler |

---

## Repository layout

```
.
├── include/hcmms/        Public headers (hcmms.hpp: API, state.hpp: calculation state)
├── src/                  Library implementation (one file per Fortran component)
├── app/                  Command-line tool `hcmms`
├── tests/                Unit tests, golden tests, synthetic test-data generator
│   └── golden/           Test cases and expected results (see its README)
├── tools/golden/         Case generator and script that runs cases through a Fortran DLL
├── scripts/              Build / test scripts for Windows (.ps1) and Linux (.sh)
├── fortran/              Original Fortran sources + changelog (README.md)
│   └── gfortran/         Build the Fortran code as a reference DLL with gfortran (Docker)
├── docs/
│   ├── original/         Original interface documentation (.doc)
│   ├── adr/              Design decisions of the port (German)
│   └── spec/             Requirements, specification, architecture (German)
├── CMakeLists.txt, CMakePresets.json, Dockerfile
└── LICENSE               GNU LGPL v2.1
```

## Quick start

Requirements: CMake ≥ 3.21 and a C++17 compiler. Ninja is used when available.

**Windows** (Visual Studio 2019 or newer with the C++ workload; the script loads
the developer environment itself):

```powershell
.\scripts\build.ps1            # build (Release) -> build\release\
.\scripts\test.ps1             # build + run all tests
.\scripts\test.ps1 -Preset debug -Clean
```

**Linux / macOS:**

```sh
./scripts/build.sh             # build (Release) -> build/release/
./scripts/test.sh              # build + run all tests
./scripts/test.sh debug --clean
```

**Docker** (Linux build, tests and clang-tidy, from either OS):

```sh
./scripts/docker.sh all        # or: .\scripts\docker.ps1 -Target all
```

Plain CMake works as well:

```sh
cmake --preset release && cmake --build --preset release && ctest --preset release
```

## Using the library

Link against the CMake target `hcmms::hcmms`, either via `add_subdirectory(...)`
or after `cmake --install` via `find_package(hcmms)`.

### C++ API

```cpp
#include <hcmms/hcmms.hpp>

hcmms::State state;     // one calculation state (= COMMON blocks of the original)
hcmms::Outputs out;
std::string i_str = ...; // >= 432 chars, layout as in docs/original/Interface to the HCMMSV7DLL.doc

hcmms::hcmms_v7(state, /*C_mode*/ 0, /*bor_dis*/ 0, /*PD*/ 0.1, i_str, out);
if (out.HCM_error == 0) {
    // out.Calculated_FS, out.Perm_FS, out.Prot_margin, ...
}
// i_str now also carries the text outputs (version, info flags, new coordinates).
```

Like the original, results depend on the state left by previous calls (cached
terrain tiles, last used values). Use one `State` per thread or per independent
calculation sequence.

### C interface

`HCMMS_V7_DLL(...)` in `hcmms.hpp` has the parameter list of the original DLL;
the length of `I_str` follows as the last argument (`size_t`). It uses one
process-wide state, so it is **not thread-safe**, the same as the original.

### Command-line tool

```sh
hcmms <case-file> [<data-directory>]
```

It calculates every case of a case file (format: [`tests/golden/README.md`](tests/golden/README.md))
and prints one result line per case. `${FX}` in the case file is replaced by
`<data-directory>`.

## Tests

| Test | What it checks |
|---|---|
| `fixtures_generate` | Generates synthetic terrain, morphology and border-line data and verifies their checksum |
| `fortran_io` | Emulation of Fortran formatted / list-directed READ and F4.1 WRITE |
| `interface` | C interface vs. C++ API; defined behaviour in edge cases |
| `golden_compare` | 249 cases (all calculation modes and error codes, state carried between calls) against expected results |
| `cli`, `cli_usage` | Command-line tool |

**How the expected results were produced.** The port was built against the
original Compaq Visual Fortran DLL (V7.21.2) and reproduces it on all golden
cases. The V7.21.3–V7.21.5 changes were ported afterwards and cross-checked with
gfortran builds of both Fortran versions. Details: [`tests/golden/README.md`](tests/golden/README.md).

> **Path length:** the interface passes data paths in fixed 63-character fields.
> The test data are generated in `<build-dir>/fx`, and configuration fails if
> that path is too long. Use a shorter build directory or set
> `-DHCMMS_FIXTURE_DIR=<short path>`.

## Fortran reference build

[`fortran/gfortran/`](fortran/gfortran/) builds the unmodified Fortran sources as
`HCMMS_V7_DLL.dll` (32 and 64 bit) with MinGW-w64 gfortran in Docker. A small
compatibility patch (`compat.patch`: file-name case of `INCLUDE`, `MODE=` →
`ACTION=`, `BITEST`/`IIDNNT` intrinsics, stdcall directive) is applied to a copy;
the sources in `fortran/` stay untouched.

```powershell
.\fortran\gfortran\build.ps1        # Windows
```
```sh
docker build -t hcmms-gfortran-mingw fortran/gfortran
docker run --rm -v "$PWD/fortran:/work" hcmms-gfortran-mingw sh /work/gfortran/build.sh
```

`tools/golden/Invoke-Hcmms.ps1` runs a case file through such a DLL (Windows only).

## Numerical fidelity

The port follows the original routine by routine. Variable names match the
Fortran, so the two can be compared line by line. Important rules, each
justified in [`docs/adr/`](docs/adr/):

- `REAL` values are stored as `float` and evaluated in `double`, like the
  original. FMA contraction is disabled, and x87 extended precision is avoided
  on 32-bit builds ([ADR-0002](docs/adr/0002-numerische-treue-float-speicher.md), [ADR-0003](docs/adr/0003-real-literale-in-double-ausdruecken.md)).
- The semantics of the Compaq Fortran runtime for formatted and list-directed
  READ are emulated, including its edge cases (checked against the original DLL).
- Undefined behaviour of the original (uninitialised locals, out-of-bounds array
  accesses) is replaced by defined, documented behaviour
  ([ADR-0004](docs/adr/0004-undefiniertes-verhalten-des-originals.md), [ADR-0006](docs/adr/0006-zugriffe-ausserhalb-der-feldgrenzen.md)).

## Known limitations

- The library builds as a static library only. There is no drop-in replacement
  DLL yet: that would need `__stdcall` and an export on 32-bit Windows.
- `DEBUG.TXT` output and the `HCM_LP` test mode of the original are not ported.
- Data paths are limited to 63 characters by the interface (as in the original).

## History of the Fortran code

See the changelog in [`fortran/README.md`](fortran/README.md). Upstream:
<https://github.com/5chufti/HCM_V7>.

## License

GNU Lesser General Public License, version 2.1 or (at your option) any later
version; see [`LICENSE`](LICENSE).

- Original HCM_V7 (Fortran): Copyright (C) 2004– Peter Benner and Gottfried Harasek.
- C++ port: Copyright (C) 2026 the authors of this repository (see git history).

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; see the GNU Lesser General Public License for details.
