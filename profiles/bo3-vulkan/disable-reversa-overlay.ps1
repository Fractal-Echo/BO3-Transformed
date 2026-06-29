param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
)

$ErrorActionPreference = "Stop"

$profileRoot = Split-Path -Parent $PSCommandPath
$overlayDll = Join-Path $profileRoot "bin\reversa-overlay\dxgi.dll"
$gameDxgi = Join-Path $GameDir "dxgi.dll"
$gameSystemDxgi = Join-Path $GameDir "dxgi_system.dll"
$disabledReversa = Join-Path $GameDir "dxgi.dll.reversa-disabled"

if (Get-Process -Name BlackOps3 -ErrorAction SilentlyContinue) {
    throw "BlackOps3.exe is running. Exit the game before disabling the Reversa overlay."
}

if (-not (Test-Path -LiteralPath $gameDxgi)) {
    Write-Host "No active dxgi.dll overlay is present in $GameDir"
}
else {
    if (-not (Test-Path -LiteralPath $overlayDll)) {
        throw "Cannot verify active dxgi.dll because the staged overlay DLL is missing: $overlayDll"
    }

    $gameHash = (Get-FileHash -LiteralPath $gameDxgi -Algorithm SHA256).Hash
    $overlayHash = (Get-FileHash -LiteralPath $overlayDll -Algorithm SHA256).Hash
    if ($gameHash -ne $overlayHash) {
        throw "$gameDxgi does not match the staged Reversa overlay DLL. Refusing to remove a file we did not deploy."
    }

    if (Test-Path -LiteralPath $disabledReversa) {
        Remove-Item -LiteralPath $disabledReversa -Force
    }
    Rename-Item -LiteralPath $gameDxgi -NewName ([IO.Path]::GetFileName($disabledReversa))
    Write-Host "Disabled Reversa overlay -> $disabledReversa"
}

if (Test-Path -LiteralPath $gameSystemDxgi) {
    Remove-Item -LiteralPath $gameSystemDxgi -Force
    Write-Host "Removed proxy dependency -> $gameSystemDxgi"
}
