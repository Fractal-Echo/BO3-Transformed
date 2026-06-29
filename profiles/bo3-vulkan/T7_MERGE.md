# Reversa + T7 Merge Notes

This is the compatibility contract for running Reversa beside the BO3 T7 patch
stack during local private-lobby performance and safety testing.

## Source References

The upstream references are kept outside this repo under:

```text
/home/richtofen/.android/repositories/gaming/upstream-bo3
```

Current local reference commits:

| Reference | Commit | Local role |
| --- | --- | --- |
| `shiversoftdev/t7patch` | `9f8c0e392ee4` | Release/readme reference for public patch behavior. |
| `shiversoftdev/Black-Ops-3-Projects` | `23536c20d648` | Historical source/reference tree. |
| `Scroptss/T7Patch` | `935b03c3068d` | Maintained release/readme reference. |
| `Scroptss/T7Patch-src` | `32bf46ef26e9` | Cleanest source reference for hook/protection layout. |
| `shiversoftdev/BO3Enhanced` | `3d54e5c5545f` | Separate Windows Store executable lane; not compatible with regular T7Patch. |

Do not vendor T7 source or binaries into this repo unless the license and
provenance are reviewed first. Keep copied logic small, attributed, and
replaceable.

## Ownership Boundary

| Surface | Owner | Rule |
| --- | --- | --- |
| BO3 network/password/friend/session safety | T7 | Reversa must not duplicate those hooks. |
| BO3 memory command/menu state | ConsoleIX/Reversa command layer | Keep map, weapon, bank, table, camo, and clan-byte guards explicit. |
| D3D11/DXGI graphics telemetry | Reversa | Reversa records frame timing and draws the overlay. |
| Vulkan bridge/frame generation | Reversa Vulkan profile | Keep framegen out of BO3 memory patch code. |

The practical merge is a runtime contract first, not a blind source merge. T7 is
already in the game folder as `T7InternalWS.dll` / `T7WSBootstrapper.dll`.
Reversa now detects those files and defaults into T7 compatibility mode.

BO3Enhanced stays a separate renderer/performance test profile and is also the
required runtime lane for the partner menu. Its performance claim comes from
using the Windows Store BO3 executable path with Steam files, not from a small
patch we can apply blindly to the Steam executable. Reversa trains on the
BO3Enhanced runtime/menu contract and reports that lane explicitly.

## Runtime Behavior

When T7 companion DLLs are present:

- `dxgi.dll` remains active for Reversa overlay and DXGI-side telemetry.
- `d3d11.dll` still forwards BO3 D3D11 calls to `d3d11_system.dll`.
- Reversa skips D3D11 factory vtable hooks by default.
- The HUD reports `T7 present` plus the detected runtime lane: `Steam/T7`,
  `BO3Enhanced`, or `Unknown`.
- `reversa-wrapper-stack.json` records hashes for active Reversa and T7 files.

Overrides for one-off testing:

```powershell
$env:REVERSA_T7_COMPAT = "0"
$env:REVERSA_T7_COMPAT = "1"
$env:REVERSA_D3D11_FACTORY_HOOKS = "0"
$env:REVERSA_D3D11_FACTORY_HOOKS = "1"
$env:REVERSA_BO3ENHANCED_COMPAT = "0"
$env:REVERSA_BO3ENHANCED_COMPAT = "1"
```

Leave overrides unset for partner testing.

## Next Merge Work

1. Add a small shared BO3 command contract for map ID, weapon address, bank
   table, camo byte, and clan byte validation.
2. Move external menu actions through that contract before any write.
3. Keep `hookClanByte` and `camoByte` independent from weapon/map guards when
   their own target validation passes.
4. Run three captures per renderer state before promoting a change.
5. Keep Vulkan/framegen experiments in the renderer profile, not the memory
   patch layer.
