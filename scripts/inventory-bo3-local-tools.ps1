param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III",
    [string]$LosslessScalingDir = "D:\SteamLibrary\steamapps\common\Lossless Scaling",
    [string]$WrappersDir = "D:\Applications\Wrappers and Injectors",
    [string]$DlssUpdaterDir = "D:\Gaming PC\DLSS.Updater.2.8.0",
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$logRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $logRoot "bo3-local-tool-inventory-$stamp.json"
}

function Get-HashOrNull {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash
}

function Get-FileFact {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [pscustomobject]@{
            path = $Path
            exists = $false
        }
    }

    $item = Get-Item -LiteralPath $Path
    $version = $null
    try {
        $versionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($item.FullName)
        $version = $versionInfo.FileVersion
    }
    catch {
        $version = $null
    }

    [pscustomobject]@{
        path = $item.FullName
        exists = $true
        length = $item.Length
        last_write_time = $item.LastWriteTime.ToString("o")
        sha256 = Get-HashOrNull -Path $item.FullName
        file_version = $version
    }
}

function Get-PresentMonCandidates {
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages\Intel.PresentMon.Console_Microsoft.Winget.Source_8wekyb3d8bbwe\presentmon.exe"),
        "C:\Program Files\NVIDIA Corporation\FrameViewSDK\bin\PresentMon_x64.exe",
        "C:\Program Files\AMD\CNext\CNext\PresentMon-x64.exe"
    )

    $command = Get-Command presentmon.exe -ErrorAction SilentlyContinue
    if ($command) {
        $candidates = @($command.Source) + $candidates
    }

    $candidates |
        Select-Object -Unique |
        ForEach-Object { Get-FileFact -Path $_ }
}

$specialKFiles = @()
$specialKRoot = Join-Path $WrappersDir "Special K Wrappers"
if (Test-Path -LiteralPath $specialKRoot) {
    $specialKFiles = @(
        Get-ChildItem -LiteralPath $specialKRoot -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -in @("SKIF.exe", "SpecialK64.dll", "SpecialK32.dll", "dxgi.dll", "d3d11.dll", "XInput1_4.dll") } |
            ForEach-Object { Get-FileFact -Path $_.FullName }
    )
}

$gameDlls = @(
    "d3d11.dll",
    "dxgi.dll",
    "d3d11.dll.dxvk-disabled",
    "dxgi.dll.dxvk-disabled",
    "nvngx_dlss.dll",
    "nvngx_dlssg.dll"
) | ForEach-Object { Get-FileFact -Path (Join-Path $GameDir $_) }

$manifest = [pscustomobject]@{
    schema_version = 1
    generated_at = (Get-Date).ToString("o")
    host = [pscustomobject]@{
        computer_name = $env:COMPUTERNAME
        user = $env:USERNAME
        powershell = $PSVersionTable.PSVersion.ToString()
    }
    game = [pscustomobject]@{
        dir = $GameDir
        blackops3 = Get-FileFact -Path (Join-Path $GameDir "BlackOps3.exe")
        wrapper_candidates = $gameDlls
    }
    tools = [pscustomobject]@{
        presentmon = @(Get-PresentMonCandidates)
        lossless_scaling = [pscustomobject]@{
            dir = $LosslessScalingDir
            executable = Get-FileFact -Path (Join-Path $LosslessScalingDir "LosslessScaling.exe")
            engine_dll = Get-FileFact -Path (Join-Path $LosslessScalingDir "Lossless.dll")
            config = Get-FileFact -Path (Join-Path $LosslessScalingDir "config.ini")
        }
        dlss_updater = [pscustomobject]@{
            dir = $DlssUpdaterDir
            executable = Get-FileFact -Path (Join-Path $DlssUpdaterDir "DLSS_Updater.exe")
        }
        wrappers = [pscustomobject]@{
            dir = $WrappersDir
            dxvk_async_legacy_dir_exists = (Test-Path -LiteralPath (Join-Path $WrappersDir "DXVKAsync.1.10.3  d3d9_10_11 to DXVK"))
            special_k = $specialKFiles
        }
        staged_dxvk = [pscustomobject]@{
            x64_d3d11 = Get-FileFact -Path (Join-Path $repoRoot "profiles\bo3-vulkan\bin\x64\d3d11.dll")
            x64_dxgi = Get-FileFact -Path (Join-Path $repoRoot "profiles\bo3-vulkan\bin\x64\dxgi.dll")
            x32_d3d11 = Get-FileFact -Path (Join-Path $repoRoot "profiles\bo3-vulkan\bin\x32\d3d11.dll")
            x32_dxgi = Get-FileFact -Path (Join-Path $repoRoot "profiles\bo3-vulkan\bin\x32\dxgi.dll")
        }
    }
}

$outDir = Split-Path -Parent $OutputPath
if ($outDir) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $OutputPath -Encoding ascii
Write-Host "Wrote $OutputPath"
