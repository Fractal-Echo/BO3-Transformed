param(
    [string]$EnhancedRoot,
    [string]$OutputRoot,
    [string]$TrainingRoot,
    [string[]]$ReferenceRoots
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$repoParent = Split-Path -Parent $repoRoot
if (-not $EnhancedRoot) {
    $EnhancedRoot = Join-Path $repoParent "BO3Enhanced"
}
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
}
if (-not $TrainingRoot) {
    $TrainingRoot = Join-Path $repoRoot "profiles\bo3-vulkan\training"
}

if (-not (Test-Path -LiteralPath $EnhancedRoot -PathType Container)) {
    throw "BO3Enhanced root was not found: $EnhancedRoot"
}

function Get-ProviderPath {
    param([string]$Path)

    $resolved = Resolve-Path -LiteralPath $Path
    if ($resolved.ProviderPath) {
        return $resolved.ProviderPath
    }
    return $resolved.Path
}

function Convert-UncUbuntuPathToWsl {
    param([string]$Path)

    $prefix = "\\wsl.localhost\Ubuntu\"
    if ($Path.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        return "/" + ($Path.Substring($prefix.Length) -replace "\\", "/")
    }
    return $null
}

function Get-GitCommit {
    param([string]$Root)

    $commit = $null
    try {
        $commit = (& git -C $Root rev-parse HEAD 2>$null).Trim()
    }
    catch {
        $commit = $null
    }
    if (-not $commit) {
        $wslRoot = Convert-UncUbuntuPathToWsl $Root
        if ($wslRoot) {
            try {
                $commit = (& wsl git -C $wslRoot rev-parse HEAD 2>$null).Trim()
            }
            catch {
                $commit = $null
            }
        }
    }
    if (-not $commit) {
        $commit = "unknown"
    }
    return $commit
}

function Get-RelativeFilePath {
    param(
        [string]$Root,
        [string]$Path
    )

    $prefix = $Root.TrimEnd("\") + "\"
    if ($Path.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Path.Substring($prefix.Length)
    }
    return $Path
}

function Get-TrainingSourceFiles {
    param(
        [string]$Root,
        [string[]]$PriorityFiles
    )

    $files = New-Object System.Collections.Generic.List[string]
    foreach ($relative in $PriorityFiles) {
        if (Test-Path -LiteralPath (Join-Path $Root $relative) -PathType Leaf) {
            if (-not $files.Contains($relative)) {
                $files.Add($relative)
            }
        }
    }

    $extensions = @(".cpp", ".h", ".hpp", ".md", ".vcxproj")
    Get-ChildItem -LiteralPath $Root -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object {
            $extensions -contains $_.Extension.ToLowerInvariant() -and
            $_.FullName -notmatch "\\\.git\\"
        } |
        Sort-Object FullName |
        ForEach-Object {
            $relative = Get-RelativeFilePath $Root $_.FullName
            if (-not $files.Contains($relative)) {
                $files.Add($relative)
            }
        }

    return @($files)
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
New-Item -ItemType Directory -Force -Path $TrainingRoot | Out-Null

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$jsonPath = Join-Path $OutputRoot "reversa-bo3enhanced-training-$stamp.json"
$markdownPath = Join-Path $OutputRoot "reversa-bo3enhanced-training-$stamp.md"
$stableJsonPath = Join-Path $TrainingRoot "bo3enhanced-menu-contract.json"
$stableMarkdownPath = Join-Path $TrainingRoot "BO3ENHANCED_MENU_CONTRACT.md"

$prioritySourceFiles = @(
    "README.md",
    "framework.h",
    "dllmain.cpp",
    "security.cpp",
    "security.h",
    "steam.cpp",
    "linux_helper.cpp",
    "T7WSBootstrapper\dllmain.cpp"
)

$patterns = @(
    [pscustomobject]@{
        id = "friend_menu_required_runtime"
        pattern = "WSINTERNAL_MOD_NAME|WSTORE_MOD_NAME|VERSION_STRING|Live_SystemInfo_Hook|BO3Enhanced v"
        lesson = "The partner menu depends on BO3Enhanced as a full runtime contract, not just a few copied offsets."
        transformed_action = "Detect the BO3Enhanced lane explicitly and keep menu compatibility features behind that detected contract."
    },
    [pscustomobject]@{
        id = "friend_menu_loader_contract"
        pattern = "T7WSBootstrapper|dummy_proc|LoadLibraryA|WSINTERNAL_MOD_NAME|check_update_moves|wsupdate\\.next"
        lesson = "BO3Enhanced enters through a bootstrapper/import boundary that loads T7InternalWS and handles update handoff."
        transformed_action = "Train Reversa to treat T7WSBootstrapper/T7InternalWS/WSBlackOps3 as one lane instead of independent optional files."
    },
    [pscustomobject]@{
        id = "friend_menu_config_contract"
        pattern = "PATCH_CONFIG_LOCATION|t7patch\\.conf|playername|isfriendsonly|networkpassword|sec_config"
        lesson = "The runtime contract includes file-backed config for player name, friends-only, and network password behavior."
        transformed_action = "Prefer file-backed runtime state and do not rely on Steam launch environment inheritance for menu-critical switches."
    },
    [pscustomobject]@{
        id = "friend_menu_stability_contract"
        pattern = "dwInstantDispatchMessage_Internal_hook|LobbyMsgRW_PrepWriteMsg_Hook|Sys_Checksum_hook|network password|remote crashes|friends only"
        lesson = "BO3Enhanced carries crash-hardening and private-session behavior that the menu expects to coexist with."
        transformed_action = "Do not run the partner menu as if plain Steam BO3 and BO3Enhanced have the same safety/runtime surface."
    },
    [pscustomobject]@{
        id = "runtime_lane"
        pattern = "WSBlackOps3|Windows Store|regular T7Patch|VERSION_STRING|BO3Enhanced"
        lesson = "Treat BO3Enhanced as the required partner-menu runtime lane when that menu is in use."
        transformed_action = "Keep Steam/T7/Reversa and BO3Enhanced manifests separated, but allow Reversa to train on and report the BO3Enhanced lane."
    },
    [pscustomobject]@{
        id = "avoid_hot_path_platform_work"
        pattern = "friends list|hitch|next_update_friendslist_time|GetFriendCount|SteamFriends"
        lesson = "Cache platform/friends work and avoid refreshing it during gameplay hot paths."
        transformed_action = "Keep overlay, Steam, display, and process telemetry idle unless visible or explicitly capturing."
    },
    [pscustomobject]@{
        id = "callback_threading"
        pattern = "SteamAPI_RunCallbacks|steam_callbacks_thread|Sleep\\(33\\)|ReadP2PPacket"
        lesson = "Move platform callbacks to a controlled background cadence rather than doing everything in-frame."
        transformed_action = "Keep Reversa telemetry publication non-blocking and avoid waiting on locks in Present paths."
    },
    [pscustomobject]@{
        id = "frame_dispatcher_budget"
        pattern = "register_frame_event|dispatch_events|steam_dispatch_every_frame|set_delay"
        lesson = "Frame-event systems need hard budget discipline; every-frame hooks should schedule rare work out of band."
        transformed_action = "Use delayed/idle timers for overlay refresh and reserve frame-linked logic for lightweight counters only."
    },
    [pscustomobject]@{
        id = "config_watcher"
        pattern = "t7patch\\.conf|watch_settings_updates|update_watcher_time|loadfrom|saveto"
        lesson = "Config changes should be file-backed and watched outside the render path."
        transformed_action = "Prefer local config/marker files for Reversa test switches over Steam environment inheritance."
    },
    [pscustomobject]@{
        id = "wine_detection"
        pattern = "wine_get_version|is_on_linux|do_linux_hooks|WineGDK"
        lesson = "Detect Wine/Linux explicitly and isolate compatibility hooks from Windows-native paths."
        transformed_action = "Keep Vulkan/RM11Pro profile switches separate from Windows-native D3D11/T7 behavior."
    },
    [pscustomobject]@{
        id = "loader_manifest"
        pattern = "T7WSBootstrapper|LoadLibraryA|check_update_moves|wsupdate\\.next"
        lesson = "Use a small loader/manifest boundary for update and DLL handoff behavior."
        transformed_action = "Keep reversa-wrapper-stack.json and deploy hash checks as the source of truth for test state."
    },
    [pscustomobject]@{
        id = "affinity_experiment"
        pattern = "SetProcessAffinityMask|GetProcessAffinityMask|DELAYED_EVENT_PROCESSORAFFINITY"
        lesson = "CPU affinity can change pacing, but it is hardware-sensitive and needs A/B captures before promotion."
        transformed_action = "Add affinity as an opt-in experiment only after overlay-idle and BO3Enhanced executable baselines."
    },
    [pscustomobject]@{
        id = "compatibility_boundary"
        pattern = "anticheat|ThreadHideFromDebugger|callspoof|AC_|ac_bypass|Arxan"
        lesson = "Separate menu-required stability/runtime features from unrelated debugger, DRM, or stealth internals."
        transformed_action = "Train on the BO3Enhanced contract and port only compatibility-facing behavior that is needed, understood, and testable."
    }
)

$menuSourceFiles = @(
    "ConsoleIX.cpp",
    "proc.cpp",
    "proc.h",
    "Signature.cpp",
    "Signature.h"
)

$menuPatterns = @(
    [pscustomobject]@{
        id = "consoleix_process_contract"
        pattern = 'GetProcId\(L"BlackOps3\.exe"\)|GetModuleBaseAddress\(procId, L"BlackOps3\.exe"\)|GetModuleBaseAddress\(procId, L"T7InternalWS\.dll"\)|CaptionAddr = DllBase'
        lesson = "ConsoleIX currently attaches to Steam BlackOps3.exe and T7InternalWS directly."
        transformed_action = "Teach the menu bridge to resolve Steam BO3 and BO3Enhanced lanes independently before trusting offsets."
    },
    [pscustomobject]@{
        id = "consoleix_bank_guard_contract"
        pattern = "zm_die|zm_tra|bValidMap|bValidWeapon|BankAddr|ScanThread|FindPatternEx|TableBase"
        lesson = "Bank writes must remain behind map, weapon, and resolved-address guards."
        transformed_action = "Keep bank/table pattern scanning ahead of signature fallback, and invalidate BankAddr on map or weapon changes."
    },
    [pscustomobject]@{
        id = "consoleix_independent_visual_contract"
        pattern = "HookClanByte|CaptionAddr|CamoAddr|Rainbow|bCaption|bClan"
        lesson = "Clan/caption/camo visual features have their own target validation and should not be blocked by bank weapon/map guards."
        transformed_action = "Bridge hookClanByte and camoByte through independent address validation, not the bank-specific guard chain."
    }
)

$referenceRootCandidates = New-Object System.Collections.Generic.List[string]
$referenceRootCandidates.Add($EnhancedRoot)
if ($ReferenceRoots) {
    foreach ($root in $ReferenceRoots) {
        if ($root) {
            $referenceRootCandidates.Add($root)
        }
    }
}
$upstreamRoot = Join-Path $repoParent "upstream-bo3\shiversoftdev-bo3enhanced"
if (Test-Path -LiteralPath $upstreamRoot -PathType Container) {
    $referenceRootCandidates.Add($upstreamRoot)
}

$seenRoots = New-Object "System.Collections.Generic.HashSet[string]" ([System.StringComparer]::OrdinalIgnoreCase)
$trainingPasses = New-Object System.Collections.Generic.List[object]
foreach ($candidate in $referenceRootCandidates) {
    if (-not (Test-Path -LiteralPath $candidate -PathType Container)) {
        continue
    }
    $rootPath = Get-ProviderPath $candidate
    if (-not $seenRoots.Add($rootPath)) {
        continue
    }

    $sourceFiles = @(Get-TrainingSourceFiles -Root $rootPath -PriorityFiles $prioritySourceFiles)
    $trainingPasses.Add([pscustomobject]([ordered]@{
        id = "pass$($trainingPasses.Count + 1)"
        root = $rootPath
        commit = Get-GitCommit $rootPath
        source_file_count = $sourceFiles.Count
        source_files = $sourceFiles
    }))
}

if ($trainingPasses.Count -eq 0) {
    throw "No BO3Enhanced training roots were available."
}

$hits = New-Object System.Collections.Generic.List[object]
foreach ($pass in $trainingPasses) {
    foreach ($relative in $pass.source_files) {
        $path = Join-Path $pass.root $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            continue
        }

        foreach ($pattern in $patterns) {
            $matches = Select-String -LiteralPath $path -Pattern $pattern.pattern -AllMatches -ErrorAction SilentlyContinue
            foreach ($match in $matches) {
                $hits.Add([pscustomobject]([ordered]@{
                    id = $pattern.id
                    pass_id = $pass.id
                    root = $pass.root
                    commit = $pass.commit
                    file = $relative
                    line = $match.LineNumber
                    text = $match.Line.Trim()
                }))
            }
        }
    }
}

$menuHits = New-Object System.Collections.Generic.List[object]
foreach ($relative in $menuSourceFiles) {
    $path = Join-Path $repoRoot $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        continue
    }

    foreach ($pattern in $menuPatterns) {
        $matches = Select-String -LiteralPath $path -Pattern $pattern.pattern -AllMatches -ErrorAction SilentlyContinue
        foreach ($match in $matches) {
            $menuHits.Add([pscustomobject]([ordered]@{
                id = $pattern.id
                file = $relative
                line = $match.LineNumber
                text = $match.Line.Trim()
            }))
        }
    }
}

$grouped = foreach ($pattern in $patterns) {
    $patternHits = @($hits | Where-Object { $_.id -eq $pattern.id })
    $passHitCounts = @(
        foreach ($pass in $trainingPasses) {
            $passHits = @($patternHits | Where-Object { $_.pass_id -eq $pass.id })
            [pscustomobject]([ordered]@{
                pass_id = $pass.id
                root = $pass.root
                commit = $pass.commit
                hit_count = $passHits.Count
            })
        }
    )
    $passesWithHits = @($passHitCounts | Where-Object { $_.hit_count -gt 0 }).Count
    $confidence = if ($passesWithHits -eq $trainingPasses.Count) {
        "confirmed_by_all_passes"
    }
    elseif ($passesWithHits -gt 0) {
        "partial_confirmation"
    }
    else {
        "no_hits"
    }
    $cleanEvidence = @(
        $patternHits |
            Sort-Object file, line, text -Unique |
            Select-Object -First 16
    )
    [pscustomobject]([ordered]@{
        id = $pattern.id
        lesson = $pattern.lesson
        transformed_action = $pattern.transformed_action
        hit_count = $patternHits.Count
        passes_with_hits = $passesWithHits
        total_passes = $trainingPasses.Count
        confidence = $confidence
        pass_hit_counts = $passHitCounts
        evidence = $cleanEvidence
    })
}

$menuGrouped = foreach ($pattern in $menuPatterns) {
    $patternHits = @($menuHits | Where-Object { $_.id -eq $pattern.id })
    [pscustomobject]([ordered]@{
        id = $pattern.id
        lesson = $pattern.lesson
        transformed_action = $pattern.transformed_action
        hit_count = $patternHits.Count
        evidence = @($patternHits | Select-Object -First 12)
    })
}

$transformedPath = Get-ProviderPath $repoRoot
$primaryPass = $trainingPasses[0]

$manifest = [pscustomobject]([ordered]@{
    schema_version = 3
    generated_at = (Get-Date).ToString("o")
    enhanced_root = $primaryPass.root
    enhanced_commit = $primaryPass.commit
    transformed_root = $transformedPath
    training_passes = @(
        foreach ($pass in $trainingPasses) {
            [pscustomobject]([ordered]@{
                pass_id = $pass.id
                root = $pass.root
                commit = $pass.commit
                source_file_count = $pass.source_file_count
            })
        }
    )
    priority_source_files = $prioritySourceFiles
    menu_source_files = $menuSourceFiles
    required_runtime = "BO3Enhanced full runtime lane"
    compatibility_rule = "Partner menu compatibility is valid only after Reversa detects the BO3Enhanced lane or an explicitly selected BO3Enhanced test profile."
    lessons = @($grouped)
    menu_contract = @($menuGrouped)
    safety_boundary = "Train on BO3Enhanced runtime/menu/stability contracts. Port only behavior that is required, understood, testable, and compatible with BO3-Transformed."
})

$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath -Encoding ascii
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $stableJsonPath -Encoding ascii

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# Reversa BO3Enhanced Training Pass")
$lines.Add("")
$lines.Add("- Generated: ``$($manifest.generated_at)``")
$lines.Add("- BO3Enhanced root: ``$($manifest.enhanced_root)``")
$lines.Add("- BO3Enhanced commit: ``$($manifest.enhanced_commit)``")
$lines.Add("- BO3-Transformed root: ``$($manifest.transformed_root)``")
$lines.Add("- Required runtime: ``$($manifest.required_runtime)``")
$lines.Add("- Compatibility rule: $($manifest.compatibility_rule)")
$lines.Add("")
$lines.Add("## Training Passes")
$lines.Add("")
foreach ($pass in $manifest.training_passes) {
    $lines.Add("- ``$($pass.pass_id)`` root: ``$($pass.root)``")
    $lines.Add("  commit: ``$($pass.commit)``")
    $lines.Add("  source files scanned: ``$($pass.source_file_count)``")
}
$lines.Add("")
$lines.Add("## Safety Boundary")
$lines.Add("")
$lines.Add($manifest.safety_boundary)
$lines.Add("")
$lines.Add("## Lessons")
$lines.Add("")
foreach ($lesson in $grouped) {
    $lines.Add("### $($lesson.id)")
    $lines.Add("")
    $lines.Add("- Lesson: $($lesson.lesson)")
    $lines.Add("- Transformed action: $($lesson.transformed_action)")
    $lines.Add("- Evidence hits: ``$($lesson.hit_count)``")
    $lines.Add("- Confidence: ``$($lesson.confidence)`` ($($lesson.passes_with_hits)/$($lesson.total_passes) passes)")
    foreach ($passCount in $lesson.pass_hit_counts) {
        $lines.Add("  - ``$($passCount.pass_id)`` hits: ``$($passCount.hit_count)``")
    }
    foreach ($hit in $lesson.evidence) {
        $lines.Add("  - ``$($hit.pass_id):$($hit.file):$($hit.line)`` $($hit.text)")
    }
    $lines.Add("")
}
$lines.Add("## ConsoleIX/Menu Contract")
$lines.Add("")
foreach ($lesson in $menuGrouped) {
    $lines.Add("### $($lesson.id)")
    $lines.Add("")
    $lines.Add("- Lesson: $($lesson.lesson)")
    $lines.Add("- Transformed action: $($lesson.transformed_action)")
    $lines.Add("- Evidence hits: ``$($lesson.hit_count)``")
    foreach ($hit in $lesson.evidence) {
        $lines.Add("  - ``$($hit.file):$($hit.line)`` $($hit.text)")
    }
    $lines.Add("")
}
$lines.Add("## Output")
$lines.Add("")
$lines.Add("- JSON: ``$jsonPath``")
$lines.Add("- Stable JSON: ``$stableJsonPath``")
$lines.Add("- Stable Markdown: ``$stableMarkdownPath``")

$lines | Set-Content -LiteralPath $markdownPath -Encoding ascii
$lines | Set-Content -LiteralPath $stableMarkdownPath -Encoding ascii

Write-Host "Wrote $jsonPath"
Write-Host "Wrote $markdownPath"
Write-Host "Wrote $stableJsonPath"
Write-Host "Wrote $stableMarkdownPath"
