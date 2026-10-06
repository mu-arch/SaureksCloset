# Building Saurek's Closet 4.1.1

The Lua addon is in `addon/SaureksCloset/`. The native renderer uses C++17 and MinHook; generated headers and build signatures are included.

## Build the DLL

Install an i686-w64-mingw32 GCC/G++ toolchain that supports `-mcrtdll=msvcrt-os`, then run from the repository root:

```sh
sh tools/build_native.sh
```

The output is `native/SaureksCloset.dll`. The prebuilt installation copy is in `addon/SaureksCloset/Installation instructions/`.

The renderer targets the exact Windows 1.12.1 / 5875 executable identified in `native/CLIENT-BUILD.json`. Generated model paths, data tables, and signatures are included. Do not assume other executables share these offsets.

## Package an update

After building, refresh the installation copy before packaging:

```sh
cp native/SaureksCloset.dll 'addon/SaureksCloset/Installation instructions/SaureksCloset.dll'
python3 tools/package.py
```

The packager creates the installable addon ZIP, checks its integrity, and verifies that the addon, bundled DLL, and update-manifest versions match.

## Validation

The local Lua regression suite uses Lua 5.0.3 and extracted original Blizzard UI references listed in `tests/test.lua`. These client references are not distributed in this repository. Native test sources cover appearance state, previews, weapon routing, and attachment lifetime simulations.

The 4.0.1 release passed the focused Lua suites for bags, wardrobe saving,
preview rotation, updates, armor recovery and equipment UI isolation; the native
weapon, armor and equipment simulations passed address and undefined-behavior
sanitizers (leak detection is unavailable in the traced build sandbox). All 83
supported-client signatures and release-version references passed. Mocked UI
and native simulations do not replace testing inside the supported game client.
See `native/RESEARCH.md` and `native/WEAPONRY.md` for renderer boundaries.

## Licenses

See `LICENSE` for PolyForm Noncommercial 1.0.0 and `LICENSING.md` for its scope and preserved third-party terms, `native/vendor/minhook/LICENSE.txt` for MinHook, and the catalog notices in the addon directory. Game executables, extracted Blizzard UI, game models, personal settings, and diagnostic captures are not included.

Optional data-regeneration tools require local client files: set `WOW_EXE` to the supported executable and `WOW_DATA` to its Data directory. Archive readers additionally require a local StormLib build at the path shown in `tools/read_client_data.py`. These tools are not required to build the DLL from the supplied headers.

### LLVM-MinGW alternative (3.7.0)

A standalone LLVM-MinGW **MSVCRT** distribution can build without a system
cross-compiler. This release was built with LLVM-MinGW 20260908:

```sh
CLOSET_TOOLCHAIN=/path/to/llvm-mingw-msvcrt sh tools/build_native.sh
```

The DLL statically links its compiler runtime and imports only Windows system
libraries. The original GCC build remains supported. Run the native weapon
simulation with address/undefined sanitizers and check client signatures with
`tests/verify_client_signatures.py` against the supported local executable.

### Armor recovery validation (3.7.12)

See `native/ARMOR-RECOVERY.md` for the read-only compositor checks and recovery
boundaries. Run `lua5.1 tests/armor_recovery.lua`, `lua5.1 tests/diagnose_report.lua`,
and compile `tests/armor_inspection.cpp` with C++17 and address/undefined-behavior
sanitizers. Verify all pinned executable signatures before installing a new DLL.

### Equipment UI isolation validation (3.7.13)

See `native/EQUIPMENT-UI.md`. Compile `tests/equipment_ui.cpp` with C++17
and address/undefined-behavior sanitizers. Run
`lua5.1 tests/equipment_ui_isolation.lua` against extracted stock 1.12
`research/client-data/PaperDollFrame.lua`; client reference files are not bundled.

### Bag list and independent attachments (3.8.0)

Run `lua5.1 tests/bag_instances.lua`, `lua5.1 tests/bags_list_ui.lua`,
`lua5.1 tests/bag_tuner.lua`, and `lua5.1 tests/weapon_full_page.lua`.
These cover actual addon state/tuner dispatch, saved-look migration, preview
isolation, five-slot capacity with eight stable native instance IDs, generation-aware fit caching, and UI
interaction/geometry. `tests/test.lua` is a historical aggregate fixture and
currently lacks newer client stubs; use the focused suites above.

Compile `tests/weapon_renderer.cpp` with C++17 and address/undefined-behavior
sanitizers. Its bag cases cover duplicate models, independent fits, stock
character-model cloning, ownership cleanup, attachment recovery, and generation
changes after context recreation. `tests/bag_tuning.cpp` covers mount defaults.
The full DLL build retains `-Wall -Wextra -Werror`.

Run `python3 tests/test_bag_catalog.py` with Pillow to validate all sixteen model
and texture pairs, conversion fidelity, stable catalog IDs and deterministic
rebuilds. Source deliveries and rebuild instructions are in `assets/bags/`.
Run `tests/verify_client_signatures.py` against the supported executable before
installation. Simulated rendering checks do not replace checking placement in
the running game after a full restart.

### Wardrobe save controls (3.8.1)

Run `lua5.1 tests/wardrobe_save.lua`, `lua5.1 tests/minimap_toggle.lua`, and
`lua5.1 tests/preview_drag.lua`. They exercise the real Save/New/Update handlers,
edited-name tracking, live-preview and bag-fit capture, discard/cancel popup
callbacks, failed-switch preservation, minimap controls and scaled mouse drag
through model-buffer swaps. Release 4.0.1 uses native renderer 40001.

### Release 4.0.1

Addon 4.0.1, native renderer 40001 (DLL 4.0.1), and `update-version.txt` must agree.
Run `python3 tests/release_version.py` after updating these references and packaging.
Run `lua5.1 tests/body_arrow_loading.lua` for native texture-load failure handling.
The five fixed bag slots preserve placements when another slot is cleared.
Bag selection, tuner cleanup, and the centered Apply button are covered by the
focused bag UI and browser geometry checks. Historical versions above identify
when individual features and validations were introduced.

### Mandatory release regression gate

`tools/package.py` runs `tools/check_release.py` before creating an installer.
Use Python 3.12, install `tools/test-requirements.txt` with pip, and install Lua 5.1.
The image-library versions are pinned to reproduce the shipped DXT1 textures;
Ubuntu's older packaged Pillow cannot encode them. The same gate runs in GitHub Actions
on every push and pull request. Body compatibility is checked using the real
release requirement; unknown versions and missing native APIs remain rejected.
Arrow tests exercise normal, hover, held, release, disabled and hide states,
including failed/intermittent texture loading. Pixel checks render the shipped
chevron with actual Lua-emitted geometry at six UI scales. These tests caught
both the 3.9.1 compatibility-list omission and the old pressed-state offset.

### Release 4.0.2

Addon 4.0.2 and renderer 40002 include body-mounted bag contacts, independent sway, size-sensitive rocking, quiet idle settling and placement down to the feet. The full release gate covers attachment contacts, jump lift, frame rates and saved-fit limits. GitHub CI uses the pinned image encoder; README text is independent of release-version checks.

### Release 4.0.4

Addon 4.0.4 and renderer 40004 restore visible walking flap on small bags,
with bounded size compensation and smooth outward lift at the actual fitted
mount. Running bob uses 1.75x gain with size applied once. Only Mageweave bags
stretch; every other model remains rigid, and the bag menu labels both types.
The release gate includes emitted walking motion, frame-rate stability,
independent phases, rigid geometry, jump response and idle settling.

### Release 4.0.6 weapon regression checks

Run the release gate for per-hand stowed visibility and two-handed recovery
after ranged-to-unarmed NPC/loot transitions. `tests/weapon_renderer.cpp` covers
world and preview routing; `tests/weapon_full_page.lua` and
`tests/bag_instances.lua` cover controls, saved choices and bridge dispatch.

### Release 4.0.7

The release gate covers stowed placement tuning for all three equipped weapon
slots in both modes, restoration of native position/rotation/size when drawn,
hand-only wands, and silent item loading in both preview windows. Original
project code uses PolyForm Noncommercial 1.0.0; see LICENSING.md for scope.

### Release 4.0.8

The release gate covers per-slot wand stow visibility, draw/restow recovery,
placement tuner input validation, and equipped weapon fits in previews.
Before every publication, merge upstream edits, preserve the README and
screenshots, and package them byte-for-byte.

### Release 4.0.9

Publishes the 4.0.8 behavior with the omitted tuner tooltip regression test
included in the repository, so the GitHub release checks can run successfully.
The 4.0.8 release tag remains intact.

### Release 4.0.10

Addon 4.0.10 and renderer 40010 include model-based mass for broad bags,
clearer precision-tuner axis labels, and Settings donation links with the supplied
Cash App QR code shown on request. The donation letter fits without scrolling,
and the minimap tooltip displays the current addon version. The release gate
covers bag mass, donation layout, browser allowlisting and copy-link fallbacks.

### Release 4.0.11

Addon 4.0.11 and renderer 40011 package the combined Settings donation button,
the supplied transparent signature artwork, and the restored plain-text sign-off.
Both donation links and the Cash App QR remain in the donation window. The release
gate checks layout, browser actions, transparent artwork and its source checksum.

### Release 4.1.1

Addon 4.1.1 and renderer 40101 include Haircraft hat placement and optional hair
fitting, per-slot weapon physics, bag physics controls, and cape motion tuning.
Transmog sharing controls and sessions remain disabled pending launch. The release
gate covers hair and scalp preservation, placement controls, physics settings,
sharing disablement, and shared window alignment.
