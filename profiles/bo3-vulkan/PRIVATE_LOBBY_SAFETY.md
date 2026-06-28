# Private Lobby Safety Checklist

This repo lane is for private-lobby research, performance experiments, and exploit-risk reduction. Keep it out of public matchmaking and do not build bypass logic.

## Before Launch

- Use a private Zombies lobby.
- Use a lobby password when available.
- Only play with known people.
- Keep Steam, BO3, GPU drivers, and Windows updated.
- Do not accept random invites while testing.
- Do not run unverified DLLs from random archives.
- Keep DXVK, ReShade, vkBasalt, or frame-gen binaries versioned in notes before testing.

## Network Hygiene

- Prefer invite-only/private sessions.
- Close the game before joining public playlists.
- If using a community safety patch, record its exact version and source in `BENCHMARK.md`.
- Treat every networking/protection claim as untrusted until verified by behavior, logs, or source review.

## Repo Hygiene

- Do not commit `BlackOps3.exe`, copied game DLLs, shader caches, crash dumps, or personal config.
- Keep local binaries under `profiles/bo3-vulkan/bin/`.
- Keep logs under `profiles/bo3-vulkan/logs/`.
- Any exploit research notes belong in private/local notes unless they are defensive, non-actionable, and sanitized.

## Hard Boundary

Allowed:

- Performance profiling.
- Private-lobby stability fixes.
- Defensive safety checklist and launch profiles.
- Graphics wrapper experiments.

Not allowed here:

- Public-match cheating.
- Anti-cheat bypass.
- RCE reproduction steps.
- Payloads, exploit chains, or weaponized networking details.

