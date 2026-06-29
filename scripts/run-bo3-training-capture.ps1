param(
    [Parameter(Mandatory = $true)]
    [string]$Label,
    [string]$ProcessName = "BlackOps3",
    [int]$DurationSeconds = 180,
    [int]$DelaySeconds = 60,
    [int]$IntervalMilliseconds = 1000,
    [string]$OutputRoot,
    [string]$PresentMonPath,
    [string]$GameDir,
    [string]$Notes
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$logRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
if (-not $OutputRoot) {
    $OutputRoot = $logRoot
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Get-SafeLabel {
    param([string]$Value)

    $clean = $Value.Trim() -replace "[^A-Za-z0-9._-]+", "-"
    $clean = $clean.Trim("-")
    if (-not $clean) {
        return "bo3-capture"
    }
    return $clean.ToLowerInvariant()
}

function Invoke-PowerShellScript {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments
    )

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Script failed with exit code $LASTEXITCODE`: $ScriptPath"
    }
}

$processBaseName = [IO.Path]::GetFileNameWithoutExtension($ProcessName)
$presentMonProcessName = if ($ProcessName.ToLowerInvariant().EndsWith(".exe")) {
    $ProcessName
}
else {
    "$ProcessName.exe"
}

$proc = Get-Process -Name $processBaseName -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) {
    throw "Process not found: $processBaseName. Launch BO3 and reach the test scene before starting capture."
}

if (-not $GameDir -and $proc.Path) {
    $GameDir = Split-Path -Parent $proc.Path
}

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$runId = "$(Get-SafeLabel -Value $Label)-$stamp"

$displayPath = Join-Path $OutputRoot "$runId-display.json"
$processPath = Join-Path $OutputRoot "$runId-process.csv"
$presentPath = Join-Path $OutputRoot "$runId-presentmon.csv"
$processSummaryPath = Join-Path $OutputRoot "$runId-process-summary.md"
$presentSummaryPath = Join-Path $OutputRoot "$runId-presentmon-summary.md"
$manifestPath = Join-Path $OutputRoot "$runId-manifest.json"
$runSummaryPath = Join-Path $OutputRoot "$runId-run.md"
$overlayLogCopy = $null

Write-Host "Reversa training capture: $runId"
Write-Host "Process: $processBaseName ($($proc.Id))"
Write-Host "Delay: $DelaySeconds sec | Duration: $DurationSeconds sec"

Invoke-PowerShellScript `
    -ScriptPath (Join-Path $PSScriptRoot "capture-display-state.ps1") `
    -Arguments @("-OutputPath", $displayPath)

$processJob = Start-Job -Name "$runId-process" -ScriptBlock {
    param($ScriptPath, $Delay, $ProcessName, $Duration, $Interval, $OutputPath)

    if ($Delay -gt 0) {
        Start-Sleep -Seconds $Delay
    }

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $ScriptPath `
        -ProcessName $ProcessName `
        -DurationSeconds $Duration `
        -IntervalMilliseconds $Interval `
        -OutputPath $OutputPath

    if ($LASTEXITCODE -ne 0) {
        throw "Process telemetry capture failed with exit code $LASTEXITCODE"
    }
} -ArgumentList @(
    (Join-Path $PSScriptRoot "benchmark-bo3-process.ps1"),
    $DelaySeconds,
    $processBaseName,
    $DurationSeconds,
    $IntervalMilliseconds,
    $processPath
)

$presentArgs = @(
    "-ProcessName", $presentMonProcessName,
    "-DurationSeconds", [string]$DurationSeconds,
    "-DelaySeconds", [string]$DelaySeconds,
    "-OutputPath", $presentPath
)
if ($PresentMonPath) {
    $presentArgs += @("-PresentMonPath", $PresentMonPath)
}

$presentJob = Start-Job -Name "$runId-presentmon" -ScriptBlock {
    param($ScriptPath, $Arguments)

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "PresentMon capture failed with exit code $LASTEXITCODE"
    }
} -ArgumentList (Join-Path $PSScriptRoot "capture-bo3-presentmon.ps1"), (,$presentArgs)

$jobs = @($processJob, $presentJob)
try {
    Wait-Job -Job $jobs | Out-Null

    foreach ($job in $jobs) {
        $jobErrors = @()
        $output = Receive-Job -Job $job -ErrorAction SilentlyContinue -ErrorVariable jobErrors
        if ($output) {
            $output | ForEach-Object { Write-Host $_ }
        }
        if ($jobErrors) {
            $jobErrors | ForEach-Object { Write-Warning $_.ToString() }
        }

        if ($job.State -ne "Completed") {
            throw "Capture job failed: $($job.Name) state=$($job.State)"
        }
    }
}
finally {
    Remove-Job -Job $jobs -Force -ErrorAction SilentlyContinue
}

foreach ($path in @($displayPath, $processPath, $presentPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected artifact was not written: $path"
    }
}

Invoke-PowerShellScript `
    -ScriptPath (Join-Path $PSScriptRoot "summarize-bo3-process.ps1") `
    -Arguments @("-InputPath", $processPath, "-OutputPath", $processSummaryPath)

Invoke-PowerShellScript `
    -ScriptPath (Join-Path $PSScriptRoot "compare-bo3-presentmon.ps1") `
    -Arguments @("-InputPath", $presentPath, "-OutputPath", $presentSummaryPath)

if ($GameDir) {
    $overlayLog = Join-Path $GameDir "reversa-overlay.log"
    if (Test-Path -LiteralPath $overlayLog) {
        $overlayLogCopy = Join-Path $OutputRoot "$runId-reversa-overlay.log"
        Copy-Item -LiteralPath $overlayLog -Destination $overlayLogCopy -Force
    }
}

$manifest = [pscustomobject]([ordered]@{
    schema_version = 1
    run_id = $runId
    label = $Label
    generated_at = (Get-Date).ToString("o")
    process_name = $processBaseName
    process_id = $proc.Id
    game_dir = $GameDir
    duration_seconds = $DurationSeconds
    delay_seconds = $DelaySeconds
    interval_milliseconds = $IntervalMilliseconds
    notes = $Notes
    artifacts = [pscustomobject]([ordered]@{
        display_state = $displayPath
        process_csv = $processPath
        presentmon_csv = $presentPath
        process_summary = $processSummaryPath
        presentmon_summary = $presentSummaryPath
        overlay_log = $overlayLogCopy
    })
})

$manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding ascii

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# Reversa BO3 Training Capture")
$lines.Add("")
$lines.Add("- Run ID: ``$runId``")
$lines.Add("- Label: ``$Label``")
$lines.Add("- Generated: ``$($manifest.generated_at)``")
$lines.Add("- Process: ``$processBaseName`` PID ``$($proc.Id)``")
$lines.Add("- Duration: ``$DurationSeconds`` seconds after ``$DelaySeconds`` second delay")
if ($Notes) {
    $lines.Add("- Notes: $Notes")
}
$lines.Add("")
$lines.Add("## Artifacts")
$lines.Add("")
$lines.Add("- Display state: ``$displayPath``")
$lines.Add("- Process CSV: ``$processPath``")
$lines.Add("- Process summary: ``$processSummaryPath``")
$lines.Add("- PresentMon CSV: ``$presentPath``")
$lines.Add("- PresentMon summary: ``$presentSummaryPath``")
if ($overlayLogCopy) {
    $lines.Add("- Overlay log copy: ``$overlayLogCopy``")
}
$lines.Add("- Manifest: ``$manifestPath``")
$lines.Add("")
$lines.Add("## Rule")
$lines.Add("")
$lines.Add("Compare this run only against captures with the same map, route, resolution, render scale, cap, and shader-cache state.")

$lines | Set-Content -Path $runSummaryPath -Encoding ascii

Write-Host "Wrote $manifestPath"
Write-Host "Wrote $runSummaryPath"
