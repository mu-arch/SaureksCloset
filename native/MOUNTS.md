# Mount summon sounds — build 5875

The Lua mount picker still uses VanillaHelpers' SetUnitMountDisplayID solely for
appearance. The added SaureksClosetSetMountSound bridge arms the current look's
mount selection while on foot, before the spell's precast and arrival events.
Original, addon disable, taxi/death and leaving the world clear the selection;
dismount retains it for the next summon. No Lua sound playback or timer guesses
are used. Older DLLs retain mount appearances and show an update/restart message.

The existing 60EDF0 visual-kit dispatcher first checks for a local-player spell
with an actual APPLY_AURA/MOUNTED effect (6/78). A canonical current mount spell
for each display comes from the existing pinned mount catalog; later spell rows
win over retired duplicates, including Kodo's corrected summon sound. Exact
Spell.dbc rows and visual-kit membership are validated before substitution.

Only SpellVisualKit field 13 (+34h, SoundEntries ID) changes in a private copy.
The incoming spell pointer, phase, arguments, animations, effects and every other
kit field remain native. Precast, cast, impact and aura kits are distinguished
by their actual visual membership. For mounts that put arrival in an aura kit,
the source primary arrival event uses the selected primary kit's sound. Old
secondary layers are silenced when the selected mount has no corresponding kit.
No extra art, sound event or mount spell is invented. Timing, interruption and
cast failure handling remain owned by the original client dispatcher.

The native read at 60EE3D and sound dispatch at 60F48A verify +34h; both are now
signature-guarded alongside the existing hook. Private kits have process lifetime
because client effects retain their pointers. Changing looks or disabling never
invalidates them. Remote units, nonmount spells, transformed characters and
unavailable DBC data take the unchanged path. Weapon casting still handles
nonmount spells through its existing hook. No game-memory or DBC row is written.

Regenerate numeric mappings/fixtures using:

    python3 tools/build_mount_sounds.py Spell.dbc SpellVisual.dbc SpellVisualKit.dbc

The Spell hash must match MOUNTS-SOURCE.json. Fixture headers pin all three DBC
hashes; no audio or full client tables are distributed. mount_catalog.py checks
all 109 display mappings against the original mount catalog. mount_sounds.cpp
runs the actual combined hook over all 126 real mounted spells and all selected
appearances, including sound-only field changes, retained pointer lifetime,
missing data, owner isolation and ordinary weapon-casting coexistence. mounts.lua
checks early arming, saved looks, dismount, taxi, disable, zoning and old DLLs.
These simulations do not replace listening to mount summons in the game.
