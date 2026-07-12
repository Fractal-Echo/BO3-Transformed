# Third-Party Components

This repository contains source headers and Windows libraries used to build the
local tooling. They are not game assets.

| Component | Version | Upstream | License | Evidence |
| --- | --- | --- | --- | --- |
| Dear ImGui | 1.90.5 | https://github.com/ocornut/imgui/tree/v1.90.5 | MIT | `include/imgui/imgui.h` SHA-256 `95404dac7a4d507e4eabb5d0aaa3940f29484a4ff66872831e6b3452b747ce8f` |
| GLFW | 3.4.0 | https://github.com/glfw/glfw/tree/3.4 | Zlib | `include/GLFW/glfw3.h` SHA-256 `aa370985f6b493bbe0358a36ab49f5780a6397c0209c6d98a62143dea595b73c` |
| GLEW | header reports 2.3.4 | https://github.com/nigels-com/glew | BSD-3-Clause and MIT | `include/GL/glew.h` SHA-256 `1a2d50dc00a69940838cb3b5413cf7a7ac3cee8078672f27ffbfa2d3b88ad344`; exact upstream tag is unverified |

Tracked library fingerprints:

- `lib/glfw3.lib`: `659fc6343c56e78d47b052ee4ee4899b7ae615008a5eb1bc2b2f80437163dd3d`
- `lib/glew32.lib`: `ad08e791f7320385fd00ad4889199280b26d2d1d91d625f4d1d930c9924e7d8f`
- `lib/glew32s.lib`: `bc7b5074e5636a465b34dc06721762dc85c9c969d978a10357474d3851f88851`

The public GLEW repository currently exposes tags through `glew-2.3.1`, while
the checked-in generated header reports `2.3.4`. Treat the three GLEW hashes as
the authority for this checkout until the original binary package provenance is
recovered; do not infer a nonexistent upstream tag.

DXVK is downloaded into an ignored local profile directory and is not tracked.
The default fetch is pinned to release `v3.0`, asset `dxvk-3.0.tar.gz`, SHA-256
`7dd3243fe1b260a0e9b0b9e49d672ae32e3398bee18c97e7e8569d0ef0eca92d`.
See the linked upstream projects for their complete license texts and notices.
