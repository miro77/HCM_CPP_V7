#!/bin/sh
# Build the original Fortran code as HCMMS_V7_DLL.dll with MinGW-w64 gfortran.
# The DLL serves as a reference for the C++ port (see README.md here).
#
#   ./build.sh [32|64|all]          (default: all)
#
# Runs in the Docker image from ./Dockerfile (see build.ps1 / docker command in
# README.md) or natively in MSYS2 (packages mingw-w64-i686-gcc-fortran /
# mingw-w64-x86_64-gcc-fortran) with FC32=/FC64= and NM32=/NM64= set.
#
# The sources in ../ stay untouched: they are copied to out/src, line endings
# normalised, and compat.patch (gfortran compatibility only) is applied there.
# Output: out/win32/HCMMS_V7_DLL.dll, out/win64/HCMMS_V7_DLL.dll
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
FORTRAN=$(cd "$HERE/.." && pwd)
OUT="$HERE/out"
SRC="$OUT/src"

rm -rf "$SRC" && mkdir -p "$SRC"
for f in "$FORTRAN"/*.f90 "$FORTRAN"/*.F90; do
    tr -d '\r' < "$f" > "$SRC/$(basename "$f")"
done
(cd "$SRC" && patch -s -p1 < "$HERE/compat.patch")

# CVF compatibility:
#   -fdec-math            DSIND/DCOSD/DATAN2D/... (degree trig functions)
#   -fno-automatic        CVF default: local variables are static (SAVEd, zero-initialised)
#   -ffree-line-length-none, -Wno-tabs   source layout of the original code
#   -fallow-argument-mismatch            no explicit interfaces in the code
FFLAGS="${FFLAGS:--O2} -fdec-math -fno-automatic -ffree-line-length-none -Wno-tabs -fallow-argument-mismatch -I$SRC"
LDFLAGS="-shared -static -static-libgcc -static-libgfortran -s"

# All sources except the INCLUDE file with the COMMON definitions.
SOURCES=$(cd "$SRC" && ls *.f90 *.F90 | grep -v '^HCM_MS_V7_definitions\.f90$')

build() {
    bits=$1 fc=$2 nm=$3 extra_f=$4 extra_l=$5
    obj="$OUT/obj$bits" dst="$OUT/win$bits"
    rm -rf "$obj" && mkdir -p "$obj" "$dst"
    echo "== $bits bit: $($fc --version | head -1)"

    for f in $SOURCES; do
        $fc $FFLAGS $extra_f -c "$SRC/$f" -o "$obj/${f%.*}.o"
    done

    # Export the entry point under the CVF name "HCMMS_V7_DLL".
    # gfortran names it hcmms_v7_dll_ (32 bit stdcall: _hcmms_v7_dll_@N).
    sym=$($nm -g "$obj/HCMMS_V7_DLL.o" | awk '$2=="T" && $3 ~ /hcmms_v7_dll_/ {print $3}')
    [ -n "$sym" ] || { echo "entry point not found in HCMMS_V7_DLL.o" >&2; exit 1; }
    [ "$bits" = 32 ] && sym=${sym#_}     # .def files use undecorated-prefix names on i686
    printf 'LIBRARY HCMMS_V7_DLL.dll\nEXPORTS\n    HCMMS_V7_DLL = %s\n' "$sym" > "$obj/HCMMS_V7_DLL.def"

    $fc $LDFLAGS $extra_l -o "$dst/HCMMS_V7_DLL.dll" "$obj"/*.o "$obj/HCMMS_V7_DLL.def" \
        -Wl,--out-implib,"$dst/libHCMMS_V7_DLL.dll.a"
    echo "   -> $dst/HCMMS_V7_DLL.dll"
}

target=${1:-all}
case $target in 32|64|all) ;; *) echo "usage: $0 [32|64|all]" >&2; exit 2;; esac

if [ "$target" != 64 ]; then
    build 32 "${FC32:-i686-w64-mingw32-gfortran}" "${NM32:-i686-w64-mingw32-nm}" \
          "-DHCM_STDCALL -cpp" "-Wl,--kill-at"
fi
if [ "$target" != 32 ]; then
    build 64 "${FC64:-x86_64-w64-mingw32-gfortran}" "${NM64:-x86_64-w64-mingw32-nm}" \
          "-cpp" ""
fi
