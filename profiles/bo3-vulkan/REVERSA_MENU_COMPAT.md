# Reversa Friend Menu Compatibility

Reversa treats BO3Enhanced as the required runtime lane for the partner menu.
The menu is not modeled as a generic Steam BO3 overlay anymore.

## Runtime Contract

| Runtime | Expected signals | Menu posture |
| --- | --- | --- |
| Steam/T7 | `BlackOps3.exe`, `T7InternalWS.dll`, optional `T7WSBootstrapper.dll` | Baseline Reversa overlay and telemetry. Menu writes stay guarded by local address checks. |
| BO3Enhanced | `WSBlackOps3.exe`, `T7InternalWS.dll`, `T7WSBootstrapper.dll`, `BO3Enhanced v1.16`, `t7patch.conf` | Required partner-menu compatibility lane. Reversa trains on this contract and reports it in wrapper logs/HUD. |
| Unknown | Missing or mixed runtime signals | Do not trust offsets or menu compatibility claims. Capture manifest first. |

## Training Inputs

BO3Enhanced reference:

- Local clone: `/home/richtofen/.android/repositories/gaming/BO3Enhanced`
- Upstream clone: `/home/richtofen/.android/repositories/gaming/upstream-bo3/shiversoftdev-bo3enhanced`
- Commit currently trained: `3d54e5c5545f`

ConsoleIX dependency signals:

- Attaches to `BlackOps3.exe`.
- Resolves `T7InternalWS.dll`.
- Uses `CaptionAddr = DllBase + 0x50000`.
- Expects bank behavior behind Tranzit/Die Rise map guards and valid weapon guards.
- Keeps clan/caption/camo visuals independent from bank-specific map/weapon guards.

BO3Enhanced dependency signals:

- Defines `WSINTERNAL_MOD_NAME` as `T7InternalWS.dll`.
- Defines `WSTORE_MOD_NAME` as `WSBlackOps3.exe`.
- Exposes `VERSION_STRING` as `BO3Enhanced v1.16`.
- Uses `T7WSBootstrapper` and `dummy_proc` as the import/load boundary.
- Uses `t7patch.conf` for `playername`, `isfriendsonly`, and `networkpassword`.
- Caches Steam friends work because in-game refreshes can hitch.

## Reversa Actions

- Detect and report `Steam/T7`, `BO3Enhanced`, or `Unknown` runtime lane.
- ConsoleIX auto-attach prefers `WSBlackOps3.exe` when BO3Enhanced is running,
  then falls back to `BlackOps3.exe`.
- Keep D3D11 factory hooks disabled by default for T7 and BO3Enhanced lanes.
- Generate a stable training contract at
  `profiles/bo3-vulkan/training/BO3ENHANCED_MENU_CONTRACT.md`.
- Keep BO3Enhanced menu/runtime compatibility separate from Vulkan/framegen work.

## Next Bridge Work

1. Split ConsoleIX process attach into runtime-specific process/module resolution.
2. Add a shared command contract for bank, weapon, table, clan, caption, and camo actions.
3. Keep bank actions behind valid map, valid weapon, and valid `BankAddr`.
4. Keep `hookClanByte` and `camoByte` behind their own address validation, not bank map/weapon checks.
5. Run the same Eisen route after each bridge change and compare capture manifests.

## Current ConsoleIX Guard State

- Weapon and camo selector writes now fail closed when their resolved target
  address is unreadable.
- Bank writes go through one guard: readable `MapIdAddr`, readable weapon
  address, `zm_die` or `zm_tra` map identity, weapon `246` or `241`, and valid
  `BankAddr`.
- Bank scan order is now named in logs: bank pattern first, table pattern probe
  second, signature fallback last. The table and fallback phases are deliberately
  non-writing until a verified bank offset/signature is configured.
- Clan/caption/camo visual paths are not blocked by the bank map/weapon guard.
