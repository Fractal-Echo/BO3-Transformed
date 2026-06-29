$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $repoRoot "tools\reversa-dxgi-overlay\src"
$outRoot = Join-Path $repoRoot "profiles\bo3-vulkan\bin\reversa-overlay"
$objRoot = Join-Path $repoRoot "tools\reversa-dxgi-overlay\build"
$vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vsWhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools 2022 with the C++ workload."
}

$installPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installPath) {
    throw "MSVC build tools were not found. Install Visual Studio Build Tools 2022 with the C++ workload."
}

$vcVars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcVars)) {
    throw "vcvars64.bat was not found under $installPath."
}

New-Item -ItemType Directory -Force -Path $outRoot, $objRoot | Out-Null

$source = Join-Path $sourceRoot "reversa_dxgi_overlay.cpp"
$def = Join-Path $sourceRoot "reversa_dxgi_overlay.def"
$importDef = Join-Path $sourceRoot "dxgi_system_import.def"
$importLib = Join-Path $objRoot "dxgi_system.lib"
$outDll = Join-Path $outRoot "dxgi.dll"
$outPdb = Join-Path $outRoot "reversa_dxgi_overlay.pdb"
$outLib = Join-Path $objRoot "reversa_dxgi_overlay.lib"
$obj = Join-Path $objRoot "reversa_dxgi_overlay.obj"

foreach ($path in @($source, $def, $importDef)) {
    if (-not (Test-Path $path)) {
        throw "Missing build input: $path"
    }
}

$command = @(
    "`"$vcVars`"",
    "&&",
    "lib.exe /nologo /machine:x64 /def:`"$importDef`" /out:`"$importLib`"",
    "&&",
    "cl.exe /nologo /EHsc /std:c++17 /W4 /WX /O2 /LD `"$source`" /Fo`"$obj`" /Fd`"$outPdb`" /link /NOLOGO /DEF:`"$def`" /OUT:`"$outDll`" /IMPLIB:`"$outLib`" `"$importLib`" user32.lib gdi32.lib psapi.lib"
) -join " "

Push-Location $env:TEMP
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}

Get-Item -LiteralPath $outDll
