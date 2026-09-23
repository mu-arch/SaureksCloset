# Weapon appearances — 3.7.11

The full Weaponry page has two modes. The **Advanced mode** checkbox switches
between them without erasing any appearance or placement settings.

## Simple mode (Advanced mode unchecked)

Three large rows match your real equipment slots: **Main hand**, **Off hand**,
and **Ranged**. Choose each slot separately. Two equipped short swords have two
independent appearance choices; a shield uses the off-hand row. Empty slots are
unavailable, and a two-handed main-hand weapon makes the off-hand row unavailable.

The item picker only lists appearances supported by that equipped slot. Melee
weapons retain their broad one-handed, two-handed, or shield category. Ranged
appearances can cross bows, guns, crossbows, and wands. Weapons draw and sheath
normally. The carried section is hidden in Simple mode.

Right-click an equipped slot and choose **Passthrough** to use the real item's
appearance. Its icon then shows the real equipped item. Hover for item details.

## Advanced mode (checked)

**In your hands** has separate main-hand and off-hand choices side by side,
with ranged below. These still follow real equipped slots.

**Carried on your body** separately chooses left/right waist, left/right back,
shield, ranged, and quiver appearances. These decorative slots do not require
matching real equipment. Each icon's corner cog opens its placement editor.
Full item details are in the hover tooltip, without repeated subtext on the page.

Carried choices replace normal sheathed weapons and stay visible during attacks.
Hand weapons disappear when put away; empty carried slots stay empty. For example,
a gun can appear in your hands while a decorative bow remains on your back.
Switch back to Simple mode to sheath the gun normally and hide the decorative bow.

Existing looks retain their previous behavior: looks with carried display on open
in Advanced mode; looks with it off open in Simple mode. New looks start Simple.
Switching modes or changing real equipment never erases saved appearances.

Bows and crossbows fire arrows, guns fire bullets, and wands use the selected
wand's bolt. Loading and firing sounds follow the selected weapon. Ordinary spell
casts, real equipment, abilities, damage, ammunition, projectile speed, targeting,
spell results, and other characters are unchanged. The separate Bags page is
unaffected.

## Changes in 3.7.7

The equipped-slot explanation sits directly beneath its heading, with extra
padding above the section. Advanced mode has no inline subtext; its tooltip
explains the carried-item behavior.

Armor appearances recover after individual repairs and Repair All, including
when only the rendered textures reset. Repair detection uses the original
client's inventory-tooltip repair cost and inventory/merchant events. Damage,
equipment swaps and ordinary weapon drawing do not trigger that texture refresh.
Active item previews and saved appearances remain intact.

The large window portrait uses the supplied mannequin artwork, cropped to its
circular background and exported as a 64 x 64 RGBA TGA (16 KiB texture memory).
The minimap icon is unchanged. Restart WoW if the old window icon is cached.

## Changes in 3.7.8

Bags copied into the regular character-screen preview now receive the fitted
size and position used by the world bag. The renderer uses the preview's own
bones and camera transform, respects saved bag placement, and keeps world
running/jumping motion out of the character sheet. Native shields are unaffected.

## Changes in 3.7.9

Selected staff appearances fit closer to the back while sheathed, in both Simple
mode and the Advanced carried-back slots. The fit accounts for the character's
back and the staff shaft's thickness, keeping its normal angle and height. Staffs
that already sit close to the body keep their normal mounting point. Hand poses,
Passthrough weapons and saved placement adjustments retain their behavior.

## Changes in 3.7.10

Equipped weapon slots now starts near the top of the Weaponry page, with room
above its heading and description. Advanced mode sits at the bottom below both
the equipped and carried choices, leaving their cards unobstructed.

On Body only, the character preview moves slightly right and frames the bust at
a closer scale. Its lower half fades smoothly into the existing window artwork.
The usual full-body view returns on Outfit, Bags and Exposure.

## Changes in 3.7.11

The Body-only character preview now zooms more tightly on the face, shoulders
and upper torso. Its existing lower fade, position, and all other wardrobe page
framing stay in place.

## Installing this update

Addon 3.7.11 requires renderer 30711 (DLL 3.7.11). Install the included DLL beside
WoW.exe, then fully quit and restart WoW. A UI reload cannot replace a loaded DLL.
Version Details should show addon **3.7.11**, loaded DLL **3.7.11**, and required
DLL **3.7.11**.
