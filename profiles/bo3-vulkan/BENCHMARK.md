# BO3 Vulkan Benchmark Sheet

Use this file as the run log template. Do not compare runs unless the map, resolution, render scale, lobby state, and shader-cache state are recorded.

## Reversa-Matrix Rule

WEIGH IT AGAIN = TESSERACT:

- One run is a signal, not proof.
- Three runs with the same setup are a baseline.
- One changed variable per comparison.
- Keep raw logs in `profiles/bo3-vulkan/logs/`.

## Test Phases

| Phase | Platform | Renderer | Goal | Pass Criteria |
| --- | --- | --- | --- | --- |
| 0 | Windows | Native D3D11 | Control sample | Known-good launch and private match |
| 1 | Windows | DXVK/Vulkan | Wrapper sanity | Same map loads, no crash, DXVK HUD visible |
| 2 | Windows | DXVK/Vulkan tuned | Frametime work | Lower 1%/0.1% spikes than Phase 0 |
| 3 | Linux desktop | Wine + DXVK | Cross-platform proof | Same map loads and logs collect |
| 4 | RM11 Pro | Wine/Winlator/Droidspace + DXVK + Turnip | Device bring-up | Menu, map load, stable 5-minute run |
| 5 | External frame-gen | Vulkan/window layer | Experiment only | No input breakage, no crash, artifact notes |
| 6 | Screen-space RT/post | ReShade/vkBasalt-style | Experiment only | Visual toggle works, perf cost measured |

## Per-Run Template

```text
Run ID:
Date:
Commit:
Platform:
GPU/Driver:
Wrapper:
DXVK version:
Wine/Proton/Winlator version:
Map:
Players:
Resolution:
Render scale:
Texture settings:
VSync / cap:
Shader cache state: cold / warm
Private lobby password: yes / no
Network exposure notes:

Average FPS:
1% low:
0.1% low:
Worst visible hitch:
CPU notes:
GPU notes:
Thermal notes:
Crash/hang:
Artifacts:
Decision:
```

## Windows Capture

1. Start the game through `launch-windows-dxvk.ps1`.
2. Load the same private Zombies map.
3. Wait 60 seconds after load for shader compilation to settle.
4. Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\benchmark-bo3-process.ps1 -DurationSeconds 180
```

5. Copy the relevant DXVK logs and CSV name into the run template.

## What Counts As Better

Prefer frametime stability over peak FPS. A lower average with fewer hitches is better for BO3 Zombies than a high average with ugly spikes.

