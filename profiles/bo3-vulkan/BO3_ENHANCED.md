# BO3Enhanced Reference Lane

Upstream reference:

- Repo: `https://github.com/shiversoftdev/BO3Enhanced`
- Local clone: `/home/richtofen/.android/repositories/gaming/upstream-bo3/shiversoftdev-bo3enhanced`
- Commit inspected: `3d54e5c5545f`

## Read

BO3Enhanced is a separate Windows Store BO3 executable path that uses Steam game
files and Steam identity/networking integration. Its README claims much better
performance from that executable path and the lack of Arxan DRM.

For the partner menu, BO3Enhanced is treated as a required runtime lane. Reversa
trains on the full BO3Enhanced runtime/menu contract instead of assuming the
plain Steam/T7 lane has equivalent behavior.

This is not a drop-in replacement for the current Steam/T7/Reversa stack:

- It requires the Windows Store BO3 executable and dependencies.
- It is Windows-only according to upstream.
- Upstream says regular T7Patch cannot be used with that version right now.
- Some workshop maps/mods may need native-component updates.

## Useful Lessons For Reversa

- Keep expensive platform/API work out of the frame path. BO3Enhanced caches
  friend checks and explicitly avoids refreshing the friends list in-game
  because Steam API calls can hitch.
- Treat Windows Store BO3 as a separate A/B performance lane, not as a source
  merge into the Steam executable wrapper.
- Keep network password/friends-only behavior in the T7/BO3Enhanced safety
  layer, while Reversa owns graphics telemetry and renderer experiments.
- Detect `WSBlackOps3.exe`, `T7WSBootstrapper.dll`, `T7InternalWS.dll`,
  `BO3Enhanced v1.16`, and `t7patch.conf` as one compatibility contract for
  the partner menu.
- Keep ConsoleIX bank/table/weapon guards independent from clan/caption/camo
  target validation.

## Training Output

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\train-reversa-bo3enhanced.ps1
```

Stable output:

```text
profiles\bo3-vulkan\training\bo3enhanced-menu-contract.json
profiles\bo3-vulkan\training\BO3ENHANCED_MENU_CONTRACT.md
```

## Test Gate

Before testing this lane:

1. Preserve the current Steam/T7/Reversa install as a restorable state.
2. Acquire the required Windows Store executable/dependencies through a
   provenance-clean path.
3. Create a separate manifest/hash capture before launch.
4. Run the same Eisen route with the same display, resolution, render scale, and
   capture settings.
5. Do not compare BO3Enhanced numbers against Steam/T7 numbers unless the route
   and test state are recorded.
