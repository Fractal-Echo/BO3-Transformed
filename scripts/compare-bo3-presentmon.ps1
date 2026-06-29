param(
    [Parameter(Mandatory = $true)]
    [string[]]$InputPath,
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

function Get-Quantile {
    param(
        [double[]]$SortedValues,
        [double]$Percentile
    )

    if (-not $SortedValues -or $SortedValues.Count -eq 0) {
        return $null
    }

    $index = [math]::Floor(($SortedValues.Count - 1) * $Percentile)
    $index = [math]::Max(0, [math]::Min($SortedValues.Count - 1, [int]$index))
    return [math]::Round($SortedValues[$index], 4)
}

function Format-NullableNumber {
    param($Value)

    if ($null -eq $Value) {
        return "NA"
    }

    return ([double]$Value).ToString("0.####", [Globalization.CultureInfo]::InvariantCulture)
}

function Get-PresentMonSummary {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Input does not exist: $Path"
    }

    $rows = Import-Csv -LiteralPath $Path
    $frameTimes = @(
        $rows |
            Where-Object { $_.FrameTime -and $_.FrameTime -ne "NA" } |
            ForEach-Object { [double]$_.FrameTime } |
            Sort-Object
    )

    if ($frameTimes.Count -eq 0) {
        throw "No frame-time rows found in $Path"
    }

    $avgMs = ($frameTimes | Measure-Object -Average).Average
    $presentModes = @(
        $rows |
            Where-Object { $_.PresentMode } |
            Group-Object PresentMode |
            Sort-Object Count -Descending |
            ForEach-Object { "$($_.Name)=$($_.Count)" }
    )

    $runtimes = @(
        $rows |
            Where-Object { $_.PresentRuntime } |
            Group-Object PresentRuntime |
            Sort-Object Count -Descending |
            ForEach-Object { "$($_.Name)=$($_.Count)" }
    )

    $pids = @(
        $rows |
            Where-Object { $_.ProcessID } |
            Select-Object -ExpandProperty ProcessID -Unique
    )

    [pscustomobject]@{
        Label = [IO.Path]::GetFileNameWithoutExtension($Path)
        Path = (Get-Item -LiteralPath $Path).FullName
        Rows = $rows.Count
        Frames = $frameTimes.Count
        AvgFrameMs = [math]::Round($avgMs, 4)
        AvgFps = [math]::Round(1000.0 / $avgMs, 2)
        P50Ms = Get-Quantile -SortedValues $frameTimes -Percentile 0.50
        P95Ms = Get-Quantile -SortedValues $frameTimes -Percentile 0.95
        P99Ms = Get-Quantile -SortedValues $frameTimes -Percentile 0.99
        P999Ms = Get-Quantile -SortedValues $frameTimes -Percentile 0.999
        MinMs = [math]::Round(($frameTimes | Measure-Object -Minimum).Minimum, 4)
        MaxMs = [math]::Round(($frameTimes | Measure-Object -Maximum).Maximum, 4)
        Over16_67Ms = @($frameTimes | Where-Object { $_ -gt 16.67 }).Count
        Over33_33Ms = @($frameTimes | Where-Object { $_ -gt 33.33 }).Count
        Over50Ms = @($frameTimes | Where-Object { $_ -gt 50.0 }).Count
        ProcessIds = ($pids -join ",")
        PresentRuntimes = ($runtimes -join "; ")
        PresentModes = ($presentModes -join "; ")
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

$InputPath = @($normalizedInputs)

$summaries = @()
foreach ($path in $InputPath) {
    $summaries += Get-PresentMonSummary -Path $path
}

$summaries |
    Select-Object Label,Frames,AvgFrameMs,AvgFps,P50Ms,P95Ms,P99Ms,P999Ms,MaxMs,Over16_67Ms,Over33_33Ms,Over50Ms |
    Format-Table -AutoSize

if (-not $OutputPath) {
    $repoRoot = Split-Path -Parent $PSScriptRoot
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $repoRoot "profiles\bo3-vulkan\logs\bo3-presentmon-summary-$stamp.md"
}

$base = $summaries[0]
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# BO3 PresentMon Summary")
$lines.Add("")
$lines.Add("Generated: $(Get-Date -Format o)")
$lines.Add("")
$lines.Add("| Capture | Frames | Avg ms | Avg FPS | P50 ms | P95 ms | P99 ms | P99.9 ms | Max ms | >16.67 ms | >33.33 ms | >50 ms |")
$lines.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
foreach ($summary in $summaries) {
    $lines.Add("| $($summary.Label) | $($summary.Frames) | $(Format-NullableNumber $summary.AvgFrameMs) | $(Format-NullableNumber $summary.AvgFps) | $(Format-NullableNumber $summary.P50Ms) | $(Format-NullableNumber $summary.P95Ms) | $(Format-NullableNumber $summary.P99Ms) | $(Format-NullableNumber $summary.P999Ms) | $(Format-NullableNumber $summary.MaxMs) | $($summary.Over16_67Ms) | $($summary.Over33_33Ms) | $($summary.Over50Ms) |")
}

if ($summaries.Count -gt 1) {
    $lines.Add("")
    $lines.Add("## Delta Versus First Capture")
    $lines.Add("")
    $lines.Add("| Capture | Avg ms delta | Avg FPS delta | P95 ms delta | P99 ms delta | Max ms delta |")
    $lines.Add("| --- | ---: | ---: | ---: | ---: | ---: |")
    foreach ($summary in $summaries | Select-Object -Skip 1) {
        $lines.Add("| $($summary.Label) | $(Format-NullableNumber ($summary.AvgFrameMs - $base.AvgFrameMs)) | $(Format-NullableNumber ($summary.AvgFps - $base.AvgFps)) | $(Format-NullableNumber ($summary.P95Ms - $base.P95Ms)) | $(Format-NullableNumber ($summary.P99Ms - $base.P99Ms)) | $(Format-NullableNumber ($summary.MaxMs - $base.MaxMs)) |")
    }
}

$lines.Add("")
$lines.Add("## Capture Metadata")
$lines.Add("")
foreach ($summary in $summaries) {
    $lines.Add("### $($summary.Label)")
    $lines.Add("")
    $lines.Add("- Path: ``$($summary.Path)``")
    $lines.Add("- Process IDs: ``$($summary.ProcessIds)``")
    $lines.Add("- Present runtimes: ``$($summary.PresentRuntimes)``")
    $lines.Add("- Present modes: ``$($summary.PresentModes)``")
    $lines.Add("")
}

$outDir = Split-Path -Parent $OutputPath
if ($outDir) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$lines | Set-Content -Path $OutputPath -Encoding ascii
Write-Host "Wrote $OutputPath"
