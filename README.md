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

3. Run the output as administrator when attaching to `BlackOps3.exe`.

WSL is not currently configured as the compiler environment for this project. Treat WSL as the clean repo/build-space staging area unless a dedicated cross-build setup is added later.

## Current Focus

- Reduce frame-loop memory patching.
- Separate UI, process attach, address resolution, and feature patch logic.
- Keep changes small enough to verify against BO3 behavior.
- Avoid frame generation work until frametime bottlenecks are measured.
