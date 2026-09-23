# Building Saurek's Closet

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

The 3.7.9 build passed the native weapon and staff-placement simulations under address and undefined-behavior sanitizers, version consistency checks, all 70 supported-client signatures, and the 16-body staff-fit audit. Mocked UI and native simulations do not replace testing inside the supported game client. See `native/RESEARCH.md` and `native/WEAPONRY.md` for renderer behavior and remaining validation boundaries.

## Licenses

See `LICENSE` for the project license, `native/vendor/minhook/LICENSE.txt` for MinHook, and the catalog notices in the addon directory. Game executables, extracted Blizzard UI, game models, personal settings, and diagnostic captures are not included.

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
