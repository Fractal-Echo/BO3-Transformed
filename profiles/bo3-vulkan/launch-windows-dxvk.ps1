param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III",
    [string]$ExeName = "BlackOps3.exe",
    [switch]$NoDllDeploy
)

$ErrorActionPreference = "Stop"

$profileRoot = Split-Path -Parent $PSCommandPath
$repoRoot = Split-Path -Parent (Split-Path -Parent $profileRoot)
$dxvkBin = Join-Path $profileRoot "bin\x64"
$config = Join-Path $profileRoot "dxvk.conf"
$logs = Join-Path $GameDir "dxvk-logs"
$gameExe = Join-Path $GameDir $ExeName

if (-not (Test-Path $gameExe)) {
    throw "Game executable not found: $gameExe"
}

if (-not (Test-Path $config)) {
    throw "DXVK config not found: $config"
}

New-Item -ItemType Directory -Force -Path $logs | Out-Null

if (-not $NoDllDeploy) {
    foreach ($dll in @("d3d11.dll", "dxgi.dll")) {
        $source = Join-Path $dxvkBin $dll
        $target = Join-Path $GameDir $dll

        if (-not (Test-Path $source)) {
            throw "Missing $dll in $dxvkBin. Run scripts\fetch-dxvk-release.ps1 first."
        }

        if ((Test-Path $target) -and ((Get-FileHash $source).Hash -ne (Get-FileHash $target).Hash)) {
            $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
            Rename-Item -LiteralPath $target -NewName "$dll.bak-$stamp"
        }

        Copy-Item -LiteralPath $source -Destination $target -Force
    }
}

$env:DXVK_CONFIG_FILE = $config
$env:DXVK_LOG_PATH = $logs
$env:DXVK_HUD = "version,fps,frametimes,compiler"

Write-Host "Launching $gameExe"
Write-Host "DXVK_CONFIG_FILE=$env:DXVK_CONFIG_FILE"
Write-Host "DXVK_LOG_PATH=$env:DXVK_LOG_PATH"

Start-Process -FilePath $gameExe -WorkingDirectory $GameDir
