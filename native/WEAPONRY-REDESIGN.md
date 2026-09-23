# Weaponry and embedded page controls (bridge 30608)

This working copy combines the GitHub native source with the newer locally
installed addon. Do not replace its addon files with the older GitHub versions.

## Default behavior and explicit changes

The addon formerly populated empty custom positions from equipped items in
`EffectiveWeapons`. This made an unchanged ranged slot an explicit back
override, including wands. Empty selections now remain empty. The native game
owns their placement, drawing and sheathing. Selecting Game Default removes
that position's override. Primary is the normal back position; Alt is the
second back position.

Only explicitly selected ranged appearances participate in cross-family
routing. The real equipped ranged slot must exist. Visual consumers of the
player virtual-item getter at `0x5EC240` receive a private copy of the selected
appearance's subclass, inventory and sheath data. The eleven permitted return
addresses are listed in `weaponInfoAt` and independently signature-checked.
All other consumers receive the real item information. No player descriptor,
inventory, skill, spell, damage or ammunition values are changed.

The unit animation scheduler at `0x5FE2F0` maps only the equipped ranged
family's ready/attack/load/hold phases to the selected appearance's family.
Movement, melee, casts and other units are untouched; the stock scheduler
retains timing and completion. Animation IDs are documented at
<https://wowdev.wiki/M2/AnimationList>. Cosmetic changes do not change a wand
spell into a gun ability or change its projectile damage/effects.

## Placement fitting

The existing bag fitting bridge now accepts target 1 (bag), or 101–107
(weapon positions including quiver). Each target has separate race/sex fits.
Weapon defaults are neutral with 100% scale. Stored custom attachments use a
fresh adjustment on each attachment update; drawn weapons are never shifted
by stored-placement tuning. This uses existing attachment updates, not model
reloads. The quiver horizontal option is no longer shown; old saved poses are
retained for compatibility and can be adjusted with the tuner.

Right-click an item slot for Placement Tuner. A weapon must first have a custom
selection. Older bridges expose bag tuning only and require a full restart
with bridge 30608 for weapon fitting.

## Embedded layout

Weapons, Race/Body and Bags use content-sized 121px-wide panes beginning at
(19, 80). Their stock dialog backgrounds are tiled, untinted and translucent.
The pane stays wholly within the window. The actual 26px left strip of the
character window's TopLeft and BottomLeft artwork is cropped and redrawn above
the pane. This preserves the real metal window edge instead of faking the join
with a shaded mask. Controls have consistent outer and inter-group padding.
The tabs end well before the bottom wardrobe selector at y=392 and remain below
the header.
Section headings replace floating framed cards.
Each side tab uses the stock dialog background at partial opacity and the stock
tooltip trim on its three exposed sides. Page actions remain in their established
top-right positions: Body Disable, Weapon Settings and Bag Tuner. Weapon Settings
is a separate neighboring page; stored-weapon
visibility belongs there. Detailed item selection and placement tuning remain
explicit secondary tools.
The Body tooltip remains “Disable all race & body changes”.

## Verification and installation

Automated checks cover Lua syntax, embedded control bounds, unmodified slots,
all 16 bow/gun/crossbow/wand pairs, native drawing/sheathing and metadata
isolation, animation phases, all 112 weapon target/race/sex identities,
placement camera invariance, persistence, and previous armor recovery and
world-preview scenarios. Native simulations run with address and undefined
behavior sanitizers (leak detection disabled for the sandbox).

These are simulations, not an in-game rendering test. Verify firing animations,
wand/gun hand alignment, and final UI appearance in the 5875 client after a
full exit and restart. `/reload` cannot load the new DLL. Original installed
files are preserved in the workspace's `backups` directory.

## 3.7.0: explicit in-use and carried appearances

Bridge 30700 adds independent role appearances at Lua positions 108–110.
The original 101–107 selections are carried decoration in independent mode.
The optional SetWeapons parameters 17–19 contain the three in-use IDs,
20 is the independent-mode flag, and 21 selects a wardrobe preview pose
(0 stowed, 1 melee, 2 ranged). Old callers retain their automatic routing.
The native unit, spell, inventory, and equipment fields are unchanged.
In-use children are hidden at their storage points; independently owned
carried children stay visible and never participate in hand routing.
Preview children have separate ownership and pose state; world calls ignore
the preview pose. Clearing in-use choices restores the stock appearance.

The former visual metadata allowlist missed two additional native consumers:
return 0x5FD4A5 selects the ranged reload/hold animation (writes animation IDs
105/106/112/111), and 0x5FE019 chooses rifle versus thrown attachment handling
during visual animation updates. Both call sites were disassembled and their
12-byte prefixes verified against the pinned 5875 executable. They now receive
the same private cosmetic metadata as the composer. Other callers still get
real equipment data. No new native detour is required.

Explicit roles require a nonempty real equipment slot but do not require its
ID to be in the cosmetic asset catalog. This supports server-specific real
weapons. The original native metadata getter supplies the equipped ranged
subclass for animation mapping when the real item is absent from the catalog.
Known melee types still must match; ranged families may differ.

The UI has four separated left tabs, a single-column item list, explicit
in-use/carried descriptions, corner placement cogs, uniform context-menu
rows, and wardrobe-only Stowed/Melee/Ranged controls. Tests cover all sixteen
ranged family pairs, a server-specific wand, independent back/hand meshes,
native callbacks, repeated updates, reset, preview poses and cleanup, saved
look migration, disabled-state drafts, tab switching and bounds.


## 3.7.3: one carried-display mode

SetWeapons parameter 22 is now the optional carried-display switch. Omitting
it retains the previous API behavior; explicit 0/1 selects the new independent
attacking/carried behavior. The addon requires renderer 30703 for nonempty
weapon settings and keeps empty clears compatible with earlier DLLs.

With the switch on, native attacking weapons disappear at stored attachment
points, including when no decorative choice occupies that point. Chosen carried
weapons and quivers remain attached during attacks. With the switch off, the
addon preserves its saved choices but submits no carried items, and selected
attacking appearances use their native sheath positions. Native quivers return;
the separate bag option is unchanged. The old per-family hide flags apply only
to callers that omit parameter 22.

Observed native weapon children are tracked through the existing composition
hook and invalidated by the existing destruction hook. This identifies
server-specific weapons without suppressing unrelated props by attachment
point alone. Mode changes rebuild native weapon composition once; unchanged
synchronization is idempotent. No new client detours were added.

The Weaponry page presents the switch beside the attacking slots. Redundant
item/equipment status labels and the two hide-on-back checkboxes were removed.
Passthrough restores an attacking slot to its real equipment appearance.
Disabled carried choices remain visible but cannot be edited until enabled.
