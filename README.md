# BO3 Transformed

Tooling and experiments for Black Ops 3 Zombies custom maps.

## Workspace

The repo is intended to live in Ubuntu/WSL for source control and code review:

```sh
cd ~/repositories/BO3-Transformed
git status
```

The executable target is Windows-only. `ConsoleIX.vcxproj` uses WinAPI, OpenGL, GLFW, ImGui, and bundled Windows `.lib` files, so build it with Visual Studio/MSBuild on Windows.

## Build Target

Recommended build path:

1. Install Visual Studio Build Tools 2022 with the C++ workload.
2. From Windows PowerShell, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-release.ps1
```

   Or invoke the same Windows toolchain from WSL through the checked-in bridge:

```sh
bash scripts/build-windows-from-wsl.sh release
```

   The bridge starts PowerShell from `C:\Windows\Temp` and normalizes `TEMP`
   and `TMP`. This avoids UNC-current-directory failures without copying the
   source tree out of WSL. Use `overlay` instead of `release` to build the
   Reversa DXGI/D3D11 proxy pair. The `dxvk` target runs the pinned DXVK fetch
   and digest-verification script through the same bridge.

3. Run the output as administrator when attaching to `BlackOps3.exe`.

WSL remains the source-control environment. Compilation is performed by the
installed Windows MSVC toolchain through the bridge above; WSL does not attempt
to cross-compile the executable.

## Current Focus

- Reduce frame-loop memory patching.
- Separate UI, process attach, address resolution, and feature patch logic.
- Keep changes small enough to verify against BO3 behavior.
- Avoid frame generation work until frametime bottlenecks are measured.

## Vulkan Experiment Lane

The first cross-platform graphics lane lives in `profiles/bo3-vulkan/`.

Start with `profiles/bo3-vulkan/README.md`, then use the benchmark and safety checklists before testing DXVK, Wine, RM11 Pro, frame generation, or screen-space ray effects.
