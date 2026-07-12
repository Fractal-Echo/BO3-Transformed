$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repoRoot "ConsoleIX.vcxproj"
$vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vsWhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools 2022 with the C++ workload."
}

$installPath = & $vsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installPath) {
    throw "MSVC build tools were not found. Install Visual Studio Build Tools 2022 with the C++ workload."
}

$vcVars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
$msBuild = Join-Path $installPath "MSBuild\Current\Bin\amd64\MSBuild.exe"

if (-not (Test-Path $vcVars)) {
    throw "vcvars64.bat was not found under $installPath."
}

if (-not (Test-Path $msBuild)) {
    throw "MSBuild.exe was not found under $installPath."
}

$command = "`"$vcVars`" && `"$msBuild`" `"$project`" /p:Configuration=Release /p:Platform=x64 /m"
Push-Location $env:TEMP
try {
    & cmd.exe /d /s /c $command
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
