# Haircraft

Wardrobe → Haircraft offers **Keep hair: On/Off** and **Default**. This is a
per-character preference (`keepHairWithHat`), initially off. It uses the current
native or customized hairstyle without replacing the selected hat. Default
restores ordinary hat visibility. Disabling the addon suspends the effect while
retaining the preference. The registered wardrobe previews use the same policy.
It does not reshape hair to fit helmets; overlapping geometry remains possible.

Build 5875 evidence (verified against the installed executable):

- `4799A0` attaches the head model, then applies HelmetGeosetVisData. The row is
  selected by ItemDisplayInfo + `30 + sex*4`; its hair race mask at +4 replaces
  component + `144` with group 1. Later masks control facial hair and ears.
- `478540` resolves the correct hair group from the appearance descriptor's
  race, sex and hairstyle. `4784C0` uses it when selecting hair, and `478960`
  uses it when removing a helmet. Bald/unmatched hair returns group 1.
- The hook runs the original helmet compositor, then restores only group `144`
  using that same resolver on component + `18`. It never modifies the shared
  DBC row, helmet attachment, facial groups, inventory fields, or saved hair.
- Ownership is restricted to the local player's ordinary body or a registered
  Closet preview belonging to that player. Other players and shapeshifts use
  their normal visibility. This preference is not sent through look sharing.

`SaureksClosetSetHaircraft(0|1)` is idempotent and rebuilds the local compositor
only when the effective preference changes. Lua invalidates its preview on a
successful edit and reapplies the preference during normal synchronization.
Old DLLs disable the controls and show an update instruction.

Mandatory tests cover the UI/settings lifecycle, repeated helmet composition,
bald and selected hair groups, restoring defaults, unchanged facial groups,
and local/preview ownership. Executable signatures guard the native functions
and the verified hair-mask instructions. Actual hat/hair overlap still needs
visual evaluation in the running game.
