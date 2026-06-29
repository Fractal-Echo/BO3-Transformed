# Reversa DXGI Overlay

This is the first Reversa-owned overlay lane for BO3 experiments. It is a local
`dxgi.dll` / `d3d11.dll` proxy pair. The DXGI proxy forwards exports to a copied
`dxgi_system.dll` and owns the transparent overlay window. The D3D11 proxy
forwards exports to a copied `d3d11_system.dll` and hooks the BO3 swap-chain
creation path for frame telemetry.

It is not an EXE patch, anti-cheat bypass, network mod, trainer, or stealth
injector. Keep it private-lobby and reversible.

The overlay waits for the real BO3 window, then creates itself as an owned
tool window with no title. This keeps Windows automation, capture tools, and
benchmark scripts pointed at the game window instead of the overlay surface.

## What v0.1 Shows

- Process ID.
- CPU usage on the same one-core scale as the PowerShell benchmark script.
- Working set and private memory.
- Thread and handle counts.
- Current game-window rect.
- Display handle and refresh rate.

## What v0.2 Adds

- In-process swap-chain `Present` / `Present1` telemetry.
- Frame-time last sample, EMA, and FPS estimate inside the overlay.
- A Reversa partner-menu shell that mirrors the external ConsoleIX menu
  categories without firing memory patches from the wrapper.
- A D3D11 proxy that intercepts BO3's `D3D11CreateDevice` path, installs DXGI
  factory hooks, wraps the returned swap chain, and shares present counters with
  the overlay through a per-process named mapping.

## What v0.3 Adds

- A transparent top-band HUD instead of a filled panel.
- High-contrast outlined text so the overlay stays readable without hiding the
  lobby, menu, or Zombies view behind a black rectangle.

## What v0.4 Adds

- Lower overlay overhead during captures: hotkeys poll at 100 ms, lightweight
  repaint refresh is capped at 500 ms, and expensive window/process/display
  stats refresh at 1000 ms.
- Menu and HUD toggle actions still force an immediate repaint, so interaction
  stays responsive without polling process/thread/display counters four times a
  second.
- Validation artifacts:
  `profiles\bo3-vulkan\logs\bo3-run1-vs-run2-vs-run3-presentmon-summary.md`
  and `profiles\bo3-vulkan\logs\bo3-run1-vs-run2-vs-run3-process-summary.md`.

## What v0.5 Adds

- Non-blocking present telemetry publication. The render path now tries to
  acquire the shared telemetry slot and skips the publish if the overlay or
  another proxy owns it.
- DXGI fallback telemetry also avoids waiting on its local stats lock, so the
  wrapper cannot hold a frame while publishing HUD counters.

## What v0.6 Adds

- T7 companion detection for `T7InternalWS.dll` and `T7WSBootstrapper.dll`.
- T7 compatibility mode defaults on when those companion DLLs are present.
- In compatibility mode, Reversa keeps the DXGI overlay path but skips the
  D3D11 factory vtable hook path. T7 owns the BO3 safety/network patch surface;
  Reversa owns graphics telemetry and overlay.
- Deploy writes `reversa-wrapper-stack.json` into the game directory with
  hashes for active Reversa, system-forwarder, and T7 companion DLLs.

## What v0.7 Adds

- Overlay content starts hidden by default.
- `F9`, `F10`, and `F11` use registered Windows hotkeys instead of 100 ms
  polling.
- The overlay timer stops completely while both the HUD and partner menu are
  hidden.
- The partner menu still polls arrows and `Space`, but only while the menu is
  open.

## What v0.8 Adds

- BO3 runtime lane detection shared by the DXGI and D3D11 proxies.
- Wrapper logs and HUD now report `Steam/T7`, `BO3Enhanced`, or `Unknown`.
- BO3Enhanced forces the same conservative D3D11 compatibility posture as T7
  unless explicitly overridden for a one-off test.
- Reversa training output for the partner menu is generated from BO3Enhanced
  and ConsoleIX evidence under `profiles\bo3-vulkan\training`.

Hotkeys:

- `F9`: toggle the Reversa partner menu.
- `F10`: toggle overlay text.
- `F11`: toggle click-through mode.
- `Left` / `Right`: change partner-menu tab.
- `Up` / `Down`: select a partner-menu item.
- `Space`: toggle the selected partner-menu item as local armed state.

The partner menu is intentionally local state only in this wrapper. ConsoleIX
still owns process attach, address resolution, and patch writes until those
actions are split behind shared validation guards.

## Build

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-reversa-dxgi-overlay.ps1
```

Build output:

```text
profiles\bo3-vulkan\bin\reversa-overlay\dxgi.dll
profiles\bo3-vulkan\bin\reversa-overlay\d3d11.dll
```

## Deploy

Exit BO3 first, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\profiles\bo3-vulkan\deploy-reversa-overlay.ps1 -GameDir "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
```

The deploy script copies:

- `profiles\bo3-vulkan\bin\reversa-overlay\dxgi.dll` to the game directory.
- `profiles\bo3-vulkan\bin\reversa-overlay\d3d11.dll` to the game directory.
- `C:\Windows\System32\dxgi.dll` to the game directory as `dxgi_system.dll`.
- `C:\Windows\System32\d3d11.dll` to the game directory as `d3d11_system.dll`.

If a staged DXVK `dxgi.dll` or `d3d11.dll` is active, the deploy script renames
it to `*.dxvk-disabled`. If an unknown active DLL is present, the script refuses
to overwrite it.

When T7 companion files are present, the deploy script records that state in
`reversa-wrapper-stack.json`. Runtime overrides are available for one-off A/B
tests:

```powershell
$env:REVERSA_T7_COMPAT = "0"              # force compatibility mode off
$env:REVERSA_T7_COMPAT = "1"              # force compatibility mode on
$env:REVERSA_D3D11_FACTORY_HOOKS = "1"    # force Reversa D3D11 hooks on
$env:REVERSA_D3D11_FACTORY_HOOKS = "0"    # force Reversa D3D11 hooks off
$env:REVERSA_BO3ENHANCED_COMPAT = "1"     # force BO3Enhanced runtime lane
$env:REVERSA_BO3ENHANCED_COMPAT = "0"     # force Steam/T7 runtime lane
```

Default test posture: leave those variables unset.

## Disable

Exit BO3 first, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\profiles\bo3-vulkan\disable-reversa-overlay.ps1 -GameDir "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
```

The disable script verifies active `dxgi.dll` and `d3d11.dll` hashes match the
staged Reversa DLLs before renaming them to `*.reversa-disabled`. It also
removes `dxgi_system.dll` and `d3d11_system.dll`.

## Kill Switch

Set this environment variable before launching the game to load the proxy but
skip creating the overlay window:

```powershell
$env:REVERSA_OVERLAY_DISABLE = "1"
```

When launching through an already-running Steam client, prefer the file kill
switch because Steam may not inherit a new shell environment:

```powershell
New-Item -ItemType File -Force -Path "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III\reversa-overlay.disable"
```

Delete `reversa-overlay.disable` to restore the overlay window on the next game
launch.

## Next Steps

Move shared ConsoleIX labels, map guards, weapon guards, and address validation
into a common module that both the external menu and wrapper can consume. After
that, the wrapper can send guarded commands instead of directly owning process
patch logic.
