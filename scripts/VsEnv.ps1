# Dot-source this file: loads the Visual Studio developer environment (cl, cmake,
# ninja) into the current PowerShell session if the MSVC compiler is not on PATH.
#
#   . "$PSScriptRoot\VsEnv.ps1"
#   Enter-HcmmsVsEnv -Arch x64

function Enter-HcmmsVsEnv {
    param([ValidateSet('x64', 'x86')] [string] $Arch = 'x64')

    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { return }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) {
        throw 'MSVC not found: install Visual Studio (or Build Tools) with the "Desktop development with C++" workload.'
    }
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'No Visual Studio installation with C++ tools found.' }

    # VsDevCmd calls vswhere itself and expects it on PATH.
    $env:PATH = "$(Split-Path $vswhere);$env:PATH"
    $devShell = Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    Import-Module $devShell
    Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=$Arch -host_arch=x64" | Out-Null
    Write-Host "Using Visual Studio at $vs ($Arch)"
}
