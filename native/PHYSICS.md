# Physics controls

Wardrobe → Physics contains separate Bag Physics and Weapon Physics sheets.
Bag amplitude (0–200%), per-bag on/off, per-bag Default and all-bags Default
retain their behavior. Default restores physics on at 100% without changing fit.
Vanilla sliders use EnableMouse and dimming, not Button-only Enable/Disable.

Weapon physics is an opt-in per-character setting, off by default. Its Default
turns it off. The renderer uses the existing rigid bag spring, independent motion
history per weapon and restrained gravity-relative jump lift. Weapons use the
standard bag response with 1.5× secondary amplitude, so quiet torso mounts still
show bob and rocking; model size and animation frequency are unchanged.
It applies after normal bow/staff/back placement corrections and saved stowed fits.
Only the final rigid attachment matrix changes: no bone or vertex deformation.
The current actor/render transforms and child-local fit are removed before motion
and restored afterward. Unusual nonuniform/sheared/reflected authored matrices
fall back untouched rather than being normalized into a different shape.

Only recognized, visible stowed/body-carried weapons in owned player or opted-in
shared contexts are eligible. Hands, quivers, other props and stationary previews
keep native transforms. Drawing, hiding, disabling, changing attachment points,
model destruction and context rebuilds reset motion history. Idle settles using
the bag solver's existing quiet-stop behavior. Native sheath/draw routing remains
responsible for weapon ownership and visibility.

The former cape/race replacement systems, loose-model resolver paths, generators,
assets and options are removed. Models always use their normal native animations.
Old saved cape/race preferences are cleared during physics synchronization.

The Lua bridge exports SaureksClosetPhysicsVersion (1) and
SaureksClosetSetWeaponPhysics (0/1). Sharing wire version 4 uses appearance flag 4
for weapon physics; bytes 85–87 are reserved again. Older wire versions are
rejected so retired cape flags cannot accidentally turn weapon physics on.

Tests cover real attachment routing plus running/walking response, rigid shape,
idle, jumping, duplicate frames, camera/root transforms, ownership, hand exclusion,
previews, toggle/reset, remote settings and the removed feature/package boundary.
These are renderer simulations; final visual tuning still requires in-game review.
