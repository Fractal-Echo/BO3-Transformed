param(
    [string]$ProcessName = "BlackOps3.exe",
    [int]$DurationSeconds = 180,
    [int]$DelaySeconds = 5,
    [string]$OutputPath,
    [string]$PresentMonPath
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$logRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $logRoot "bo3-presentmon-$stamp.csv"
}

function Resolve-PresentMon {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (-not (Test-Path -LiteralPath $ExplicitPath)) {
            throw "PresentMonPath does not exist: $ExplicitPath"
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $command = Get-Command presentmon.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidates = @(
        (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages\Intel.PresentMon.Console_Microsoft.Winget.Source_8wekyb3d8bbwe\presentmon.exe"),
        "C:\Program Files\NVIDIA Corporation\FrameViewSDK\bin\PresentMon_x64.exe",
        "C:\Program Files\AMD\CNext\CNext\PresentMon-x64.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "PresentMon was not found. Install Intel.PresentMon.Console with winget or pass -PresentMonPath."
}

$presentMon = Resolve-PresentMon -ExplicitPath $PresentMonPath

$proc = Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($ProcessName)) -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) {
    throw "Process not found: $ProcessName"
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Warning "PresentMon may need elevation to target processes started by another account."
}

if ($DelaySeconds -gt 0) {
    Write-Host "Waiting $DelaySeconds seconds before PresentMon capture..."
    Start-Sleep -Seconds $DelaySeconds
}

Write-Host "Capturing $ProcessName frame times for $DurationSeconds seconds..."
& $presentMon `
    --process_name $ProcessName `
    --output_file $OutputPath `
    --timed $DurationSeconds `
    --terminate_after_timed `
    --stop_existing_session `
    --exclude_dropped `
    --date_time `
    --v2_metrics `
    --no_console_stats

if (-not (Test-Path -LiteralPath $OutputPath)) {
    throw "PresentMon did not create output: $OutputPath"
}

$lineCount = (Get-Content -LiteralPath $OutputPath -ReadCount 1000 | ForEach-Object { $_.Count } | Measure-Object -Sum).Sum
if ($lineCount -lt 2) {
    throw "PresentMon output has no frame rows: $OutputPath"
}

Write-Host "Wrote $OutputPath"
