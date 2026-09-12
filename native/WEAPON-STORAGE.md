# Equipped weapon storage repair (3.4.31)

## Sword orientation correction (3.4.32)

The 3.4.31 storage repair changed a type-1 two-handed sword's visual pose from
point 26 to the staff-style point 30, reversing its intended stored direction.
For sheath-type-1 two-handers owned/routed by Closet at logical points 30/31,
the recursive child draw now uses the active model's full animated transform
for points 26/27. Logical ownership stays at 30/31 to prevent quiver removal;
the grip and orientation follow the original sword attachment. Hand attachments,
staff-style weapons, unrelated children and the bow correction are unaffected.
The shared attachment reader reproduces the matrix copy and local-position
transform at 0x7186A6..0x718756 without changing parent bone matrices.

The source model audit covers both sword attachment records on all 16 race/sex
models. Native tests check the full orientation and translation with animated
rotation/scale matrices, world and preview ownership, unchanged logical points,
drawn-weapon exclusion and invalid-data fallback. Renderer version: 30432.
Visual confirmation in WoW remains necessary after a full restart.

The September 12 schema-3 capture used addon 3.4.30 / renderer 30429,
an Orc male (display 51), sword 4939 and bow 2507. All seven saved weapon
positions were empty. The capture includes drawn weapons, but its later
mode-0 snapshots have no attached children. No wardrobe previews were opened.
This establishes that the reported missing weapons also occur without forced
weapon selections; it does not establish that every race was tested in game.

The build-5875 executable's sheath lookup at 0x47A070 returns -1 for sheath
type 0 (the bow's metadata), and point 26 for a main-hand type-1 weapon (the
sword's metadata). Quiver composition at 0x478B60 clears point 26 and calls
0x479C50 to load a quiver there. Native quiver hide/refresh operations and a
sword using that same point can therefore remove each other's attachments.

While the wardrobe is enabled, Lua derives unsaved fallback placements from
equipped items when no compatible custom placement already serves that role.
Two-handed weapons use 30/31, ranged weapons 27, shields 28, and one-handed
weapons use the independent waist/back positions. Position 26 stays reserved
for quivers. Unknown items remain client-managed. Legacy inventory overrides,
including explicit hides, suppress the respective equipped fallback.

World and preview requests use the same resolution. Routed preview inventory
items are skipped by TryOn, so a bow isn't dressed once by vanilla and again
by the independent renderer. Clearing custom choices restores equipped items;
fallbacks are never written into saved outfits. Disabling the wardrobe releases
the routes and returns to the client's normal behavior.

The native missing-child rebuild at 0x60B770 also needs a role-specific scope.
Storing the ranged weapon through 0x60B590 can recursively rebuild both melee
weapons. The rebuild calculates a sheath point before composition and uses it
for weapon effects afterward. Each nested role now gets its own route, and
calls involving other units do not inherit the local player's route. The new
entry point is covered by the build-signature check. Renderer version: 30431.

Validation: Lua 5.0 tests cover exact reported equipment, quiver off/on,
custom bow removal, equipment changes, disable/re-enable, hidden legacy slots,
outfit isolation, dual wield, shields and unknown items. The native hook
simulation reproduces point-26 replacement, exercises draw/store transitions,
nested melee rebuilds, other-unit isolation and idle idempotence under address
and undefined-behavior sanitizers. Native build and installer rollback tests
pass. Visual placement and client animation still require an in-game check
after a full restart; no in-game verification is claimed here.
