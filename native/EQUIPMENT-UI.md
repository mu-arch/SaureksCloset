# Equipment UI isolation (4.1.1)

The 1.12 client sometimes obtains inventory icons from its visible equipment
records before reading real inventory. VanillaHelpers armor appearances modify
those visible records. As a result, the Character window can show cosmetic
goggles in an empty head slot after a refresh. The inventory-tooltip method also
falls back to those records when there is no real item, producing a cosmetic
tooltip for an empty equipment slot.

The renderer now isolates these two presentation paths at the verified
build-5875 visible-record getter, `0x5F0D60`:

- Return address `0x4C8428` is the icon getter. Returning no visible record makes
  the original client resolve the actual inventory item through its existing
  fallback at `0x4C843C`. Empty inventory stays empty.
- Return address `0x533319` is the tooltip's empty-inventory fallback. Returning
  no visible record preserves the client's normal `nil, nil, 0` result for
  item presence, cooldown and repair cost. Real-item tooltips already use the
  actual item and retain their enchants, durability and repair information.

The filter applies only to the current player's verified object and valid
equipment indices. All other callers and other players use the original
getter. In particular, world armor, the normal Character window's 3D model,
Closet previews and preview dressing retain their cosmetic appearances.
No equipment fields, Lua inventory APIs, tooltip methods, stats or equipment
button handlers are rewritten. The fix does not depend on frame refresh order,
selected-look caches or item-info availability.

The hook and both call sites are protected by executable signatures. Native
dispatch tests check both UI paths, real inventory fallback, empty slots,
other units and renderer callers; stock PaperDoll integration tests check slot
icons, occupied state and tooltips across equip/unequip and cosmetic changes.
The stock FrameXML used by those tests is a local client reference and is not
included in release archives. A full restart is required to load renderer 30713.
