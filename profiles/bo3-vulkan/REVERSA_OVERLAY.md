# Reversa DXGI Overlay

This is the first Reversa-owned overlay lane for BO3 experiments. It is a local
`dxgi.dll` proxy that forwards DXGI exports to a copied `dxgi_system.dll` and
starts a small transparent overlay window inside the BO3 process.

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

Hotkeys:

- `F10`: toggle overlay text.
- `F11`: toggle click-through mode.

## Build

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-reversa-dxgi-overlay.ps1
```

Build output:

```text
profiles\bo3-vulkan\bin\reversa-overlay\dxgi.dll
```

## Deploy

Exit BO3 first, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\profiles\bo3-vulkan\deploy-reversa-overlay.ps1 -GameDir "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
```

The deploy script copies:

- `profiles\bo3-vulkan\bin\reversa-overlay\dxgi.dll` to the game directory.
- `C:\Windows\System32\dxgi.dll` to the game directory as `dxgi_system.dll`.

If a staged DXVK `dxgi.dll` is active, the deploy script renames it to
`dxgi.dll.dxvk-disabled`. If an unknown `dxgi.dll` is present, the script refuses
to overwrite it.

## Disable

Exit BO3 first, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\profiles\bo3-vulkan\disable-reversa-overlay.ps1 -GameDir "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III"
```

The disable script verifies the active `dxgi.dll` hash matches the staged Reversa
overlay before renaming it to `dxgi.dll.reversa-disabled`. It also removes
`dxgi_system.dll`.

## Kill Switch

Set this environment variable before launching the game to load the proxy but
skip creating the overlay window:

```powershell
$env:REVERSA_OVERLAY_DISABLE = "1"
```

## Next Steps

v0.2 should hook the swap-chain path in a controlled way so the overlay can show
true in-game frame timing without depending on external PresentMon captures. Do
that after v0.1 proves stable in a private Zombies run.
