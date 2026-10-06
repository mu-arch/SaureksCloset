# Armor visual recovery (4.1.1)

A client model or inventory update can restore real armor while VanillaHelpers'
visible-item fields and the addon's applied cache still contain the requested
appearance. Reasserting the same item ID does not rebuild the model. Version
3.7.12 checks the compositor after ordinary field repair and scheduled appearance
syncs, including world entry and equipment changes.

`SaureksClosetInspectArmor(slot, expectedDisplay)` is read-only and returns
`status, renderedDisplay, dirtyTextures, attachmentMask`. Status is 1 for a match,
0 for a confirmed mismatch, -1 for loading/unavailable, and -2 for invalid input.
Only the current player's ordinary playable model is inspected. Preview models,
shapeshifts, unreadable memory and unfinished composition cannot request repair.

The inspector compares all eleven armor slots' composed ItemDisplayInfo IDs to
the catalog's display IDs. Hidden head and shoulders also check actual attached
armor meshes; custom shoulders check the expected mesh on each side. Spell
effects at the same attachment points are ignored. Empty shoulder sides are
valid. Positive head and cloak selections respect the client's Hide Helm/Cloak
settings. Positive head attachment absence is not used as proof of corruption.

Build-5875 evidence, guarded by executable signatures:

- `478CB0` maps inventory slots to compositor slots.
- `478AD0` stores ItemDisplayInfo pointers at component + `4A8 + 4*slot`;
  `478960` clears them; `478DC0` reads their first DWORD as the display ID.
- `479D10` attaches shoulder row +4 at point 6 and row +8 at point 5;
  `4799A0` builds the helmet at point 11.
- `477860` waits on component +8 and +10, clearing them after composition.
- Model child-list and resource-path offsets are shared with the existing
  read-only weapon probe and weapon resource checks.

Lua requests the existing single temporary-field change and full-outfit restore
only for a confirmed mismatch. The whole pair happens within one Lua update.
The effective browser draft is preserved. All-hidden, unequipped slots can use
a compatible cached catalog item for the same-frame refresh pulse. Loading retries stop after four
additional checks; a mismatch that survives repair cannot repeatedly refresh
the model until a healthy check, changed selection/state, or manual Retry
rearms recovery. Missing inspection support retains the older field-only path.
Diagnostics now record native armor results and confirmed repair reasons.

Tabards also have a native prevention path. Build 5875's `5E0830` and `5E08E0`
guild refreshes read the actual inventory item and call `5E0720` directly,
bypassing visible-item overrides. The user's saved history confirmed repeated
`19: expected=0; rendered=23140` failures. The `5E0720` hook resolves the current
local visible tabard record (zero-based slot 18) through `62EB40`, so hidden and
replacement tabards survive those refreshes without waiting for a Lua repair.
Item refreshes through `5ED700` converge on the same tabard boundary.

The `47A610` guild-texture hook also rejects late emblem responses when the
current local tabard is hidden, loading, replaced, or not a guild tabard. It
checks the current display against component row `4D0` and the guild flag at
ItemDisplayInfo + `24`. Other players, previews, transformed bodies and inventory
metadata are unchanged. No new frame loop, model reload, or player-field write
is needed. Both hook prefixes and their verified callers are signature-guarded.
`tests/tabard_visibility.cpp` exercises these refreshes and selection transitions;
it, the armor inspector, and inventory UI isolation tests are mandatory checks.

Validation uses the installed client's executable signatures, native memory
fixtures under ASan/UBSan, Lua recovery simulations and a strict PE32 DLL build.
These checks do not replace testing during combat in the running client.
This detects composed-slot and attachment drift, not every possible graphics
driver or texture corruption with otherwise unchanged compositor state.
