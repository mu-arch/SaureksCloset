# Saurek's Closet renderer replacement, build 5875

Status: implemented and compiled as 3.0.0-rc1; **no running-client validation performed**. The intermittent running problem is deferred at the user's request. It has been reported on both Windows and Wine; neither platform is an established cause.

## Evidence and implementation

The exact executable hash and checked addresses are in CLIENT-BUILD.json. Local disassembly is kept under research/v3-disassembly (not redistributed in the release archive).

- 0x600320 is the unit's model filename getter, with no stack arguments. Both unit/player virtual tables reference it at offset 0x28. The hook returns a canonical playable race/sex filename only for the active player's normal native display. Model creation remains in the client's own scene manager and lifecycle.
- 0x476B90 initializes CCharacterComponent, takes one descriptor pointer and returns a bool in AL (ret 4). It copies 91 DWORDs into component+0x18 and consumes the caller's model reference. Our hook copies the complete input and changes only words 0,1,2,3,5,6,7: race, sex, hair color, skin, face, facial feature, hair style. The model handle at word 8 and all other state pass through. No additional reference is acquired or released by the hook. The original initializer creates textures and geosets, and native equipment composition follows.
- The component at unit+0xD30 is assigned before initialization. unit+0xD8 is the model; +0xD58 is flags, NOT a model pointer. Every hooked initialization checks current player ownership and the input model handle.
- 0x5FB200 rebuilds the character component with no stack arguments. It prepares the descriptor from real player data and fills equipment afterward. Detail changes call this once if a component/model is present. Loading models receive current settings on normal initialization.
- 0x60ABE0 updates the unit model. A race/sex change invokes it once; a narrowly scoped hook at 0x60AE10 consumes a one-shot refresh request. No fake display ID or descriptor write is used. The normal update method still changes its own render/animation state; this is not a claim that every internal side effect has been ruled out.
- 0x7106C0 builds a CM2 transform and returns with ret 20. Its call at 0x614DED supplies position, angle, axis, scale vector, smoothed position. 0x7BDC40 translates; 0x7BDCA0 scales (earlier provisional interpretations of these were reversed). Only a copied scale vector is adjusted for the currently owned world model.
- Mounted rider scale is supplied as a fresh matrix to 0x710620 at call 0x607BC3. That exact caller alone receives a copied, rescaled basis. Translation/homogeneous elements remain unchanged; other matrix-copy calls pass through to prevent compounding scale.
- Baseline race scales and filenames come from local ChrRaces -> CreatureDisplayInfo -> CreatureModelData. The render ratio is target baseline / native baseline; the real object's scale remains unchanged. This includes male Tauren 1.35, female Tauren 1.25 and Gnomes 1.15. Actual dimensions still require comparison in game.
- The stock player-model UI clones the live model via 0x707400, and its character component via 0x476CB0 at 0x5043D5. The addon uses DressUpModel:SetUnit("player") after showing the frame, followed by Undress/TryOn. A bounded preview refresh allows asynchronous body loading; it does not call the world setter.

State lives in the bridge, keyed by the active player GUID. Current object pointers and model ownership are rechecked. Other units, unsupported native displays, and temporary transformed displays pass through. Selection calls validate all seven inputs and exact allowed combinations. Same selections are idempotent. Hook setup checks executable signatures, creates all hooks before enabling them, and leaves the Lua bridge unavailable on failure.

## Limits and remaining runtime work

Local evidence verifies function signatures and the data flow described above. It does not prove correct asynchronous lifetimes, mount attachments, animation transitions, camera behavior, scale, or rendered appearance under all conditions. Test all eight races and both models, several skin/face/hair combinations, world and UI views, clear/pause/resume, saved-look switches, mounts, shapeshifts, zoning, death and logout.

The old v2 code is preserved under research/v2-retired. It changed player appearance descriptors, invoked VanillaHelpers display morphs (which also rewrite identity/native-display fields), and toggled display through 15435 for full rebuilds. The new addon never calls those body APIs. VanillaHelpers' retained world armor setter still uses that helper's own rebuild path; no running-bug root cause or fix is claimed.

## Local validation

Lua 5.0.3 syntax and behavioral tests, including actual extracted Blizzard panel/rotation functions and mocked rendering; native validation of 2,570 allowed appearance inputs, rejection of invalid choices, preservation of all other descriptor words/model references, and 256 baseline scale conversions; PE32 x86 DLL build with warnings as errors; isolated installer migration/backup/idempotence checks. None is an in-game test.

## 3.1.0: independent outfit preview

Not yet validated in a running client. The existing world hooks retain their behavior. New exported APIs bracket one Lua DressUpModel:SetUnit call: BeginPreview validates seven appearance fields; EndPreview always clears the scope and returns a model token; PreviewStatus reports composition completion for that token and current player GUID.

- 0x707400 is CM2Scene clone (thiscall, model pointer and flags, ret 8). Only caller return address 0x5059DA, a matching game-thread scope, and the current player's source model can substitute it. The substitution calls the stock scene factory at 0x707350 (filename and flags, ret 8). The UI owns its ordinary returned reference and handles async callbacks/camera setup. Other cloning calls pass through.
- The UI callback at 0x504350 retains its new model at 0x5043C2, then calls CCharacterComponent clone 0x476CB0 at 0x5043D5. Only that caller and a registered preview model can use the new path. It reads 91 descriptor words from the current live component into a local array, replaces appearance words and the destination model reference, and calls the original fresh initializer at 0x476B90. Fresh initialization avoids copying race-specific texture caches. The caller's retained reference is consumed by the destination component, just as in the native clone. No source descriptor is written.
- The model destructor 0x70E170 removes registry entries before normal destruction. Eight fixed entries cap memory; token and GUID checks reject stale results and recycled pointers. No world-model pointer or gameplay descriptor is modified. Asynchronous loads remain owned by the stock frame and scene lifecycle.

All new function prefixes and the two narrowly scoped call sites are checked against the original build before hooks activate. Preview-registry tests check ownership, stale tokens, capacity, release/reuse, and preservation of every descriptor word other than appearance/model reference. These tests cannot establish native rendering or lifetime correctness in a running client.

## 3.3.0 static outfit icons

Removed the experimental 3.2.x thumbnail screenshot pipeline following runtime flicker/fallback reports. SnapshotCapture.h, Snapshot.h, four capture APIs, the EndScene hook and its signature checks are removed. Renderer 30005 keeps body overrides and independent large outfit previews. No Direct3D include or screenshot disk I/O remains.

All 16 Interface/CharacterFrame/TemporaryPortrait-{Male,Female}-{Human,Orc,Dwarf,NightElf,Scourge,Tauren,Gnome,Troll}.blp assets were verified in the installed client's interface.MPQ. Lua points ordinary textures directly to these existing assets. No Blizzard image is copied into the addon or release archive.

## 3.3.75 weapon attachment investigation

Renderer 30006 adds an explicitly requested read-only weapon attachment probe. No additional rendering hooks, attachment setters, or model generation are included. See WEAPONRY.md for exact-build evidence, user-confirmed draw behavior, capture instructions, and remaining work.


## 3.4.0 weapon implementation

See WEAPONRY.md and WeaponRenderer.h for the newly enabled, exact-build attachment hooks. Renderer 30400 introduces separate physical positions, native animation-timed routing based on actual equipment, retained extra children and independent preview ownership. Expanded BuildSignatures.h verifies every newly called/hooked native entry point before enabling any hooks. This is implemented and locally simulated, with in-client visual verification still outstanding.
