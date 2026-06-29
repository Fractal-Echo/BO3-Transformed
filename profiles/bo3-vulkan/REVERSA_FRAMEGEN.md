# Reversa BO3 Framegen Lab

This profile turns the current BO3 work into a Reversa-owned performance lab. The first objective is not to train a frame-generation model. The first objective is to collect evidence that tells us which layer is actually failing: native D3D11, DXVK, capture/present, shader cache, CPU scheduling, GPU load, or an external frame-generation layer.

## Current Evidence

- Native D3D11 loaded from `C:\WINDOWS\SYSTEM32\d3d11.dll` and `C:\WINDOWS\SYSTEM32\dxgi.dll` after the DXVK wrappers were disabled.
- DXVK v3.0 launched BO3 on the RTX 5090 and produced DXVK logs.
- PresentMon Console 2.4.1 captured valid native frame rows.
- Native PresentMon capture `bo3-native-presentmon-20260628-1731.csv`:
  - 29621 frames.
  - Average frame time: 6.0771 ms.
  - Average FPS: 164.55.
  - P50: 5.9962 ms.
  - P95: 8.2648 ms.
  - P99: 9.5399 ms.
  - Worst frame: 72.3292 ms.
- ASUS 240 Hz display proof `display-state-asus-240-20260628-1751.json`:
  - Primary display: `\\.\DISPLAY6`.
  - Mode: 2560x1440 at 240 Hz.
  - Active monitor ID list includes `ROG PG279QM` / manufacturer `AUS`.
- DXVK + ASUS 240 Hz Zombies stress baseline:
  - `bo3-dxvk-asus240-gameplay-20260628-1757.csv`: 157.74 FPS average, p95 8.384 ms, p99 9.4536 ms, p99.9 11.6759 ms, max 65.5133 ms.
  - `bo3-dxvk-asus240-gameplay2-20260628-1811.csv`: 149.06 FPS average, p95 8.8678 ms, p99 9.9459 ms, p99.9 11.5805 ms, max 46.1345 ms.
  - `bo3-dxvk-asus240-gameplay3-20260628-1825.csv`: 161.53 FPS average, p95 8.2493 ms, p99 9.389 ms, p99.9 11.9957 ms, max 97.1086 ms.
- Paired process/GPU telemetry for run 2:
  - CPU average: 312.84 on one-core scale.
  - Working set average: 4986.81 MB.
  - Private memory average: 13636.94 MB.
  - Thread count: 81 stable.
  - GPU utilization average: 58.55%, max 96%.
  - GPU power average: 183.33 W.
- Process telemetry comparison showed native used less private memory than DXVK, while native CPU sampling was higher and noisier. That is useful context, but frame-time data is the stronger decision signal.
- Reversa native wrapper v0.4 throttled-HUD validation:
  - Comparison artifact: `bo3-run1-vs-run2-vs-run3-presentmon-summary.md`.
  - Run 1 before throttling: 76.86 FPS average, p95 18.4528 ms, p99 23.263 ms, 1274 frames over 16.67 ms.
  - Run 2 after throttling: 83.83 FPS average, p95 16.0187 ms, p99 19.1528 ms, 501 frames over 16.67 ms.
  - Run 3 after throttling: 86.91 FPS average, p95 14.8837 ms, p99 17.1332 ms, 211 frames over 16.67 ms.
  - Process CPU average on the one-core scale stepped down across the same captures: 582.84 -> 461.45 -> 382.05.
  - Decision: keep the v0.4 overlay throttling patch. The direction repeated across two post-patch captures, with fewer bad frames and lower wrapper-side sampling pressure.

## Local Tooling We Already Have

| Tool | Local role | BO3 decision |
| --- | --- | --- |
| DXVK v3.0 staged in `profiles/bo3-vulkan/bin/` | D3D11/DXGI to Vulkan wrapper | Keep as the Linux/RM11 Pro bridge and Windows A/B test lane. |
| Lossless Scaling | External upscaling/frame-generation layer | Best first framegen experiment because it does not require BO3 memory patches. |
| Special K | Frame pacing, limiter, overlay, and D3D11 control layer | Useful for native D3D11 frame pacing tests. Do not combine with every other wrapper at once. |
| DLSS Updater | Updates DLSS DLLs for games that already ship them | Not directly useful for BO3 right now because no `nvngx`/DLSS DLLs were found in the BO3 directory. |
| DXVKAsync 1.10.3 | Legacy async DXVK drop | Keep as archive only unless a specific old behavior needs reproduction. DXVK 3.0 is the cleaner default. |
| PresentMon | Frame-time capture | Required for every comparison that claims performance improvement. |

## Reversa Rule

WEIGH IT AGAIN = TESSERACT:

- One capture is a signal.
- Three captures on the same route are a baseline.
- Change one variable per run.
- Prefer frame-time percentiles over peak FPS.
- Do not promote generated Reversa summaries as source authority.
- Do not ship or commit local game binaries, wrapper DLLs, logs, captures, or model weights.

## Test Lanes

| Lane | Renderer path | Framegen path | Purpose |
| --- | --- | --- | --- |
| Native | BO3 D3D11 -> Windows D3D11 | None | Control. |
| DXVK | BO3 D3D11 -> DXVK -> Vulkan | None | Vulkan bridge for Linux/RM11 Pro. |
| Native + LSFG | BO3 D3D11 -> Windows D3D11 -> Lossless Scaling | External | Lowest-risk framegen test. |
| DXVK + LSFG | BO3 D3D11 -> DXVK -> Vulkan -> Lossless Scaling | External | Windows proxy for future Linux/RM11 behavior. |
| Native + Special K | BO3 D3D11 -> Special K -> Windows D3D11 | None first | Frame pacing and limiter experiments. |
| DXVK + post stack | BO3 D3D11 -> DXVK -> Vulkan layer/post | External | Later shader/ray-effect experiments only after baseline stability. |
| DXVK + Vulkan framegen | BO3 D3D11 -> DXVK/Reversa Vulkan -> Vulkan framegen path | Vulkan-side | Future RTX 5090 lane. Keep this behind stable DXVK timing and clean SDK/provenance review. |

## Commands

Inventory local tools:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\inventory-bo3-local-tools.ps1
```

Capture process and GPU adapter telemetry:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\benchmark-bo3-process.ps1 -DurationSeconds 180
```

Capture display layout and refresh state:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\capture-display-state.ps1
```

Summarize process/GPU telemetry:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\summarize-bo3-process.ps1 `
  -InputPath .\profiles\bo3-vulkan\logs\bo3-dxvk-asus240-gameplay2-process-20260628-1811.csv
```

Capture real frame times:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\capture-bo3-presentmon.ps1 -DurationSeconds 180
```

Summarize one or more PresentMon captures:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\compare-bo3-presentmon.ps1 `
  -InputPath .\profiles\bo3-vulkan\logs\bo3-native-presentmon-20260628-1731.csv
```

## Training Gate

Training Reversa is useful only after we have a small, clean evidence pack. For this project, training means advisory/eval data first, not model weights first.

Required before any training pass:

- Three native PresentMon captures on the same map, route, resolution, and cap.
- Three DXVK PresentMon captures with the same conditions. The ASUS 240 Hz DXVK Zombies stress lane now satisfies this once the raw artifacts are retained locally.
- At least one Lossless Scaling capture with settings recorded.
- Tool inventory JSON with hashes for the local executables/wrappers used.
- Notes on visible artifacts, input latency, and UI capture behavior.
- Clear label set: `NATIVE_BASELINE`, `DXVK_BASELINE`, `LSFG_EXTERNAL`, `FRAME_PACING_WIN`, `FRAMEGEN_ARTIFACT`, `INPUT_LATENCY_RISK`, `RM11PRO_CANDIDATE`.

## Next Controlled Branch

Do not enable frame generation yet. The next controlled test is native D3D11 on the same ASUS 240 Hz display, same map/session pattern, and same Zombies stress flow. That answers whether DXVK is actually improving frame pacing or whether the 240 Hz display and warmed cache are doing most of the work.

After native 240 Hz has its own three-run baseline, test Lossless Scaling as an external layer over the better renderer path.

Only after that should Reversa classify routes or produce a training/eval pack. A custom frame-generation model is a later research project and should use open/provenance-clean video data for training. BO3 captures can be private evaluation artifacts, not redistributed training assets.

## Vulkan Framegen Gate

The RTX 5090 frame-generation lane belongs after the wrapper is stable:

- Confirm T7 compatibility mode does not worsen native/DXVK frame timing.
- Re-enable DXVK as the Vulkan bridge and capture three clean PresentMon runs.
- Choose the framegen path from clean SDK/licensed components, not copied game
  binaries or untracked DLL drops.
- Record exact DLL hashes and settings in the run notes.
- Compare p95/p99 frame time, input latency feel, HUD artifacts, and capture
  behavior. Do not judge by peak FPS alone.

If this lane wins, Reversa should expose it as a Vulkan profile switch rather
than a BO3 memory patch.

## Safety Boundary

Keep frame generation outside the BO3 memory patch tool. The memory tool should stay focused on known map/weapon/camo/bank logic and safe guards. Rendering experiments belong in wrapper, capture, Vulkan, or external window layers.
