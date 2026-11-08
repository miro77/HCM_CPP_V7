# Build and run all tests (Windows, MSVC).
#
#   .\scripts\test.ps1 [-Preset release|debug] [-Clean]
param(
    [ValidateSet('release', 'debug')] [string] $Preset = 'release',
    [switch] $Clean
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build.ps1') -Preset $Preset -Clean:$Clean

Push-Location (Join-Path $PSScriptRoot '..')
try {
    ctest --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
} finally {
    Pop-Location
}
