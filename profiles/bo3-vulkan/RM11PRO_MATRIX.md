# RM11 Pro BO3 Vulkan Bring-Up Matrix

The RM11 Pro lane is the endgame, but do not start there. Prove the Windows and Linux desktop paths first, then transfer one known-good variable set to the phone.

## Target Pipeline

```text
BlackOps3.exe
  -> Wine/Winlator/Droidspace
  -> x86_64 translation layer
  -> DXVK
  -> Vulkan
  -> Turnip/Adreno driver
```

## Bring-Up Order

1. **Windows native D3D11** - prove the map and private-lobby flow.
2. **Windows DXVK** - prove BO3 tolerates D3D11 to Vulkan wrapping.
3. **Linux desktop Wine + DXVK** - prove the Wine profile before Android-specific variables.
4. **RM11 Pro menu boot** - reach main menu only.
5. **RM11 Pro private lobby boot** - load the map, no gameplay benchmark yet.
6. **RM11 Pro 5-minute stability run** - record thermal and CPU/GPU notes.
7. **RM11 Pro tuning** - only after a stable baseline.

## RM11 Pro Variables

| Variable | Record |
| --- | --- |
| ROM | Global / China / custom / GSI |
| Kernel | Stock / custom, commit or build tag |
| Driver | Turnip version |
| Container | Droidspace / Winlator / other |
| Wine | Version and build |
| Translation | Box64 / FEX / other |
| DXVK | Version |
| Resolution | Exact width x height |
| Controls | Touch / controller / mouse-keyboard |
| Thermal state | Cool / warm / throttling |

## First-Pass Settings

- Start at 720p or lower.
- Disable post-processing experiments.
- Disable frame generation.
- Use warm shader cache for comparison runs, but record cold-cache behavior separately.
- Cap FPS before chasing max FPS.

## Stop Conditions

Stop the run and record notes if:

- The device thermal-throttles hard.
- Vulkan driver crashes.
- Audio desyncs badly.
- Input latency becomes unplayable.
- The private lobby becomes reachable by unknown players.

