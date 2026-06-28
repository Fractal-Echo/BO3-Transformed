param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
)

$ErrorActionPreference = "Stop"

$profileRoot = Split-Path -Parent $PSCommandPath
$dxvkBin = Join-Path $profileRoot "bin\x64"

foreach ($dll in @("d3d11.dll", "dxgi.dll")) {
    $gameDll = Join-Path $GameDir $dll
    $profileDll = Join-Path $dxvkBin $dll

    if (-not (Test-Path $gameDll)) {
        Write-Host "$dll is not present in $GameDir"
        continue
    }

    if (-not (Test-Path $profileDll)) {
        throw "Cannot verify $dll because profile DLL is missing: $profileDll"
    }

    $gameHash = (Get-FileHash -LiteralPath $gameDll).Hash
    $profileHash = (Get-FileHash -LiteralPath $profileDll).Hash

    if ($gameHash -ne $profileHash) {
        throw "$gameDll does not match the staged DXVK DLL. Refusing to remove a file we did not deploy."
    }

    $disabled = "$gameDll.dxvk-disabled"
    if (Test-Path $disabled) {
        Remove-Item -LiteralPath $disabled -Force
    }

    Rename-Item -LiteralPath $gameDll -NewName ([IO.Path]::GetFileName($disabled))
    Write-Host "Disabled $dll -> $disabled"
}

