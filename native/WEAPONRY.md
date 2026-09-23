# Independent weapon placements — 3.4.0 / renderer 30400

## Implemented renderer

`WeaponRenderer.h` hooks the exact-build composer, storage-point lookup, animation-timed move, child lookup and remove-by-point paths. The normal composer retains its attachment-ID return contract, including ranged callback ownership. No update fields or gameplay equipment are written.

Physical IDs 101–107 are Lua-only keys; they never reach the normal inventory override API. Their native attachment points are 32, 33, 30, 31, 28, 27 and 26. Quiver point 26 was also verified in the native quiver composer at 0x479c50. Positions are distinct; there are no invented bone offsets. Back left/right accept both short and large weapons.

`WeaponState.h` selects unique drawn placements using real equipment IDs supplied by GetInventoryItemLink. Exact subclass wins; melee falls back to the same family. Ranged subclass stays exact to retain the stock bow/gun hand and callback behavior. Empty real slots do not acquire drawn weapons. Each unmatched item has an independent child model retained by a bounded context registry. Stock find/remove operations exclude owned decorative children. The native move routine still performs the animation-timed handoff for routed weapons.

The world and each preview have separate context/model ownership. Model destruction removes retained children before invalidating the context. Idempotent synchronization reattaches retained children after a stock clear without recreating them every tick. Changing the loadout rebuilds weapons, not the full character. Toggling off releases all extra children and restores stock weapon composition. Wardrobe and saved previews use the existing independent model-copy API.

Generated WeaponAssets.h validates 2,774 item/model/skin combinations against local client archives. WeaponData.lua supplies compatibility and 11 selectable quivers. Generated data contains file paths and metadata, not redistributed game models. Unsupported or absent client assets are excluded. Custom-server-only items are not included in this allowlist.

Tests: tests/weapon_state.cpp covers category/side routing and all asset compatibility; tests/weapon_renderer.cpp runs the actual hooks against a native-object simulator with address/undefined sanitizers, exercising references, cleanup, same-instance movement, callbacks, isolated previews and idempotence. tests/test.lua covers the UI, seven selections, real equipment inputs, drafts, saving/loading, legacy migration, toggling and clearing. These do not replace in-client rendering validation; attachment orientation/clipping, mounted transitions and client-specific mods still require live testing.

## Historical research and capture notes

The following records describe the investigation before the 3.4.0 implementation; statements about missing hooks or the earlier three-slot UI refer to that earlier stage.

## Requested behavior

Replace the three inventory-slot controls with separately customizable body placements: left and right waist, back positions for melee/two-handed weapons, shield/back equipment, ranged weapon storage, and quiver. More than three models may be visible simultaneously. Store these selections with each saved look and the single unsaved draft.

The user confirmed that stored weapons move into the hands when drawn, and selection of the drawn appearance follows the type of the weapon actually equipped. Extra unmatched models stay stored. Actual equipment and gameplay data must remain unchanged. A quiver is storage decoration, never a hand weapon.

Weapon category routing must use the real inventory item, not a visual override's virtual-item metadata. A two-handed weapon must not consume or delete the stored one-handed appearances. Ranged drawing should restore melee items to their storage positions. Ties between identical categories need a deterministic hand/side preference. Missing matches should preserve the normal equipped weapon's appearance. Saved-look previews must hold their own placement state and not adopt the active world loadout.

## Exact-build evidence inspected locally

Pinned WoW.exe: build 5875, SHA256 b4756d38ef207c02ed651f4952bd89a70b4857b73a33413339e1b285b28d2dc7.

- `0x47a0c0`: fastcall held-item composer; ECX parent CM2 model, EDX ItemDisplayInfo record, five stack arguments, ret 20. Handles internal equipment slots 15/16/17. Chooses hand/storage attachment, removes prior occupants, loads the replacement, returns attachment ID. Ranged caller `0x611eb7` then resolves the returned attached child and retains it for callbacks, so this return contract cannot be bypassed casually.
- `0x47a070`: storage-point mapping; fastcall with sheath type in ECX and side flag in DL. Native table produces paired IDs 26/27, 30/31, 32/33, or single ID 28. These are candidate points, not yet verified visual left/right labels for the proposed UI.
- `0x4798c0`: child loader; ECX parent, EDX attachment, three stack arguments (model path, texture path, effect), ret 12. Creates through scene factory, binds texture unit 2, clears occupants at the attachment, attaches the child, releases its temporary reference. Calling this repeatedly at shared points would destroy other placements.
- `0x712f70`: thiscall attach, two stack arguments (parent, attachment ID), ret 8. Adds to parent's child list and acquires a reference.
- `0x713020`: detach unlinks and releases the parent-owned reference. A caller retaining a child across detach must own a separate reference.
- `0x7130a0`: remove all children matching an attachment ID, not just one inventory-slot model.
- `0x712f00`: resolves the first child at an attachment ID.
- CM2 offsets observed in those functions: child parent +0x1cc, attachment ID +0x1d0, resolved attachment index +0x1d4, parent first child +0x1dc, child previous-link pointer +0x1e0, next child +0x1e4. +0x10 is the loaded state; refcount is at +0.
- Unit +0xd40 is read by the normal melee composer at 0x605dbc to decide whether to request the sheathed route. Its full transition semantics still need a running-client capture.
- Local ItemDisplayInfo.dbc contains Quiver_A.mdx with numerous separate texture variants (examples 13991, 16284–16295, 21318–21332, 21712, 31162). Current catalog generation excludes inventory-type 18 / quiver class, so quiver selections need catalog work too.

Reference consulted alongside local disassembly: https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/character-model.md . Addresses are not accepted solely on that reference. New rendering hooks have NOT been added.

## Required renderer design

Keep a separate placement record array, rather than inventing additional normal equipment slot numbers. A model registry must own each additional child's reference, validate parent ownership, and release on look changes, disable, model destruction, zoning and logout. Share asset data, not live child instances, between world and preview models. Reattach an existing instance on draw-state changes; avoid a periodic full model reload.

Two logical positions may share an underlying native bone point and need separate local transforms. Native code removes all occupants at one point; additional placements must coexist with that cleanup and the ranged callback holder. This lifetime/routing boundary is the current blocker for enabling the requested feature.

## 3.3.75 diagnostic bridge (renderer 30006)

Adds only SaureksClosetWeaponryProbe(selector). No new hooks or model mutations. Selector -1 reads the current player's world model summary and native virtual-item metadata; 0..63 reads one child. Reads use ReadProcessMemory, check parent ownership, detect cycles, and bound traversal. Each callback returns no more than 15 numbers, below Lua 5.0's guaranteed 20-slot callback space. It never enumerates other units.

`/closet weaponscan` captures 30 seconds at at most 10 samples/sec, retaining only changed snapshots with a 128 KiB cap. Records real inventory item IDs/types alongside attached-child IDs, ownership, references and raw draw state. A second invocation stops early. No timer work runs when inactive. Writes VanillaHelpersData/SaureksCloset-weaponry.txt through the existing helper, with a saved-variable fallback. No account/character names or GUIDs are collected; model addresses identify instances within this one process session.

Needed on the user's machine: copy BOTH the addon folder and SaureksCloset.dll, restart through VanillaFixes, run the command, draw/sheathe melee and ranged weapons, and return the capture. The old three-slot Weaponry menu is deliberately still present; the multi-placement feature is not shipped or claimed complete.

Validation: existing Lua suite, dedicated capture lifecycle/error/size-limit tests, pure native child-list traversal tests (including missing memory, wrong owner and cycles), PE32 x86 build with warnings as errors. No running-client validation performed here.

## First running-client capture: two-hander and gun (3.3.75)

Preserved locally at research/weaponry-captures/twohand-gun-3.3.75.txt (SHA256 72133cf29c71ed0c345b08d4387e42d73abdda23e8f7054fec3c8e873997b735), with parsed transition summary alongside it. Thirty-one changed snapshots; capture ends normally.

Observed:

- Real main-hand inventory reports item 5581, Smooth Walking Staff (inventory type 17, class 2, subclass 10). Off-hand and ranged inventory links are absent throughout. The user confirmed the gun was selected only as a Closet appearance, not actually equipped. This explains the empty ranged inventory slot. The probe does not record the addon's selected appearance IDs, so the gun's item identity cannot be determined from this file.
- The same child instance moves from attachment 30 (resolved index 18) to 1 (index 1) and back, repeatedly. Its sampled refcount remains 2. The first moves occur at 1.73/2.77 seconds and repeat at 6.01/6.61 seconds. This validates reuse during ordinary draw/sheath for this example, not arbitrary new attachment ownership.
- Raw unit +0xd40 takes 0, 1 and 2. In this capture, 0 corresponds to resting/sheathing, 1 to melee draw, and 2 to ranged draw. Child movement follows the state change by about 0.30–0.31 seconds in the sampled sequence. Do not move models immediately on the raw state transition: preserve the native animation's handoff timing.
- During state 2, the staff returns to attachment 30 at 25.05 seconds; an additional child appears at hand attachment 1 at 25.56 seconds and disappears after state returns to 0. With the user confirming an appearance-only gun, this is consistent with that gun being drawn. The file still does not identify its exact asset or establish a gun back placement.
- The parent model changes at 22.98 seconds. Its former address is reused as a child under the new parent. Registries must invalidate ownership on destruction/replacement; addresses alone are not durable identities.
- All sampled child-parent relationships pass validation. Points 16 and 17 also carry short-lived children; their asset identities are unknown. Do not classify them as quivers merely from an attachment number.
- All generic virtual-display/info fields read by the probe are zero despite the visible staff; routing cannot depend on those fields for this player. Keep using the real inventory item identity as requested, and inspect the player's native virtual getters if renderer-side metadata is needed.

Follow-up local disassembly identifies 0x60b590 as the native move path (thiscall, two stack arguments, ret 8). It chooses storage/hand points via 0x47a070, finds the existing child, retains it (0x710390), detaches (0x713020), attaches (0x712f70), and releases the temporary reference (0x7103a0). It also updates the hand pose. This is the relevant transition path; the earlier 0x47a0c0 loader alone was insufficient. Evidence retained in research/weaponry-captures/native-draw-move.asm. No new hook has been enabled yet.

## 3.4.24 stored bow height

Bows (weapon kind 4/subclass 2, including Laminated Recurve Bow 2507) at
ranged-back point 27 receive a -0.35 model-unit translation along their
parent character's up axis. Guns, crossbows, quivers and hand attachments
retain their existing transforms. The adjustment applies only to a registered
Closet extra or routed ranged child, in the world and independent previews.

Exact-build evidence: CM2 update 0x714260 takes a matrix, scale vector,
lighting vector and float (thiscall, ret 16). It multiplies its local +0xBC
matrix by the input into +0xFC at 0x714389. The recursive child call at
0x71875C receives the animated attachment matrix. The hook only adjusts a
local copy at that call site (return 0x718761), using the parent's +0xFC up
basis so character scale/rotation are preserved. Shared model data, bone
positions and stored child matrices are never overwritten. No accumulation
or additional model reload occurs; moving to a hand automatically bypasses
the adjustment. Both new hook/callsite signatures are verified.

Tests exercise the actual adjustment with Laminated Recurve, gun exclusion,
world and preview ownership, draw/sheath-point exclusion, unrelated children,
rotated/scaled parent bases, unchanged input and repeated-frame stability.
ASan/UBSan simulation passes. The 0.35-unit visual offset is an initial
adjustment based on the reported high Orc placement; in-game visual fitting
remains to be checked.

## 3.4.26 stored bow centering

The user confirmed the height correction worked; the original placement
was also left of center. Keep the -0.35 local-Z offset and add -0.20 local-Y
(toward character-right) using the parent matrix. This is a separate lateral
adjustment, not a reversal of the height correction. Scope and draw behavior
are unchanged. Simulation verifies both offsets, unchanged inputs and centering
that follows a rotated/scaled character. The exact visual fit needs an in-game
check, particularly the reported Orc/Laminated Recurve combination.

## 3.4.29 model-authored bow back placement

Replaces the 3.4.24/26 universal lateral/vertical translation. The old point-27
position minus (0, 0.20, 0.35) cannot center all models: original Y is about
0.167 for male Gnomes, 0.359 for male Tauren, and 0.194 for female Night Elves.
The correction also ignored each animated spine's movement.

Vanilla bows such as 2507 have sheath type 0, so the client supplies no dedicated
stored-bow point. Closet continues using logical point 27 for its ranged slot,
but now places the bow grip at the character model's authored center-back anchor
(point 28, normally used for a sheathed shield). Only the translation comes
from that anchor; the bow retains point 27's animated orientation. A shield
still owns its independent logical point 28; there is no reattachment, extra
reference, player-field write, or change to ranged draw/callback behavior.

`tools/audit_bow_attachments.py` reads the installed patch-2, patch and model
archives in priority order. All 16 playable race/gender M2 files contain point
28. It validates the lookup, actual ID, bone index and coordinates against
`tests/bow_attachment_fixtures.h`. No per-race offsets are embedded in the DLL.
The active parent model supplies its own lookup and animated bone matrix, so
race overrides, world models and independent outfit previews resolve correctly.

Exact build-5875 layout verified against 0x712CB0, 0x712DE0 and the recursive
update at 0x71868F..0x718756: model+0x30 -> data+0x130 -> M2 header; attachment
array at +0x104/+0x108, lookup at +0x10C/+0x110, 48-byte attachment records,
bone index at record+4, position at +8; animated 64-byte matrices at model+0x94.
The parent has completed bone evaluation before recursive child updates. The
replacement translation uses that same space; calling the public position API
inside this hook could recursively update the parent and also applies a scene
transform, so the hook only reads the already-computed bone data. Bounds,
missing-point, ID, finite-value and matrix checks retain the stock placement
when data is unavailable. No new native calls or hooks are introduced.

Validation: all 16 installed model fixtures; actual hook simulation under
ASan/UBSan for every model in world and preview contexts, with scale, rotation,
body movement, preserved orientation, repeated-frame stability, missing data,
draw exclusion, non-bow exclusion, identity checks and existing ownership tests.
Weapon routing tests, 37,803 Lua 5.0.3 assertions, real-body regression, installed
executable signatures and Windows x86 DLL build all pass. This establishes the
new anchor selection and transforms; live clipping/visual fit still needs an
in-game check and is not claimed from the simulations.

## 3.7.8 — bags inherited by stock character previews

The existing CM2 clone hook records weak model/GUID identities for unbracketed
clones originating from the local player's bag-bearing model, including clones
of those previews. At recursive attachment draw, only the allowlisted backpack
mesh on attachment 28 receives the existing bag fit. The fit reads the copied
actor's bones, local child matrix and model/view transform; no world animation
history is reused. Missing bones hide the copied bag until a valid fit is ready.
The existing destruction hook removes these weak entries without retaining or
releasing cloned children. Explicit addon preview ownership remains unchanged.

The native simulation checks all 16 race/sex fits, camera rotation/translation
and zoom, nonidentity child transforms, saved tuning, repeated opens/destruction,
copy-of-copy provenance, unrelated models and shields, GUID changes, and pending
bones. Address/undefined-behavior sanitizer checks pass (leak detection disabled
because the execution sandbox uses ptrace). Actual visual confirmation remains
an in-game check after restarting with the rebuilt DLL.


## 3.7.9 — closer selected-staff back fit

Explicitly selected staffs at native back attachments 30/31 receive a bounded
inward translation after normal bone evaluation. Simple mode's main-hand route
and Advanced carried-back extras share this fit; held points, unselected native
weapons and other players are excluded. No mesh reload, bone ownership, route,
orientation, scale or metadata changes are needed. Existing placement tuning
applies after this base fit.

`tools/audit_staff_fits.py` measures neutral rear-body clearance along each
stock staff attachment's authored shaft path, through the torso, for all 16
playable bodies. It validates the helper-bone pose and exports numeric values
plus source hashes to `StaffFits.h`. Runtime matching uses the active model's
attachment coordinates. `tools/audit_staff_shafts.py` measures transverse bounds
within a torso-contact band (weapon-local X -1.0..0.15) for all 44 staff meshes.
The union includes triangle intersections at band boundaries; decorative heads
outside that band do not force an unnecessary gap. No client meshes are bundled.

The runtime subtracts transformed shaft thickness and a 0.012-unit air gap,
clamps the inward movement to 0.12 actor units, and never shifts an already-tight
staff outward. Fitting occurs in actor space along the animated torso's inward
axis, then returns to render space, keeping preview zoom and world transforms
independent. Unknown geometry or invalid matrices retain native placement.
These measurements establish a conservative neutral fit, not collision
avoidance in every animated pose. Live visual confirmation needs a full game
restart with this DLL.
