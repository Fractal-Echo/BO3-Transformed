param(
    [Parameter(Mandatory = $true)]
    [string[]]$InputPath,
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

function Format-NullableNumber {
    param($Value)

    if ($null -eq $Value) {
        return "NA"
    }

    return ([double]$Value).ToString("0.##", [Globalization.CultureInfo]::InvariantCulture)
}

function Get-ProcessSummary {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Input does not exist: $Path"
    }

    $rows = Import-Csv -LiteralPath $Path
    if ($rows.Count -eq 0) {
        throw "No rows found in $Path"
    }

    $times = @($rows | Where-Object { $_.Time } | ForEach-Object { [datetime]$_.Time })
    $metrics = @(
        "CpuPercentOneCoreScale",
        "WorkingSetMB",
        "PrivateMemoryMB",
        "HandleCount",
        "Threads",
        "GpuUtilPercent",
        "GpuMemoryUtilPercent",
        "GpuMemoryUsedMB",
        "GpuTemperatureC",
        "GpuPowerW"
    )

    $metricStats = [ordered]@{}
    foreach ($metric in $metrics) {
        $values = @(
            $rows |
                Where-Object { $_.$metric -ne $null -and $_.$metric -ne "" } |
                ForEach-Object { [double]$_.$metric }
        )

        if ($values.Count -eq 0) {
            continue
        }

        $metricStats[$metric] = [pscustomobject]@{
            avg = [math]::Round(($values | Measure-Object -Average).Average, 2)
            min = [math]::Round(($values | Measure-Object -Minimum).Minimum, 2)
            max = [math]::Round(($values | Measure-Object -Maximum).Maximum, 2)
        }
    }

    $gpuNames = @($rows | Where-Object { $_.GpuName } | Select-Object -ExpandProperty GpuName -Unique)
    $gpuDrivers = @($rows | Where-Object { $_.GpuDriver } | Select-Object -ExpandProperty GpuDriver -Unique)
    $processIds = @($rows | Where-Object { $_.ProcessId } | Select-Object -ExpandProperty ProcessId -Unique)

    [pscustomobject]@{
        label = [IO.Path]::GetFileNameWithoutExtension($Path)
        path = (Get-Item -LiteralPath $Path).FullName
        rows = $rows.Count
        start = if ($times.Count) { $times[0].ToString("o") } else { $null }
        end = if ($times.Count) { $times[-1].ToString("o") } else { $null }
        duration_sec = if ($times.Count -gt 1) { [math]::Round(($times[-1] - $times[0]).TotalSeconds, 2) } else { $null }
        process_ids = ($processIds -join ",")
        gpu_names = ($gpuNames -join "; ")
        gpu_drivers = ($gpuDrivers -join "; ")
        metrics = $metricStats
    }
}

$normalizedInputs = New-Object System.Collections.Generic.List[string]
foreach ($path in $InputPath) {
    if ($path -match ",") {
        foreach ($part in ($path -split ",")) {
            $clean = $part.Trim().Trim("'").Trim('"')
            if ($clean) {
                $normalizedInputs.Add($clean)
            }
        }
    }
    elseif ($path) {
        $normalizedInputs.Add($path)
    }
}

$summaries = @()
foreach ($path in $normalizedInputs) {
    $summaries += Get-ProcessSummary -Path $path
}

$tableRows = foreach ($summary in $summaries) {
    [pscustomobject]@{
        Label = $summary.label
        Rows = $summary.rows
        DurationSec = $summary.duration_sec
        CpuAvg = $summary.metrics.CpuPercentOneCoreScale.avg
        WorkingSetAvgMB = $summary.metrics.WorkingSetMB.avg
        PrivateAvgMB = $summary.metrics.PrivateMemoryMB.avg
        ThreadsAvg = $summary.metrics.Threads.avg
        GpuUtilAvg = $summary.metrics.GpuUtilPercent.avg
        GpuPowerAvgW = $summary.metrics.GpuPowerW.avg
    }
}

$tableRows | Format-Table -AutoSize

if (-not $OutputPath) {
    $repoRoot = Split-Path -Parent $PSScriptRoot
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $repoRoot "profiles\bo3-vulkan\logs\bo3-process-summary-$stamp.md"
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# BO3 Process/GPU Summary")
$lines.Add("")
$lines.Add("Generated: $(Get-Date -Format o)")
$lines.Add("")
$lines.Add("| Capture | Rows | Duration sec | CPU avg | Working set avg MB | Private avg MB | Threads avg | GPU util avg | GPU power avg W |")
$lines.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
foreach ($summary in $summaries) {
    $lines.Add("| $($summary.label) | $($summary.rows) | $(Format-NullableNumber $summary.duration_sec) | $(Format-NullableNumber $summary.metrics.CpuPercentOneCoreScale.avg) | $(Format-NullableNumber $summary.metrics.WorkingSetMB.avg) | $(Format-NullableNumber $summary.metrics.PrivateMemoryMB.avg) | $(Format-NullableNumber $summary.metrics.Threads.avg) | $(Format-NullableNumber $summary.metrics.GpuUtilPercent.avg) | $(Format-NullableNumber $summary.metrics.GpuPowerW.avg) |")
}

$lines.Add("")
$lines.Add("## Capture Metadata")
$lines.Add("")
foreach ($summary in $summaries) {
    $lines.Add("### $($summary.label)")
    $lines.Add("")
    $lines.Add("- Path: ``$($summary.path)``")
    $lines.Add("- Process IDs: ``$($summary.process_ids)``")
    $lines.Add("- GPU names: ``$($summary.gpu_names)``")
    $lines.Add("- GPU drivers: ``$($summary.gpu_drivers)``")
    $lines.Add("")
    $lines.Add("| Metric | Avg | Min | Max |")
    $lines.Add("| --- | ---: | ---: | ---: |")
    foreach ($metric in $summary.metrics.Keys) {
        $stat = $summary.metrics[$metric]
        $lines.Add("| $metric | $(Format-NullableNumber $stat.avg) | $(Format-NullableNumber $stat.min) | $(Format-NullableNumber $stat.max) |")
    }
    $lines.Add("")
}

$outDir = Split-Path -Parent $OutputPath
if ($outDir) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$lines | Set-Content -Path $OutputPath -Encoding ascii
Write-Host "Wrote $OutputPath"
