# Configure and build the C++ library, CLI and tests (Windows, MSVC).
#
#   .\scripts\build.ps1 [-Preset release|debug] [-Clean]
#
# Loads the Visual Studio developer environment automatically and builds with
# Ninja (bundled with Visual Studio). Output: build\<preset>\
param(
    [ValidateSet('release', 'debug')] [string] $Preset = 'release',
    [switch] $Clean
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
. (Join-Path $PSScriptRoot 'VsEnv.ps1')
Enter-HcmmsVsEnv -Arch x64

Push-Location $root
try {
    $bin = Join-Path $root "build\$Preset"
    if ($Clean -and (Test-Path $bin)) { Remove-Item -Recurse -Force $bin }

    # The generator is fixed at the first configure; only pick one for a fresh tree.
    $gen = @()
    if (-not (Test-Path (Join-Path $bin 'CMakeCache.txt')) -and (Get-Command ninja -ErrorAction SilentlyContinue)) {
        $gen = @('-G', 'Ninja')
    }
    cmake --preset $Preset @gen
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    cmake --build --preset $Preset --parallel
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
} finally {
    Pop-Location
}
