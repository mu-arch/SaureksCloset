# Haircraft

Wardrobe → Haircraft offers **Keep hair: On/Off** and **Trim hair**. This is a
per-character preference (`keepHairWithHat`), initially off. It uses the current
native or customized hairstyle without replacing the selected hat. Turning Keep hair off
restores ordinary hat visibility. Disabling the addon suspends the effect while
retaining the preference. The registered wardrobe previews use the same policy.
Without an enabled mask it restores the whole hairstyle; overlapping geometry remains possible.

Build 5875 evidence (verified against the installed executable):

- `4799A0` attaches the head model, then applies HelmetGeosetVisData. The row is
  selected by ItemDisplayInfo + `30 + sex*4`; its hair race mask at +4 replaces
  component + `144` with group 1. Later masks control facial hair and ears.
- `478540` resolves the correct hair group from the appearance descriptor's
  race, sex and hairstyle. `4784C0` uses it when selecting hair, and `478960`
  uses it when removing a helmet. Bald/unmatched hair returns group 1.
- The hook runs the original helmet compositor, then restores only group `144`
  using that same resolver on component + `18`. It never modifies the shared
  DBC row, helmet attachment, facial groups, inventory fields, or saved hair.
- Ownership is restricted to the local player's ordinary body or a registered
  Closet preview belonging to that player. Other players and shapeshifts use
  their normal visibility. This preference is not sent through look sharing.

`SaureksClosetSetHaircraft(0|1)` is idempotent and rebuilds the local compositor
only when the effective preference changes. Lua invalidates its preview on a
successful edit and reapplies the preference during normal synchronization.
Old DLLs disable the controls and show an update instruction.

Mandatory tests cover the UI/settings lifecycle, repeated helmet composition,
bald and selected hair groups, restoring defaults, unchanged facial groups,
and local/preview ownership. Executable signatures guard the native functions
and the verified hair-mask instructions. Actual hat/hair overlap still needs
visual evaluation in the running game.

## Hat placement

**Adjust hat** opens the shared precision tuner without leaving Haircraft. It
offers live lateral/longitudinal/vertical offsets, three rotation axes, size,
relative editing, individual resets, Save Fit, Load Saved, and Default. Target
111 uses separate `hatTuningEntries`; weapon entries and sharing's weapon fit
mask retain their original ten slots. Fits follow the existing account profile
format, keyed by race and gender, and apply to hats worn with that body. They
work independently of the keep-hair switch. The overall addon/live-tuning
switches suspend placement overrides. Save Fit persists an edit; Default
previews the native placement until saved.

The attachment dispatcher adjusts only point 11 children with a
`Item\ObjectComponents\Head\` resource path and a current local-player or
registered Closet-preview parent. It composes the fresh attachment matrix with
the fit, preserving its animated head transform and camera transform. Each
update, including lazy updates, starts from the native input; there is no
accumulated offset, dynamic motion, head-bone edit or model reload per nudge.
Head spell effects, weapons, other players and transformed world bodies retain
their native placement. Capability probing of target 111 disables Adjust hat
when an older DLL is loaded.

`tests/hat_tuner.lua`, `tests/hat_placement.cpp`, and the actual hook simulation
in `tests/weapon_renderer.cpp` cover persistence, resets, body isolation, live
dispatch, default/disabled behavior, camera invariance and lazy updates.

## Precomputed hair mask

**Trim hair** opens a shared character sheet (the Haircraft Default button has
been removed). The user-facing mode now cuts all hair above one explicit plane,
with **Cut height**, **Forward tilt**, and **Side tilt** controls. Cut height is
0–100% of the hat's vertical bounds; tilts are -80 to 80 degrees in the hat's
coordinate frame. The initial height is 35%. Apply precomputes intersections
and keeps the portion of each hair triangle below the plane. Lowering the plane
cuts more hair. It does not attempt to infer which protrusions belong inside a
crown. The earlier 16-sided crown mode remains supported by the native API for
compatibility, but is no longer exposed in the UI.

Applying an enabled mask also enables Keep hair. Turning the mask off restores
the complete hairstyle without disabling Keep hair. Profiles are character
settings keyed by race, sex, hairstyle and active head item, including transmog.
An old profile's lower cutoff becomes its new plane height. Switching identities
discards unfinished edits. Hidden hats and a disabled addon suspend the mask.

`SaureksClosetHairMaskVersion()` returns 2. The version gates the new controls.
`SaureksClosetSetHairMask(on,width,depth,top,cutoff,mode,height,pitch,roll)` returns
status, crossing-triangle count and preview generation. The first five arguments
retain their legacy meaning; mode 1 selects the explicit plane and its three
additional values. Status 0 is pending capture, 1 is applied/off, 2 means no
visible hat, 3 means waiting for the trimmed model to load, -2 invalid inputs,
-3 invalid bounds, -5 bake/file failure, and -6 failure to load within five
seconds. A successful file write alone no longer reports "applied": the live
model resource must match the private path. The Lua bridge invalidates its
preview only when generation changes.

The existing attachment dispatcher captures a loaded local head-equipment
model once when requested. It removes the current head skin matrix from the
actual hat attachment transform, including target-111 tuning. It does not
capture spell effects or another character. Once captured, the render hook
returns immediately; it does not scan meshes or solve collisions each frame.
The ordinary Lua synchronization path validates the current hat/body/style/fit
and bakes only when those, the mask parameters, or cape motion change.

`HairMask.h` clips polygons at the selected plane, interpolating position,
normals, texture UVs and skin weights at new edges. The older crown mode
estimates its envelope from the upper 65% of hat vertices. Every M2 view receives valid new indices, vertex
properties and bone-palette mappings. Only sections matching the active hair
geoset are replaced. Body/face/ear sections, skeletons, animation tracks,
materials, textures and attachment records remain intact. Model bounds remain
conservative. Invalid/oversized data fails without loading a partial model.

The bake uses the shipped B01-B16 source models, composing with an active cape
motion bake when necessary. Output is a content-addressed `H1_*.m2` under
CapeMotion/Cache. Only exact paths generated by this process enter the loose
file allowlist. Shared resources and source files are never modified. The
world name hook uses the private model only for the local ordinary character;
registered live wardrobe previews opt in through BeginPreview's eighth argument.
Body and saved-look previews use an unmasked model to avoid inheriting a mask
for a different outfit. Masks are currently local and are not part of sharing.

Limitations: the plane uses the hat bounds and orientation, so unusual hats
can need manual height/tilt adjustment. A mask is baked in rest space, so animated
hair can still cross the plane later. It does not simulate collision or
repair holes in the hat. Actual client appearance needs visual verification.
Tests exercise all 148 hair groups in the 16 shipped bodies at every LOD,
non-hair preservation, lower-cutoff preservation, skin palettes, deterministic
bakes, runtime dispatch, fit/hat changes, ownership, UI validation and persistence.
