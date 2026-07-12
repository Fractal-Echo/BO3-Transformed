$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$dxgiSourceRoot = Join-Path $repoRoot "tools\reversa-dxgi-overlay\src"
$d3d11SourceRoot = Join-Path $repoRoot "tools\reversa-d3d11-proxy\src"
$outRoot = Join-Path $repoRoot "profiles\bo3-vulkan\bin\reversa-overlay"
$dxgiObjRoot = Join-Path $repoRoot "tools\reversa-dxgi-overlay\build"
$d3d11ObjRoot = Join-Path $repoRoot "tools\reversa-d3d11-proxy\build"
$vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vsWhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools 2022 with the C++ workload."
}

$installPath = & $vsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installPath) {
    throw "MSVC build tools were not found. Install Visual Studio Build Tools 2022 with the C++ workload."
}

$vcVars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcVars)) {
    throw "vcvars64.bat was not found under $installPath."
}

New-Item -ItemType Directory -Force -Path $outRoot, $dxgiObjRoot, $d3d11ObjRoot | Out-Null

$dxgiSource = Join-Path $dxgiSourceRoot "reversa_dxgi_overlay.cpp"
$dxgiDef = Join-Path $dxgiSourceRoot "reversa_dxgi_overlay.def"
$dxgiImportDef = Join-Path $dxgiSourceRoot "dxgi_system_import.def"
$dxgiImportLib = Join-Path $dxgiObjRoot "dxgi_system.lib"
$dxgiOutDll = Join-Path $outRoot "dxgi.dll"
$dxgiOutPdb = Join-Path $outRoot "reversa_dxgi_overlay.pdb"
$dxgiOutLib = Join-Path $dxgiObjRoot "reversa_dxgi_overlay.lib"
$dxgiObj = Join-Path $dxgiObjRoot "reversa_dxgi_overlay.obj"

$d3d11Source = Join-Path $d3d11SourceRoot "reversa_d3d11_proxy.cpp"
$d3d11Def = Join-Path $d3d11SourceRoot "reversa_d3d11_proxy.def"
$d3d11ImportDef = Join-Path $d3d11SourceRoot "d3d11_system_import.def"
$d3d11ImportLib = Join-Path $d3d11ObjRoot "d3d11_system.lib"
$d3d11OutDll = Join-Path $outRoot "d3d11.dll"
$d3d11OutPdb = Join-Path $outRoot "reversa_d3d11_proxy.pdb"
$d3d11OutLib = Join-Path $d3d11ObjRoot "reversa_d3d11_proxy.lib"
$d3d11Obj = Join-Path $d3d11ObjRoot "reversa_d3d11_proxy.obj"

foreach ($path in @($dxgiSource, $dxgiDef, $dxgiImportDef, $d3d11Source, $d3d11Def, $d3d11ImportDef)) {
    if (-not (Test-Path $path)) {
        throw "Missing build input: $path"
    }
}

$command = @(
    "`"$vcVars`"",
    "&&",
    "lib.exe /nologo /machine:x64 /def:`"$dxgiImportDef`" /out:`"$dxgiImportLib`"",
    "&&",
    "cl.exe /nologo /EHsc /std:c++17 /W4 /WX /O2 /LD `"$dxgiSource`" /Fo`"$dxgiObj`" /Fd`"$dxgiOutPdb`" /link /NOLOGO /DEF:`"$dxgiDef`" /OUT:`"$dxgiOutDll`" /IMPLIB:`"$dxgiOutLib`" `"$dxgiImportLib`" user32.lib gdi32.lib psapi.lib",
    "&&",
    "lib.exe /nologo /machine:x64 /def:`"$d3d11ImportDef`" /out:`"$d3d11ImportLib`"",
    "&&",
    "cl.exe /nologo /EHsc /std:c++17 /W4 /WX /O2 /LD `"$d3d11Source`" /Fo`"$d3d11Obj`" /Fd`"$d3d11OutPdb`" /link /NOLOGO /DEF:`"$d3d11Def`" /OUT:`"$d3d11OutDll`" /IMPLIB:`"$d3d11OutLib`" `"$d3d11ImportLib`" user32.lib"
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

Get-Item -LiteralPath $dxgiOutDll, $d3d11OutDll
