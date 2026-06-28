param(
    [string]$ProcessName = "BlackOps3",
    [int]$DurationSeconds = 180,
    [int]$IntervalMilliseconds = 1000,
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$logRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $logRoot "bo3-process-$stamp.csv"
}

$samples = New-Object System.Collections.Generic.List[object]
$previous = $null
$end = (Get-Date).AddSeconds($DurationSeconds)
$nvidiaSmi = Get-Command nvidia-smi.exe -ErrorAction SilentlyContinue

Write-Host "Sampling $ProcessName for $DurationSeconds seconds..."

while ((Get-Date) -lt $end) {
    $proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
    $now = Get-Date
    $gpu = $null

    if ($nvidiaSmi) {
        $gpuLine = & $nvidiaSmi.Source --query-gpu=name,driver_version,utilization.gpu,utilization.memory,memory.used,memory.total,temperature.gpu,power.draw --format=csv,noheader,nounits 2>$null | Select-Object -First 1
        if ($gpuLine) {
            $gpuParts = $gpuLine -split ',\s*'
            if ($gpuParts.Count -ge 8) {
                $gpu = [pscustomobject]@{
                    Name = $gpuParts[0]
                    Driver = $gpuParts[1]
                    UtilPercent = $gpuParts[2]
                    MemoryUtilPercent = $gpuParts[3]
                    MemoryUsedMB = $gpuParts[4]
                    MemoryTotalMB = $gpuParts[5]
                    TemperatureC = $gpuParts[6]
                    PowerW = $gpuParts[7]
                }
            }
        }
    }

    if ($proc) {
        $cpuPercent = $null
        if ($previous) {
            $elapsed = ($now - $previous.Time).TotalSeconds
            $cpuDelta = $proc.CPU - $previous.CpuSeconds
            if ($elapsed -gt 0) {
                $cpuPercent = [math]::Round(($cpuDelta / $elapsed) * 100, 2)
            }
        }

        $samples.Add([pscustomobject]@{
            Time = $now.ToString("o")
            ProcessId = $proc.Id
            CpuPercentOneCoreScale = $cpuPercent
            WorkingSetMB = [math]::Round($proc.WorkingSet64 / 1MB, 2)
            PrivateMemoryMB = [math]::Round($proc.PrivateMemorySize64 / 1MB, 2)
            HandleCount = $proc.HandleCount
            Threads = $proc.Threads.Count
            GpuName = if ($gpu) { $gpu.Name } else { $null }
            GpuDriver = if ($gpu) { $gpu.Driver } else { $null }
            GpuUtilPercent = if ($gpu) { $gpu.UtilPercent } else { $null }
            GpuMemoryUtilPercent = if ($gpu) { $gpu.MemoryUtilPercent } else { $null }
            GpuMemoryUsedMB = if ($gpu) { $gpu.MemoryUsedMB } else { $null }
            GpuMemoryTotalMB = if ($gpu) { $gpu.MemoryTotalMB } else { $null }
            GpuTemperatureC = if ($gpu) { $gpu.TemperatureC } else { $null }
            GpuPowerW = if ($gpu) { $gpu.PowerW } else { $null }
        })

        $previous = [pscustomobject]@{
            Time = $now
            CpuSeconds = $proc.CPU
        }
    }
    else {
        $samples.Add([pscustomobject]@{
            Time = $now.ToString("o")
            ProcessId = $null
            CpuPercentOneCoreScale = $null
            WorkingSetMB = $null
            PrivateMemoryMB = $null
            HandleCount = $null
            Threads = $null
            GpuName = if ($gpu) { $gpu.Name } else { $null }
            GpuDriver = if ($gpu) { $gpu.Driver } else { $null }
            GpuUtilPercent = if ($gpu) { $gpu.UtilPercent } else { $null }
            GpuMemoryUtilPercent = if ($gpu) { $gpu.MemoryUtilPercent } else { $null }
            GpuMemoryUsedMB = if ($gpu) { $gpu.MemoryUsedMB } else { $null }
            GpuMemoryTotalMB = if ($gpu) { $gpu.MemoryTotalMB } else { $null }
            GpuTemperatureC = if ($gpu) { $gpu.TemperatureC } else { $null }
            GpuPowerW = if ($gpu) { $gpu.PowerW } else { $null }
        })
    }

    Start-Sleep -Milliseconds $IntervalMilliseconds
}

$samples | Export-Csv -NoTypeInformation -Path $OutputPath
Write-Host "Wrote $OutputPath"
