# Mount appearances

Open **Wardrobe → Mounts**, search by name or browse the icon grid, then click an
icon. Hover an icon to see its full name. **Original** restores the appearance of
the mount you actually summoned. The main **Save** button stores your mount
choice with the active look; switching looks restores that look's choice.

Selections apply when you use an actual mount. Your mount spell, speed and
movement permissions are unchanged. The override affects your local view of your
own character. It clears on dismount, taxi travel, addon disable, and leaving the
world. Selecting an appearance while on foot does not summon a mount.

The catalog includes 109 distinct appearances from all 126 mounted-aura spells in
the Windows 1.12.1 build-5875 client, including retired, event and test appearances.
Repeated spells sharing one display are combined. Later-expansion models and
server-custom mounts are outside this catalog. Normal mount icons work across
locales; shared-icon event/test mounts additionally require the English mounted
aura name from this client's Spell.dbc.

Requires VanillaHelpers exposing `SetUnitMountDisplayID`, `UnitDisplayInfo` and
the original client's buff APIs. No new SaureksCloset DLL is required. The
implementation releases the helper override with the omitted optional argument,
restoring its retained server value, rather than assigning a fake mount ID of 0.
Only a real mounted aura plus a positive mount display enables the override.
Normal mounted-state checks never infer mounted status from our overridden
display alone. A dedicated hidden tooltip handles ambiguous mount aura icons;
it does not alter the user's visible tooltip.

Catalog generation: `tools/build_mount_catalog.py --dbc <extracted DBC directory>
--sql <pinned classic SQL gzip>`. Inputs are Spell, SpellIcon,
CreatureDisplayInfo and CreatureModelData DBCs, plus the same pinned CMaNGOS
database used by the equipment catalog. `MOUNTS-SOURCE.json` records input
checksums, source revision, spell-to-display mappings and model references.
Data notices are in **CATALOG-LICENSE.md** and **CATALOG-COPYRIGHT.md**.

Tests cover icon paging/search/selection, independent saved looks, on-foot safety,
mount/dismount, native original restoration, taxi travel, disable/enable, zoning,
reload ownership, API failures/retries and catalog completeness. In-client visual
validation remains necessary for mount-specific rider positioning and scales.
