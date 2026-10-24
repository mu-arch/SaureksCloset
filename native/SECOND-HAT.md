# Second cosmetic hat, build 5875

The head-slot cog opens the normal searchable appearance browser for virtual
slot 112. It reads the head catalog, but never sends 112 to inventory, TryOn or
SetUnitVisibleItemID. `secondHat` stores the selected item ID in the current
look's settings. Preview/cancel/Apply, Save, discard, defaults and look switching
use the normal working-copy lifecycle. Missing settings mean no second hat.

`SaureksClosetSecondHatVersion()` returns 1.
`SaureksClosetSetSecondHat(previewToken, displayID)` returns 1 when ready, 0
while loading, -1 for an unavailable/unowned model, or -2 for invalid input.
Token 0 targets the local player; other tokens require an owned, ready preview.
Display 0 removes only the additional hat. Shapeshifts clear its attachment.

The renderer follows the existing helmet compositor at 4799A0: ItemDisplayInfo
(C0DC10) model at +4 and texture at +C, ChrRaces (C0DEE0) race suffix at +18,
sex suffix M/F, and attachment 11 through the existing 4798C0 factory. Rows and
bounded strings are read only; only Helm_ models are accepted. Native equipment,
item fields, helmet geoset masks and the character-menu slot remain unchanged.
The extra child is retained separately in its owned model context. Clear/find
hooks preserve it while leaving the first helmet discoverable by native code.
Weapon and bag synchronization retain hat-only contexts. Repeated setters do
not reload the child; destruction, replacement and removal release its reference.

Addon preview clones temporarily omit the extra child, then dress their own
selection, preventing duplicates and cross-look leakage. The temporary detach
holds an extra reference and restores the source even if cloning fails. Stock
Character-window clones retain both visible hats. The first hat's placement
and automatic hair-trimming surface remain independent of the added hat.

`tests/second_hat.cpp` exercises the actual hooks/bridge, native clears, loading,
clones, all 16 body suffixes, isolation and reference cleanup. The Lua tests
cover current-look persistence, drafts, Save, removal, failures, disabled/old
DLL paths, and cog/picker navigation. Tests use client-shaped mocks; final
appearance and any clipping between particular hats require an in-game check.
