# BO3 Vulkan Experiment Profile

This profile is for private-lobby research on Black Ops 3 Zombies using a Vulkan-based graphics path.

Primary pipeline:

```text
BlackOps3.exe -> D3D11/DXGI -> DXVK -> Vulkan driver
```

The goal is to measure and tune frametime stability before experimenting with frame generation or screen-space ray effects. Keep every change reversible and test one variable at a time.

## Layout

- `dxvk.conf` - conservative DXVK profile for repeatable frametime tests.
- `launch-windows-dxvk.ps1` - Windows launcher that points BO3 at this profile.
- `launch-wine-dxvk.sh` - Wine/Linux launcher for Proton, Droidspace, or Winlator-style lanes.
- `BENCHMARK.md` - test matrix and result log template.
- `PRIVATE_LOBBY_SAFETY.md` - private-session safety and exploit-reduction checklist.
- `RM11PRO_MATRIX.md` - RM11 Pro bring-up path.
- `bin/` - local-only DXVK DLL drop zone. DLLs are intentionally ignored by Git.
- `logs/` - local-only run logs and benchmark captures.

## Windows DXVK Quick Start

1. Download DXVK with:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\fetch-dxvk-release.ps1
```

2. Launch BO3 through the profile:

```powershell
powershell -ExecutionPolicy Bypass -File .\profiles\bo3-vulkan\launch-windows-dxvk.ps1 -GameDir "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
```

3. Capture a process benchmark while the map is running:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\benchmark-bo3-process.ps1 -DurationSeconds 180
```

## Linux/Wine Quick Start

Set the Wine prefix and game directory, then run:

```sh
BO3_GAME_DIR="/path/to/Call of Duty Black Ops III" \
WINEPREFIX="$HOME/.wine-bo3" \
./profiles/bo3-vulkan/launch-wine-dxvk.sh
```

## Frame Gen And Ray Effects

Do not put frame generation inside the memory-patch tool. First prove whether the game is CPU-bound, GPU-bound, shader-cache-bound, or translation-layer-bound.

- Frame generation: external/window or Vulkan-layer experiments only after stable baseline frametimes.
- Ray tracing: screen-space/post-process experiments only. Real RT needs engine scene data BO3 does not expose cleanly.
- RM11 Pro: treat every Android/Wine/DXVK build as a separate profile, because Turnip, Box64/FEX, and container behavior can change results.
