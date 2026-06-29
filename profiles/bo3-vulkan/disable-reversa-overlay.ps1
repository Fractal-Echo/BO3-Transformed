param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
)

$ErrorActionPreference = "Stop"

$profileRoot = Split-Path -Parent $PSCommandPath
$overlayDxgi = Join-Path $profileRoot "bin\reversa-overlay\dxgi.dll"
$overlayD3d11 = Join-Path $profileRoot "bin\reversa-overlay\d3d11.dll"
$gameDxgi = Join-Path $GameDir "dxgi.dll"
$gameD3d11 = Join-Path $GameDir "d3d11.dll"
$gameSystemDxgi = Join-Path $GameDir "dxgi_system.dll"
$gameSystemD3d11 = Join-Path $GameDir "d3d11_system.dll"
$disabledReversa = Join-Path $GameDir "dxgi.dll.reversa-disabled"
$disabledReversaD3d11 = Join-Path $GameDir "d3d11.dll.reversa-disabled"

if (Get-Process -Name BlackOps3 -ErrorAction SilentlyContinue) {
    throw "BlackOps3.exe is running. Exit the game before disabling the Reversa overlay."
}

function Disable-ReversaDll {
    param(
        [string]$GameDll,
        [string]$OverlayDll,
        [string]$DisabledDll,
        [string]$Name
    )

    if (-not (Test-Path -LiteralPath $GameDll)) {
        Write-Host "No active $Name Reversa proxy is present in $GameDir"
        return
    }

    if (-not (Test-Path -LiteralPath $OverlayDll)) {
        throw "Cannot verify active $Name because the staged Reversa DLL is missing: $OverlayDll"
    }

    $gameHash = (Get-FileHash -LiteralPath $GameDll -Algorithm SHA256).Hash
    $overlayHash = (Get-FileHash -LiteralPath $OverlayDll -Algorithm SHA256).Hash
    if ($gameHash -ne $overlayHash) {
        throw "$GameDll does not match the staged Reversa DLL. Refusing to remove a file we did not deploy."
    }

    if (Test-Path -LiteralPath $DisabledDll) {
        Remove-Item -LiteralPath $DisabledDll -Force
    }
    Rename-Item -LiteralPath $GameDll -NewName ([IO.Path]::GetFileName($DisabledDll))
    Write-Host "Disabled Reversa $Name -> $DisabledDll"
}

Disable-ReversaDll -GameDll $gameDxgi -OverlayDll $overlayDxgi -DisabledDll $disabledReversa -Name "dxgi.dll"
Disable-ReversaDll -GameDll $gameD3d11 -OverlayDll $overlayD3d11 -DisabledDll $disabledReversaD3d11 -Name "d3d11.dll"

foreach ($path in @($gameSystemDxgi, $gameSystemD3d11)) {
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Force
        Write-Host "Removed proxy dependency -> $path"
    }
}
