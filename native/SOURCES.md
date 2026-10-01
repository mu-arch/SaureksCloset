# Native renderer sources and build

Original bridge source: PolyForm-Noncommercial-1.0.0; see ../LICENSING.md for scope and third-party exclusions. MinHook v1.3.4 is vendored under its own license in `vendor/minhook/LICENSE.txt`. [PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0).

Primary references: [VanillaHelpers](https://github.com/isfir/VanillaHelpers) for Lua/API declarations and local object access; [1.12.1 client-internals research](https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/character-model.md) for initial character-component investigation; [WoWDBDefs](https://github.com/wowdev/WoWDBDefs) for DBC schemas. Function prototypes and call sites were checked against the user's exact executable. The new render hooks are documented in RESEARCH.md.

The release includes generated BodyOptions.h and RaceModels.h so building the bridge does not require redistributing Blizzard DBCs, model assets, or the executable. From the source directory run `sh tools/build_native.sh` with i686-w64-mingw32 GCC installed. Output: native/SaureksCloset.dll, PE32 x86, imports only KERNEL32.dll and msvcrt.dll. Existing generated signatures target the executable hash in CLIENT-BUILD.json. Do not regenerate signatures for another executable and assume the offsets remain valid.

The Lua functions are SaureksClosetSetAppearance(race,sex,skin,face,hairStyle,hairColor,facial), SaureksClosetClearAppearance(), SaureksClosetRealBody(), SaureksClosetRendererVersion(), and SaureksClosetInspect(). Renderer version is 30001. Setter/clear return 1 on accepted request, -1 when no player is available, -2 for invalid input, -3 for unsupported native display, -4 for reentry. Accepted requests can still await model/texture loading.

`/closet diagnose` includes an explicit inspection. Following the pcall success flag, its 18 values are: schema 3, override enabled, override applicable, requested revision, composed revision, full reload count, detail refresh count, real display, real native display, real object scale, component race, component sex, component hair color, component skin, component face, component facial feature, component hairstyle, visual scale ratio. Zero component values when no component exists are not appearance defaults. No pointers, GUIDs or names are returned.

The bridge has been compiled and checked locally, but has not been run in a WoW process here. Read RESEARCH.md for implementation boundaries and remaining runtime validation.

Cape physics now statically links the CPU-only NvCloth source pinned in
`vendor/nvcloth/PROVENANCE.md`, under its separate NVIDIA license. The build
requires Python 3 and an x86 Windows C++ toolchain; `tools/build_nvcloth.py`
builds the dependency locally without downloads. `CapeNvCloth.cpp` and
`CapeWorker.cpp` implement the solver and asynchronous handoff. See
`CAPE-PHYSICS.md` for threading, renderer integration and validation limits.
