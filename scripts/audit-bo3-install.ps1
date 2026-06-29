param(
    [string]$GameDir = "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III",
    [string]$OutputRoot,
    [switch]$HashAllRootDlls
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$jsonPath = Join-Path $OutputRoot "bo3-install-audit-$stamp.json"
$markdownPath = Join-Path $OutputRoot "bo3-install-audit-$stamp.md"

function Get-HashOrNull {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    try {
        return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash
    }
    catch {
        return $null
    }
}

function Get-SignatureFact {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    try {
        $sig = Get-AuthenticodeSignature -LiteralPath $Path
        return [pscustomobject]([ordered]@{
            status = $sig.Status.ToString()
            signer = if ($sig.SignerCertificate) { $sig.SignerCertificate.Subject } else { $null }
            issuer = if ($sig.SignerCertificate) { $sig.SignerCertificate.Issuer } else { $null }
            thumbprint = if ($sig.SignerCertificate) { $sig.SignerCertificate.Thumbprint } else { $null }
        })
    }
    catch {
        return [pscustomobject]([ordered]@{
            status = "Unreadable"
            error = $_.Exception.Message
        })
    }
}

function Get-PeFact {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    $stream = $null
    $reader = $null
    try {
        $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        $reader = [IO.BinaryReader]::new($stream)

        if ($reader.ReadUInt16() -ne 0x5A4D) {
            return [pscustomobject]([ordered]@{ valid = $false; reason = "Missing MZ header" })
        }

        $stream.Seek(0x3C, [IO.SeekOrigin]::Begin) | Out-Null
        $peOffset = $reader.ReadUInt32()
        if ($peOffset -gt ($stream.Length - 24)) {
            return [pscustomobject]([ordered]@{ valid = $false; reason = "Invalid PE header offset"; pe_offset = $peOffset })
        }

        $stream.Seek($peOffset, [IO.SeekOrigin]::Begin) | Out-Null
        if ($reader.ReadUInt32() -ne 0x00004550) {
            return [pscustomobject]([ordered]@{ valid = $false; reason = "Missing PE signature"; pe_offset = $peOffset })
        }

        $machine = $reader.ReadUInt16()
        $sections = $reader.ReadUInt16()
        $timeDateStamp = $reader.ReadUInt32()
        $stream.Seek(8, [IO.SeekOrigin]::Current) | Out-Null
        $optionalHeaderSize = $reader.ReadUInt16()
        $characteristics = $reader.ReadUInt16()

        $optionalHeaderStart = $stream.Position
        $optionalMagic = $reader.ReadUInt16()
        $stream.Seek($optionalHeaderStart + 16, [IO.SeekOrigin]::Begin) | Out-Null
        $entryPoint = $reader.ReadUInt32()
        $stream.Seek($optionalHeaderStart + 56, [IO.SeekOrigin]::Begin) | Out-Null
        $sizeOfImage = $reader.ReadUInt32()
        $stream.Seek($optionalHeaderStart + 68, [IO.SeekOrigin]::Begin) | Out-Null
        $subsystem = $reader.ReadUInt16()
        $dllCharacteristics = $reader.ReadUInt16()

        $machineName = switch ($machine) {
            0x014c { "x86" }
            0x8664 { "x64" }
            0xAA64 { "arm64" }
            default { "0x{0:X4}" -f $machine }
        }

        $subsystemName = switch ($subsystem) {
            2 { "Windows GUI" }
            3 { "Windows CUI" }
            default { "0x{0:X4}" -f $subsystem }
        }

        $timestampUtc = $null
        try {
            $timestampUtc = ([DateTimeOffset]::FromUnixTimeSeconds([int64]$timeDateStamp)).UtcDateTime.ToString("o")
        }
        catch {
            $timestampUtc = $null
        }

        return [pscustomobject]([ordered]@{
            valid = $true
            pe_offset = $peOffset
            machine = $machineName
            sections = $sections
            linker_timestamp_utc = $timestampUtc
            optional_header_magic = ("0x{0:X4}" -f $optionalMagic)
            optional_header_size = $optionalHeaderSize
            subsystem = $subsystemName
            entry_point_rva = ("0x{0:X8}" -f $entryPoint)
            size_of_image = $sizeOfImage
            characteristics = ("0x{0:X4}" -f $characteristics)
            dll_characteristics = ("0x{0:X4}" -f $dllCharacteristics)
        })
    }
    catch {
        return [pscustomobject]([ordered]@{
            valid = $false
            reason = $_.Exception.Message
        })
    }
    finally {
        if ($reader) { $reader.Dispose() }
        if ($stream) { $stream.Dispose() }
    }
}

function Get-FileFact {
    param(
        [string]$Path,
        [switch]$IncludeHash,
        [switch]$IncludePe,
        [switch]$IncludeSignature
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [pscustomobject]([ordered]@{
            path = $Path
            exists = $false
        })
    }

    $item = Get-Item -LiteralPath $Path
    $version = $null
    try {
        $versionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($item.FullName)
        $version = [pscustomobject]([ordered]@{
            file_version = $versionInfo.FileVersion
            product_version = $versionInfo.ProductVersion
            company_name = $versionInfo.CompanyName
            product_name = $versionInfo.ProductName
        })
    }
    catch {
        $version = $null
    }

    $fact = [ordered]@{
        path = $item.FullName
        exists = $true
        name = $item.Name
        length = $item.Length
        last_write_time = $item.LastWriteTime.ToString("o")
        version = $version
    }

    if ($IncludeHash) {
        $fact.sha256 = Get-HashOrNull -Path $item.FullName
    }
    if ($IncludePe) {
        $fact.pe = Get-PeFact -Path $item.FullName
    }
    if ($IncludeSignature) {
        $fact.signature = Get-SignatureFact -Path $item.FullName
    }

    return [pscustomobject]$fact
}

function Get-TailLines {
    param(
        [string]$Path,
        [int]$Count = 30
    )

    try {
        return @(Get-Content -LiteralPath $Path -Tail $Count -ErrorAction Stop)
    }
    catch {
        return @("Unreadable: $($_.Exception.Message)")
    }
}

function Read-SteamAppManifest {
    param([string]$GameDir)

    $commonDir = Split-Path -Parent $GameDir
    $steamAppsDir = Split-Path -Parent $commonDir
    $manifestPath = Join-Path $steamAppsDir "appmanifest_311210.acf"
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        return [pscustomobject]([ordered]@{
            path = $manifestPath
            exists = $false
        })
    }

    $keys = [ordered]@{}
    foreach ($line in Get-Content -LiteralPath $manifestPath) {
        if ($line -match '^\s*"([^"]+)"\s+"([^"]*)"\s*$') {
            $keys[$Matches[1]] = $Matches[2]
        }
    }

    $item = Get-Item -LiteralPath $manifestPath
    return [pscustomobject]([ordered]@{
        path = $manifestPath
        exists = $true
        length = $item.Length
        last_write_time = $item.LastWriteTime.ToString("o")
        appid = $keys["appid"]
        name = $keys["name"]
        buildid = $keys["buildid"]
        installdir = $keys["installdir"]
        state_flags = $keys["StateFlags"]
    })
}

function Test-HasNonAscii {
    param([string]$Value)

    foreach ($char in $Value.ToCharArray()) {
        if ([int][char]$char -gt 127) {
            return $true
        }
    }
    return $false
}

if (-not (Test-Path -LiteralPath $GameDir -PathType Container)) {
    throw "GameDir not found: $GameDir"
}

$gameDirItem = Get-Item -LiteralPath $GameDir
$exePath = Join-Path $gameDirItem.FullName "BlackOps3.exe"
$rootFiles = @(Get-ChildItem -LiteralPath $gameDirItem.FullName -File -ErrorAction SilentlyContinue)
$rootDirs = @(Get-ChildItem -LiteralPath $gameDirItem.FullName -Directory -ErrorAction SilentlyContinue)

$allFiles = @(Get-ChildItem -LiteralPath $gameDirItem.FullName -Recurse -File -Force -ErrorAction SilentlyContinue)
$allDirs = @(Get-ChildItem -LiteralPath $gameDirItem.FullName -Recurse -Directory -Force -ErrorAction SilentlyContinue)

$totalBytes = [int64]0
foreach ($file in $allFiles) {
    $totalBytes += [int64]$file.Length
}

$extensionStats = @(
    $allFiles |
        Group-Object { if ($_.Extension) { $_.Extension.ToLowerInvariant() } else { "<none>" } } |
        ForEach-Object {
            $bytes = [int64]0
            foreach ($file in $_.Group) {
                $bytes += [int64]$file.Length
            }
            [pscustomobject]([ordered]@{
                extension = $_.Name
                count = $_.Count
                bytes = $bytes
            })
        } |
        Sort-Object bytes -Descending
)

$topLevelDirs = @(
    foreach ($dir in $rootDirs) {
        $files = @($allFiles | Where-Object { $_.FullName.StartsWith($dir.FullName, [StringComparison]::OrdinalIgnoreCase) })
        $bytes = [int64]0
        foreach ($file in $files) {
            $bytes += [int64]$file.Length
        }
        [pscustomobject]([ordered]@{
            name = $dir.Name
            path = $dir.FullName
            file_count = $files.Count
            bytes = $bytes
            last_write_time = $dir.LastWriteTime.ToString("o")
            has_non_ascii_name = Test-HasNonAscii -Value $dir.Name
        })
    }
) | Sort-Object bytes -Descending

$largestFiles = @(
    $allFiles |
        Sort-Object Length -Descending |
        Select-Object -First 30 |
        ForEach-Object {
            [pscustomobject]([ordered]@{
                path = $_.FullName
                length = $_.Length
                last_write_time = $_.LastWriteTime.ToString("o")
            })
        }
)

$newestFiles = @(
    $allFiles |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 30 |
        ForEach-Object {
            [pscustomobject]([ordered]@{
                path = $_.FullName
                length = $_.Length
                last_write_time = $_.LastWriteTime.ToString("o")
            })
        }
)

$nonAsciiRootItems = @(
    @($rootFiles + $rootDirs) |
        Where-Object { Test-HasNonAscii -Value $_.Name } |
        ForEach-Object {
            [pscustomobject]([ordered]@{
                path = $_.FullName
                name = $_.Name
                type = if ($_.PSIsContainer) { "directory" } else { "file" }
                last_write_time = $_.LastWriteTime.ToString("o")
            })
        }
)

$proxySensitiveNames = @(
    "d3d9.dll",
    "d3d10.dll",
    "d3d10_1.dll",
    "d3d11.dll",
    "d3d12.dll",
    "dinput8.dll",
    "dxgi.dll",
    "opengl32.dll",
    "version.dll",
    "vulkan-1.dll",
    "winmm.dll",
    "xinput1_1.dll",
    "xinput1_2.dll",
    "xinput1_3.dll",
    "xinput1_4.dll",
    "xinput9_1_0.dll"
)

$proxySensitiveRootFiles = @(
    $rootFiles |
        Where-Object {
            $lower = $_.Name.ToLowerInvariant()
            $proxySensitiveNames -contains $lower -or
                $lower -like "reshade*.dll" -or
                $lower -like "specialk*.dll" -or
                $lower -like "nvngx*.dll"
        } |
        ForEach-Object { Get-FileFact -Path $_.FullName -IncludeHash -IncludePe -IncludeSignature }
)

$rootDllFacts = @(
    $rootFiles |
        Where-Object { $_.Extension -ieq ".dll" } |
        Sort-Object Name |
        ForEach-Object {
            if ($HashAllRootDlls -or ($proxySensitiveNames -contains $_.Name.ToLowerInvariant())) {
                Get-FileFact -Path $_.FullName -IncludeHash -IncludePe -IncludeSignature
            }
            else {
                Get-FileFact -Path $_.FullName -IncludePe -IncludeSignature
            }
        }
)

$stagedOverlayDir = Join-Path $repoRoot "profiles\bo3-vulkan\bin\reversa-overlay"
$activeWrapperNames = @("dxgi.dll", "d3d11.dll", "dxgi_system.dll", "d3d11_system.dll")
$activeWrappers = @(
    foreach ($name in $activeWrapperNames) {
        Get-FileFact -Path (Join-Path $gameDirItem.FullName $name) -IncludeHash -IncludePe -IncludeSignature
    }
)

$stagedWrappers = @(
    foreach ($name in @("dxgi.dll", "d3d11.dll")) {
        Get-FileFact -Path (Join-Path $stagedOverlayDir $name) -IncludeHash -IncludePe -IncludeSignature
    }
)

$wrapperBackups = @(
    $rootFiles |
        Where-Object { $_.Name -match '\.(dxvk-disabled|reversa-disabled)(-|$)' -or $_.Name -match '\.reversa-disabled$' } |
        Sort-Object Name |
        ForEach-Object { Get-FileFact -Path $_.FullName -IncludeHash }
)

$logFiles = @(
    $allFiles |
        Where-Object { $_.Extension -in @(".log", ".dmp", ".mdmp", ".crash") } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 100 |
        ForEach-Object {
            [pscustomobject]([ordered]@{
                path = $_.FullName
                extension = $_.Extension.ToLowerInvariant()
                length = $_.Length
                last_write_time = $_.LastWriteTime.ToString("o")
                tail = if ($_.Extension -ieq ".log" -and $_.Length -lt 1048576) { @(Get-TailLines -Path $_.FullName -Count 30) } else { @() }
            })
        }
)

$configFiles = @(
    $allFiles |
        Where-Object { $_.Extension.ToLowerInvariant() -in @(".cfg", ".conf", ".ini", ".json", ".vdf") } |
        Sort-Object FullName |
        Select-Object -First 200 |
        ForEach-Object {
            [pscustomobject]([ordered]@{
                path = $_.FullName
                length = $_.Length
                last_write_time = $_.LastWriteTime.ToString("o")
            })
        }
)

$contentDirs = @(
    foreach ($name in @("mods", "usermaps", "workshop", "zone", "players", "311210", "LPC", "dxvk-logs")) {
        $path = Join-Path $gameDirItem.FullName $name
        if (Test-Path -LiteralPath $path -PathType Container) {
            $files = @($allFiles | Where-Object { $_.FullName.StartsWith($path, [StringComparison]::OrdinalIgnoreCase) })
            $bytes = [int64]0
            foreach ($file in $files) { $bytes += [int64]$file.Length }
            [pscustomobject]([ordered]@{
                name = $name
                path = $path
                exists = $true
                file_count = $files.Count
                bytes = $bytes
            })
        }
        else {
            [pscustomobject]([ordered]@{
                name = $name
                path = $path
                exists = $false
            })
        }
    }
)

$runtime = [pscustomobject]([ordered]@{
    blackops3_process = $null
    watched_modules = @()
})

$bo3Process = Get-Process -Name "BlackOps3" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($bo3Process) {
    $runtime.blackops3_process = [pscustomobject]([ordered]@{
        id = $bo3Process.Id
        process_name = $bo3Process.ProcessName
        path = $bo3Process.Path
        start_time = try { $bo3Process.StartTime.ToString("o") } catch { $null }
    })

    try {
        $runtime.watched_modules = @(
            $bo3Process.Modules |
                Where-Object {
                    $name = $_.ModuleName.ToLowerInvariant()
                    $name -match "dxgi|d3d11|steam|gameoverlay|discord|reversa|specialk|reshade|nvngx|vulkan"
                } |
                ForEach-Object {
                    [pscustomobject]([ordered]@{
                        module_name = $_.ModuleName
                        file_name = $_.FileName
                    })
                }
        )
    }
    catch {
        $runtime.watched_modules = @([pscustomobject]([ordered]@{
            error = $_.Exception.Message
        }))
    }
}

$issues = New-Object System.Collections.Generic.List[object]

function Add-Issue {
    param(
        [string]$Severity,
        [string]$Title,
        [string]$Evidence
    )

    $issues.Add([pscustomobject]([ordered]@{
        severity = $Severity
        title = $Title
        evidence = $Evidence
    })) | Out-Null
}

$exeFact = Get-FileFact -Path $exePath -IncludeHash -IncludePe -IncludeSignature
if (-not $exeFact.exists) {
    Add-Issue -Severity "high" -Title "BlackOps3.exe missing" -Evidence $exePath
}
elseif ($exeFact.signature -and $exeFact.signature.status -ne "Valid") {
    Add-Issue -Severity "medium" -Title "BlackOps3.exe signature is not Valid" -Evidence "Status: $($exeFact.signature.status)"
}

$activeDxgi = $activeWrappers | Where-Object { $_.name -eq "dxgi.dll" } | Select-Object -First 1
$activeD3d11 = $activeWrappers | Where-Object { $_.name -eq "d3d11.dll" } | Select-Object -First 1
$stagedDxgi = $stagedWrappers | Where-Object { $_.name -eq "dxgi.dll" } | Select-Object -First 1
$stagedD3d11 = $stagedWrappers | Where-Object { $_.name -eq "d3d11.dll" } | Select-Object -First 1

if ($activeDxgi.exists -and $stagedDxgi.exists -and $activeDxgi.sha256 -ne $stagedDxgi.sha256) {
    Add-Issue -Severity "high" -Title "Active dxgi.dll does not match staged Reversa dxgi.dll" -Evidence "$($activeDxgi.sha256) != $($stagedDxgi.sha256)"
}

if ($activeD3d11.exists -and $stagedD3d11.exists -and $activeD3d11.sha256 -ne $stagedD3d11.sha256) {
    Add-Issue -Severity "high" -Title "Active d3d11.dll does not match staged Reversa d3d11.dll" -Evidence "$($activeD3d11.sha256) != $($stagedD3d11.sha256)"
}

if (($activeDxgi.exists -or $activeD3d11.exists) -and -not (Test-Path -LiteralPath (Join-Path $gameDirItem.FullName "dxgi_system.dll") -PathType Leaf)) {
    Add-Issue -Severity "high" -Title "Proxy wrapper active without dxgi_system.dll" -Evidence (Join-Path $gameDirItem.FullName "dxgi_system.dll")
}

if ($activeD3d11.exists -and -not (Test-Path -LiteralPath (Join-Path $gameDirItem.FullName "d3d11_system.dll") -PathType Leaf)) {
    Add-Issue -Severity "high" -Title "D3D11 proxy active without d3d11_system.dll" -Evidence (Join-Path $gameDirItem.FullName "d3d11_system.dll")
}

if ($wrapperBackups.Count -gt 0) {
    Add-Issue -Severity "info" -Title "Disabled wrapper backups are present" -Evidence (($wrapperBackups | ForEach-Object { $_.name }) -join ", ")
}

if ($nonAsciiRootItems.Count -gt 0) {
    Add-Issue -Severity "medium" -Title "Root contains non-ASCII item names" -Evidence (($nonAsciiRootItems | ForEach-Object { $_.name }) -join ", ")
}

$crashArtifacts = @($logFiles | Where-Object { $_.extension -in @(".dmp", ".mdmp", ".crash") })
if ($crashArtifacts.Count -gt 0) {
    Add-Issue -Severity "medium" -Title "Crash dump artifacts found" -Evidence "$($crashArtifacts.Count) crash-like files"
}

$unexpectedProxyFiles = @(
    $proxySensitiveRootFiles |
        Where-Object { $_.name -notin @("dxgi.dll", "d3d11.dll", "dxgi_system.dll", "d3d11_system.dll") }
)
if ($unexpectedProxyFiles.Count -gt 0) {
    Add-Issue -Severity "info" -Title "Additional proxy-sensitive DLL names exist in root" -Evidence (($unexpectedProxyFiles | ForEach-Object { $_.name }) -join ", ")
}

$steamManifest = Read-SteamAppManifest -GameDir $gameDirItem.FullName
$hostFact = [pscustomobject]([ordered]@{
    computer_name = $env:COMPUTERNAME
    user = $env:USERNAME
    powershell = $PSVersionTable.PSVersion.ToString()
})
$treeFact = [pscustomobject]([ordered]@{
    file_count = $allFiles.Count
    directory_count = $allDirs.Count
    total_bytes = $totalBytes
    top_level_directories = @($topLevelDirs)
    extension_stats = @($extensionStats)
    largest_files = @($largestFiles)
    newest_files = @($newestFiles)
    non_ascii_root_items = @($nonAsciiRootItems)
})
$gameFact = [pscustomobject]([ordered]@{
    dir = $gameDirItem.FullName
    blackops3_exe = $exeFact
    steam_manifest = $steamManifest
    tree = $treeFact
    root_dlls = @($rootDllFacts)
    proxy_sensitive_root_files = @($proxySensitiveRootFiles)
    active_wrappers = @($activeWrappers)
    staged_wrappers = @($stagedWrappers)
    wrapper_backups = @($wrapperBackups)
    content_dirs = @($contentDirs)
    config_files = @($configFiles)
    log_files = @($logFiles)
})

$generatedAt = (Get-Date).ToString("o")
$audit = New-Object 'System.Collections.Generic.Dictionary[string,object]'
$audit.Add("schema_version", 1)
$audit.Add("generated_at", $generatedAt)
$audit.Add("host", $hostFact)
$audit.Add("game", $gameFact)
$audit.Add("runtime", $runtime)
$audit["issues"] = $issues.ToArray()

$audit | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $jsonPath -Encoding ascii

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# BO3 Install Audit")
$lines.Add("")
$lines.Add("- Generated: ``$generatedAt``")
$lines.Add("- Game dir: ``$($gameDirItem.FullName)``")
$lines.Add("- JSON: ``$jsonPath``")
$lines.Add("")
$lines.Add("## Executable")
$lines.Add("")
if ($exeFact.exists) {
    $lines.Add("- Path: ``$($exeFact.path)``")
    $lines.Add("- Size: ``$($exeFact.length)`` bytes")
    $lines.Add("- SHA256: ``$($exeFact.sha256)``")
    $lines.Add("- Signature: ``$($exeFact.signature.status)``")
    if ($exeFact.pe) {
        $lines.Add("- PE: ``$($exeFact.pe.machine)`` / ``$($exeFact.pe.subsystem)`` / image ``$($exeFact.pe.size_of_image)`` bytes")
        $lines.Add("- Linker timestamp UTC: ``$($exeFact.pe.linker_timestamp_utc)``")
    }
}
else {
    $lines.Add("- Missing: ``$exePath``")
}

$lines.Add("")
$lines.Add("## Folder Scan")
$lines.Add("")
$lines.Add("- Files: ``$($allFiles.Count)``")
$lines.Add("- Directories: ``$($allDirs.Count)``")
$lines.Add("- Total bytes: ``$totalBytes``")
$lines.Add("")
$lines.Add("Top-level directories:")
foreach ($dir in ($topLevelDirs | Select-Object -First 12)) {
    $lines.Add("- ``$($dir.name)``: ``$($dir.file_count)`` files, ``$($dir.bytes)`` bytes")
}

$lines.Add("")
$lines.Add("## Wrapper State")
$lines.Add("")
foreach ($wrapper in $activeWrappers) {
    if ($wrapper.exists) {
        $match = "n/a"
        $staged = $stagedWrappers | Where-Object { $_.name -eq $wrapper.name } | Select-Object -First 1
        if ($staged -and $staged.exists) {
            $match = if ($wrapper.sha256 -eq $staged.sha256) { "matches staged" } else { "DIFFERS from staged" }
        }
        $lines.Add("- ``$($wrapper.name)``: present, ``$match``")
    }
    else {
        $lines.Add("- ``$([IO.Path]::GetFileName($wrapper.path))``: missing")
    }
}
if ($wrapperBackups.Count -gt 0) {
    $lines.Add("- Disabled backups: ``$($wrapperBackups.Count)``")
}

$lines.Add("")
$lines.Add("## Root DLLs")
$lines.Add("")
foreach ($dll in $rootDllFacts) {
    $sig = if ($dll.signature) { $dll.signature.status } else { "unknown" }
    $lines.Add("- ``$($dll.name)``: ``$($dll.length)`` bytes, signature ``$sig``")
}

$lines.Add("")
$lines.Add("## Logs And Dumps")
$lines.Add("")
foreach ($log in ($logFiles | Select-Object -First 20)) {
    $lines.Add("- ``$($log.path)``: ``$($log.length)`` bytes, ``$($log.last_write_time)``")
}
if ($logFiles.Count -eq 0) {
    $lines.Add("- None found.")
}

$lines.Add("")
$lines.Add("## Issues")
$lines.Add("")
if ($issues.Count -eq 0) {
    $lines.Add("- No audit flags.")
}
else {
    foreach ($issue in $issues) {
        $lines.Add("- [$($issue.severity)] $($issue.title): $($issue.evidence)")
    }
}

$lines.Add("")
$lines.Add("## Rule")
$lines.Add("")
$lines.Add("This audit does not patch or disassemble BlackOps3.exe. It records install metadata, hashes, wrapper state, logs, configs, and runtime module facts so optimization experiments have a reproducible baseline.")

$lines | Set-Content -LiteralPath $markdownPath -Encoding ascii

Write-Host "Wrote $jsonPath"
Write-Host "Wrote $markdownPath"
