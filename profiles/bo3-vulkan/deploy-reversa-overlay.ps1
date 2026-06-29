param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
)

$ErrorActionPreference = "Stop"

$profileRoot = Split-Path -Parent $PSCommandPath
$repoRoot = Split-Path -Parent (Split-Path -Parent $profileRoot)
$overlayRoot = Join-Path $profileRoot "bin\reversa-overlay"
$overlayDll = Join-Path $overlayRoot "dxgi.dll"
$systemDxgi = Join-Path $env:WINDIR "System32\dxgi.dll"
$gameDxgi = Join-Path $GameDir "dxgi.dll"
$gameSystemDxgi = Join-Path $GameDir "dxgi_system.dll"
$disabledDxvk = Join-Path $GameDir "dxgi.dll.dxvk-disabled"
$disabledReversa = Join-Path $GameDir "dxgi.dll.reversa-disabled"

if (Get-Process -Name BlackOps3 -ErrorAction SilentlyContinue) {
    throw "BlackOps3.exe is running. Exit the game before deploying the Reversa overlay."
}

foreach ($path in @($overlayDll, $systemDxgi)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required file: $path"
    }
}

if (Test-Path -LiteralPath $gameDxgi) {
    $activeHash = (Get-FileHash -LiteralPath $gameDxgi -Algorithm SHA256).Hash
    $overlayHash = (Get-FileHash -LiteralPath $overlayDll -Algorithm SHA256).Hash

    if ($activeHash -eq $overlayHash) {
        Write-Host "Reversa overlay is already deployed."
        exit 0
    }

    $dxvkDll = Join-Path $profileRoot "bin\x64\dxgi.dll"
    if ((Test-Path -LiteralPath $dxvkDll) -and $activeHash -eq (Get-FileHash -LiteralPath $dxvkDll -Algorithm SHA256).Hash) {
        if (Test-Path -LiteralPath $disabledDxvk) {
            Remove-Item -LiteralPath $disabledDxvk -Force
        }
        Rename-Item -LiteralPath $gameDxgi -NewName ([IO.Path]::GetFileName($disabledDxvk))
        Write-Host "Disabled active DXVK dxgi.dll -> $disabledDxvk"
    }
    else {
        throw "$gameDxgi exists and is not the staged Reversa overlay or staged DXVK DLL. Refusing to overwrite."
    }
}

if (Test-Path -LiteralPath $disabledReversa) {
    Remove-Item -LiteralPath $disabledReversa -Force
}

Copy-Item -LiteralPath $systemDxgi -Destination $gameSystemDxgi -Force
Copy-Item -LiteralPath $overlayDll -Destination $gameDxgi -Force

Write-Host "Deployed Reversa overlay:"
Write-Host "  $gameDxgi"
Write-Host "  $gameSystemDxgi"
Write-Host "Disable with profiles\bo3-vulkan\disable-reversa-overlay.ps1"
