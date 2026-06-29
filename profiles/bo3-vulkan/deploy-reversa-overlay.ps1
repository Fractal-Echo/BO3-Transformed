param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
)

$ErrorActionPreference = "Stop"

$profileRoot = Split-Path -Parent $PSCommandPath
$repoRoot = Split-Path -Parent (Split-Path -Parent $profileRoot)
$overlayRoot = Join-Path $profileRoot "bin\reversa-overlay"
$overlayDxgi = Join-Path $overlayRoot "dxgi.dll"
$overlayD3d11 = Join-Path $overlayRoot "d3d11.dll"
$systemDxgi = Join-Path $env:WINDIR "System32\dxgi.dll"
$systemD3d11 = Join-Path $env:WINDIR "System32\d3d11.dll"
$gameDxgi = Join-Path $GameDir "dxgi.dll"
$gameD3d11 = Join-Path $GameDir "d3d11.dll"
$gameSystemDxgi = Join-Path $GameDir "dxgi_system.dll"
$gameSystemD3d11 = Join-Path $GameDir "d3d11_system.dll"
$t7Internal = Join-Path $GameDir "T7InternalWS.dll"
$t7Bootstrapper = Join-Path $GameDir "T7WSBootstrapper.dll"
$stackManifest = Join-Path $GameDir "reversa-wrapper-stack.json"
$disabledDxvk = Join-Path $GameDir "dxgi.dll.dxvk-disabled"
$disabledDxvkD3d11 = Join-Path $GameDir "d3d11.dll.dxvk-disabled"
$disabledReversa = Join-Path $GameDir "dxgi.dll.reversa-disabled"
$disabledReversaD3d11 = Join-Path $GameDir "d3d11.dll.reversa-disabled"

if (Get-Process -Name BlackOps3 -ErrorAction SilentlyContinue) {
    throw "BlackOps3.exe is running. Exit the game before deploying the Reversa overlay."
}

foreach ($path in @($overlayDxgi, $overlayD3d11, $systemDxgi, $systemD3d11)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required file: $path"
    }
}

function Disable-ActiveDllIfNeeded {
    param(
        [string]$GameDll,
        [string]$OverlayDll,
        [string]$DxvkDll,
        [string]$DisabledDxvk,
        [string]$DisabledReversaDll,
        [string[]]$ReversaMarkers,
        [string]$Name
    )

    if (-not (Test-Path -LiteralPath $GameDll)) {
        return
    }

    $activeHash = (Get-FileHash -LiteralPath $GameDll -Algorithm SHA256).Hash
    $overlayHash = (Get-FileHash -LiteralPath $OverlayDll -Algorithm SHA256).Hash

    if ($activeHash -eq $overlayHash) {
        Write-Host "Reversa $Name proxy is already deployed."
        return
    }

    if ((Test-Path -LiteralPath $DxvkDll) -and $activeHash -eq (Get-FileHash -LiteralPath $DxvkDll -Algorithm SHA256).Hash) {
        if (Test-Path -LiteralPath $DisabledDxvk) {
            Remove-Item -LiteralPath $DisabledDxvk -Force
        }
        Rename-Item -LiteralPath $GameDll -NewName ([IO.Path]::GetFileName($DisabledDxvk))
        Write-Host "Disabled active DXVK $Name -> $DisabledDxvk"
    }
    elseif (Test-ReversaBinaryMarker -Path $GameDll -Markers $ReversaMarkers) {
        if (Test-Path -LiteralPath $DisabledReversaDll) {
            Remove-Item -LiteralPath $DisabledReversaDll -Force
        }
        Rename-Item -LiteralPath $GameDll -NewName ([IO.Path]::GetFileName($DisabledReversaDll))
        Write-Host "Rotated previous Reversa $Name -> $DisabledReversaDll"
    }
    else {
        throw "$GameDll exists and is not the staged Reversa proxy or staged DXVK DLL. Refusing to overwrite."
    }
}

function Test-ReversaBinaryMarker {
    param(
        [string]$Path,
        [string[]]$Markers
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }

    try {
        $text = [Text.Encoding]::Unicode.GetString([IO.File]::ReadAllBytes($Path))
        foreach ($marker in $Markers) {
            if ($text.Contains($marker)) {
                return $true
            }
        }
    }
    catch {
        return $false
    }

    return $false
}

function Get-StackFileFact {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [ordered]@{
            path = $Path
            exists = $false
        }
    }

    $item = Get-Item -LiteralPath $Path
    return [ordered]@{
        path = $item.FullName
        exists = $true
        bytes = $item.Length
        last_write_utc = $item.LastWriteTimeUtc.ToString("o")
        sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    }
}

Disable-ActiveDllIfNeeded `
    -GameDll $gameDxgi `
    -OverlayDll $overlayDxgi `
    -DxvkDll (Join-Path $profileRoot "bin\x64\dxgi.dll") `
    -DisabledDxvk $disabledDxvk `
    -DisabledReversaDll $disabledReversa `
    -ReversaMarkers @("REVERSA HUD", "DXGI proxy loaded") `
    -Name "dxgi.dll"

Disable-ActiveDllIfNeeded `
    -GameDll $gameD3d11 `
    -OverlayDll $overlayD3d11 `
    -DxvkDll (Join-Path $profileRoot "bin\x64\d3d11.dll") `
    -DisabledDxvk $disabledDxvkD3d11 `
    -DisabledReversaDll $disabledReversaD3d11 `
    -ReversaMarkers @("D3D11 proxy loaded", "D3D11CreateDevice") `
    -Name "d3d11.dll"

Copy-Item -LiteralPath $systemDxgi -Destination $gameSystemDxgi -Force
Copy-Item -LiteralPath $systemD3d11 -Destination $gameSystemD3d11 -Force
Copy-Item -LiteralPath $overlayDxgi -Destination $gameDxgi -Force
Copy-Item -LiteralPath $overlayD3d11 -Destination $gameD3d11 -Force

$t7Detected = (Test-Path -LiteralPath $t7Internal -PathType Leaf) -or (Test-Path -LiteralPath $t7Bootstrapper -PathType Leaf)
$manifest = [ordered]@{
    generated_at_utc = (Get-Date).ToUniversalTime().ToString("o")
    game_dir = (Resolve-Path -LiteralPath $GameDir).Path
    compatibility = [ordered]@{
        t7_detected = $t7Detected
        t7_contract = "T7 owns BO3 safety/network patches; Reversa owns graphics telemetry and overlay."
        d3d11_factory_hooks_default = "Disabled when T7 companion files are present."
        override_t7_compat = "REVERSA_T7_COMPAT=0 or 1"
        override_d3d11_factory_hooks = "REVERSA_D3D11_FACTORY_HOOKS=0 or 1"
    }
    files = [ordered]@{
        reversa_dxgi = Get-StackFileFact -Path $gameDxgi
        reversa_d3d11 = Get-StackFileFact -Path $gameD3d11
        dxgi_system = Get-StackFileFact -Path $gameSystemDxgi
        d3d11_system = Get-StackFileFact -Path $gameSystemD3d11
        t7_internal = Get-StackFileFact -Path $t7Internal
        t7_bootstrapper = Get-StackFileFact -Path $t7Bootstrapper
    }
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $stackManifest -Encoding ASCII

Write-Host "Deployed Reversa overlay/proxy:"
Write-Host "  $gameDxgi"
Write-Host "  $gameD3d11"
Write-Host "  $gameSystemDxgi"
Write-Host "  $gameSystemD3d11"
Write-Host "  $stackManifest"
if ($t7Detected) {
    Write-Host "T7 companion detected. Reversa will default to T7 compatibility mode and skip D3D11 factory vtable hooks."
}
else {
    Write-Host "T7 companion not detected. Reversa will allow its D3D11 factory hook path unless overridden."
}
Write-Host "Disable with profiles\bo3-vulkan\disable-reversa-overlay.ps1"
