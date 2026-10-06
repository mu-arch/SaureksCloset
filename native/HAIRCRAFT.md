# Haircraft

Wardrobe → Haircraft offers **Keep hair: On/Off** and **Adjust hat**. This is a
per-character preference (`keepHairWithHat`), initially off. It uses the current
native or customized hairstyle without replacing the selected hat. Turning Keep hair off
restores ordinary hat visibility. Disabling the addon suspends the effect while
retaining the preference. The registered wardrobe previews use the same policy.
Keep hair also requests automatic fitting beneath the current hat. There is no
separate trimming window or cutting-plane configuration.

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

## Automatic hair fitting

The former plane/envelope cutters are removed, including their configuration
window. A selected hair geoset can contain both hair-material and skin-material
sections: Human Female group 10 has an eight-vertex scalp patch. Cutting every
section of that geoset removed part of the head. The new implementation checks
all material batches and only fits sections using the hair replacement texture
(type 6). Skin sections and vertices shared with other sections are protected.

`SaureksClosetHairMaskVersion()` returns 3. The new call is
`SaureksClosetSetHairMask(on,3)`; old enable arguments are rejected. Disable
calls remain compatible. Lua ignores old saved cutting presets and explicitly
disables the old bake when paired with an older DLL. Keep hair is the only
switch. The current hat, hairstyle, body and hat placement determine the fit.

The attachment dispatcher captures the real hat triangle mesh and its fitted
transform once. Only opaque draw sections establish coverage; transparent
feathers and effect layers do not. The current head skin matrix is removed to
obtain the hat transform in body rest coordinates. Normal synchronization bakes
a private copy after capture; no fitting runs in the render loop.

A stock bald scalp, where present, establishes the crown reference. Rays from
that reference to hair vertices locate actual hat surfaces. Covered crown hair
is tucked under those surfaces, with clearance reduced when visible skin is
close. Visible skin is a collision floor. The hidden bald scalp is only a
reference, not a surface that the active hairstyle renders. Hanging lengths
below the crown are preserved. Missing coverage, missing reference geometry,
and impossible fits preserve the original hair rather than deleting geometry.

The fit changes only eligible vertex positions. All triangles, UVs, normals,
skinning weights, skin patches, skeletons, attachments and animation tracks are
unchanged. It does not create open cutting boundaries. Existing cape motion
bakes remain the source when active. Cached `H1_*.m2` files are content-addressed
and process-allowlisted; shared resources and source files remain untouched.
World/previews retain the existing owner/body/style isolation. Fitting is local
and is not transmitted by sharing.

Returned status is 0 while capturing, 1 for loaded/off, 2 for no hat, 3 while
loading, -2 for invalid/retired inputs, -5 for bake/file failure, and -6 if the
private model does not load within five seconds. The second return value counts
fitted vertices, not deleted triangles. Generation changes invalidate previews.

Regression tests cover all 148 hairstyle groups, skin patches inside hair
geosets, shared vertices, every LOD, unchanged topology, hanging hair, no-coverage
and impossible-fit fallback, deterministic fits, runtime capture and ownership,
opaque-section filtering, retired API rejection, old-DLL cleanup, and the
absence of a trim window under Lua 5.0.

This is a conservative rest-pose fit, not an exact mesh Boolean or an animated
collision solver. Coarse triangles, moving hair, and unusual/open hats can still
intersect. Some bodies lack a separate bald scalp and are left unchanged.
Offline inspection of the actual Human Female fishing-hat meshes supplements
the tests; it does not replace an in-game visual check.
