# Build the Fortran reference DLL (32 and/or 64 bit) on Windows using Docker
# (MinGW-w64 gfortran cross compilers in a Debian container).
#
#   .\fortran\gfortran\build.ps1 [-Arch 32|64|all]
#
# Output: fortran\gfortran\out\win32\HCMMS_V7_DLL.dll, ...\win64\HCMMS_V7_DLL.dll
# Linux/macOS: see build.sh (same steps with "docker run").
param([ValidateSet('32', '64', 'all')] [string] $Arch = 'all')
$ErrorActionPreference = 'Stop'
$image = 'hcmms-gfortran-mingw'
$fortran = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

docker build -q -t $image $PSScriptRoot | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'docker build failed' }

docker run --rm -v "${fortran}:/work" $image sh /work/gfortran/build.sh $Arch
if ($LASTEXITCODE -ne 0) { throw 'gfortran build failed' }
