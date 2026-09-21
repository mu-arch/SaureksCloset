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
