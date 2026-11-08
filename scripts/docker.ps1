# Build and test (or lint) inside a Linux container (Docker Desktop).
#
#   .\scripts\docker.ps1 [-Target test|lint|all]     (default: test)
param([ValidateSet('test', 'lint', 'all')] [string] $Target = 'test')
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $targets = if ($Target -eq 'all') { @('test', 'lint') } else { @($Target) }
    foreach ($t in $targets) {
        docker build --target $t -t "hcmms:$t" .
        if ($LASTEXITCODE -ne 0) { throw "docker build --target $t failed" }
    }
} finally {
    Pop-Location
}
