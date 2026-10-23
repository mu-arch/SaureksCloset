# Weapon casting, Windows 1.12.1 build 5875

An opt-in cosmetic path for the local player. Wardrobe → Casting stores its
enabled/effects/swing choices in each saved look. Missing settings mean disabled.
`SaureksClosetCastingVersion()` returns 1; `SaureksClosetSetCasting(enabled,
effects, swing, mainHandAppearance)` accepts four integer arguments and returns
1 on success, -1 while the player is unavailable, or -2 for invalid arguments.

The Lua controller supplies the real main-hand item or its equipped-slot
transmog, never a decorative carried item. An empty/hidden/unsupported slot uses
normal casting. The native side additionally verifies the current player's GUID,
unit, model, weapon model path, ownership, loaded resource and attachment table.
Polymorphed units, other players, target effects, passive effects, ranged attacks
and mount spells keep their existing paths. Food, drink, potions, bandaging,
crafting, gathering, fishing, opening and disenchanting retain native props and
gestures. Magic casting supports every animation style, including custom emotes
and kits with no animation.

## Binary audit

Addresses and calling conventions were inspected in the supported executable
identified by CLIENT-BUILD.json, using disassembly and its matching DBC/model
data. New entry points have executable-prefix checks in BuildSignatures.h.

| Address | Native ABI | Use |
| --- | --- | --- |
| 60EDF0 | unit thiscall(spell, kit, phase, arg4), ret 10h | Private caster-kit substitution |
| 60ED40 | unit thiscall(point, expires, spellID, kit, effect, flags, callback), ret 1Ch | Scope palm effects to weapon points |
| 611CF0 | unit thiscall(sheath, immediate, notify), ret 0Ch | Draw melee weapon without sending a sheath notification |
| 712CB0 | model thiscall(point), ret 4 | Validate synthetic weapon attachment points |
| 712D50 | model thiscall(output, point), ret 8 | Resolve the animated weapon's attachment position |
| 712F70 | child thiscall(parent, point), ret 8 | Attach casting art to the held weapon |
| 6EC780 | fastcall(output in ECX, unit in EDX, spellID on stack), ret 4 | Weapon projectile origin |
| 7121A0 | model thiscall(track, animation, variation, time, speed, repeat, blend), ret 1Ch | Preserve weapon pose through delayed casting loops |

SpellVisualKit has 35 uint fields (140 bytes); [1] is kit metadata,
[2] is animation, [6]/[7] are left/right palm effects. 60F199/60F1D7 dispatch
those effects to points 21/22. Other kit fields, including body effects, sounds,
shake and timing flags, are copied unchanged. Identical palm art is merged into
one weapon effect. The animation IDs are from AnimationData.dbc: Ready1H 26 /
Attack1H 17, Ready2H 27 / Attack2H 18, Ready2HL 28 / Attack2HL 19. Staves and
polearms use the latter pair. Channels hold a ready pose instead of swinging
repeatedly. The client's original animation dispatcher still owns scheduling.

Caster eligibility comes from the actual spell's resolved SpellVisual output:
precast [1] with dispatch phase 4, cast [2] with phase 0, and channel [5] with
phase 2. Channel kits have metadata type 2; treating only type 0 as a caster
silently excludes them. Blizzard uses kits 197/5390/717 and its channel kit 717
is dispatched at 612B31 with phase 2 (resolved from visual+14h at 612AC9).
The original restriction to phases 0/4 also skipped that path. These checks now
use kit membership and role, without filtering on metadata or animation ID.
Channel phase 2 additionally requires the same current spell ID, because state
auras also use phase 2 and three client visuals share their state/channel kit.
Victim, ground and unrelated aura dispatches remain unchanged. Charge/channel
roles use a weapon ready pose; ordinary release roles use a weapon swing.

The existing 60D450 visual hook also runs a casting filter on its caller-owned
output. With swing enabled, a spell lacking both cast and channel kits receives
the client's silent/empty cast kit 21. A private copy supplies the weapon swing;
no particles, missile, sound or target effect is invented. Null visual results
use a zeroed caller-owned output with just that cast kit. All existing fields
survive when a visual is present. This is disabled for other players, hidden or
unsupported weapons, passive/ranged/mount spells, and when swing is off.
Generic effects without a visual (such as triggered Well Fed bonuses) are also
excluded; the silent fallback is limited to actual class-spell families. There
are no global DBC edits or new native addresses for this extension.

Activity classification is shared by kit dispatch, visual augmentation, held
pose/sheath maintenance and projectile-origin routing. It reads build-5875 spell
metadata: category [2] (food 11/drink 59), family [160] (potions 13), mechanic [5]
(bandaging 16), trade-spell attribute 20h, seated-regeneration auras 84/85 with
aura-interrupt flag 40000h, and generic activity effects 33/50/95/99/118. This
also catches food/drink variants with category zero. A shared animation such as
UseStandingLoop is not an exclusion: Frostbolt, Conjure Water and Evocation must
remain eligible. Cups and similar props never enter the weapon-effect scope.

The model's resource is at +30h; its loaded header at resource+130h has
attachment count +104h, pointer +108h and lookup +10Ch. Attachments are 48 bytes,
with ID +0 and rest position +8. The farthest valid attachment from the grip is
used, rather than assuming attachment zero is the tip. GetPoint performs the
native bone/model transform, so effects follow the moving, scaled/transmogged
weapon. Only rest attachment IDs are cached; animated transforms are always
computed by the client. The synthetic marker is used only in private effect instances; all three
attachment consumers resolve it before calling native code, falling back to the
original palm point when the weapon is unavailable or the feature is disabled.

60EDF0 also calls 611CF0 to put weapons away. The sheath hook substitutes melee
during an eligible caster-kit dispatch and while that player's supported
cast/channel is still in progress with a valid held weapon. No global animation or DBC
row is edited. If drawing does not produce a valid held weapon, the prior visual
sheath is restored and the original kit runs. Disabling restores the visual
sheath captured before the first successful weapon cast, unless the player has
since switched away from the drawn melee state.

The initial dispatch scope alone is insufficient: the native scheduler can
later reapply a hand-casting loop directly through 7121A0 and restore the sheath
through 611CF0. Both paths now preserve the weapon for the full cast. The spell ID
at unit+C8Ch is set by 60D020 (called from 6EC2CF before the opening kit), and
60D040 clears it on completion/failure/interruption. Channels use the unit's
descriptor pointer at +110h, spell ID at descriptor+228h, as read by 60D340 and
60DD0C. These fields are read only. No guessed duration, Lua timer or persistent
cast flag is used. Once both IDs clear, subsequent pose/sheath requests pass
through immediately. Idle, movement, release, foreign models, unavailable
weapons, disabled swing and disabled feature paths keep their native sequences.
Delayed loop animation matching uses the current spell's actual precast and
channel kits (including a channel-style cast kit), so styles such as Frostbolt's
UseStandingLoop are covered without an animation allowlist. The seven sequence arguments, including timing, speed, repeat and blending,
remain unchanged apart from the eligible ready/channel animation ID. Ret 1Ch
and the seven argument reads were audited at 7121A0–712230 / 7127E4.

## Lifetime and timing

61FDD0 retains the kit pointer at effect+1Ch. Private kit copies therefore live
in a process-lifetime std::map whose node addresses survive insertions; they must
never be stack temporaries, an evicting cache or a cleared-on-look-change map.
Existing effects may outlive a cast, setting change or look change.

The original kit/effect dispatcher, callbacks, spell IDs and return values are
preserved. The missile constructor and its cast/target data, hit results, speed
and flight time are untouched. A swing starts at the game's normal spell-release
event; a projectile starts from the weapon then. This does not delay the server
cast until the middle/end of a swing or turn a melee attack into a spell cast.

## Verification and limitations

`tests/casting.cpp` executes the actual hooks with simulated client memory and
callbacks, covering isolation, stable kit lifetimes, field preservation, poses,
effects/origins, fallback/disable, unsupported casts and malformed bridge inputs.
It also schedules pose and sheath updates after the opening scope has ended,
through 30 seconds of simulated cast time, then checks release/cancel cleanup,
channels, one/two-handed poses, foreign models and unchanged sequence timing.
`tests/casting_catalog.cpp` adds 163 pinned client-derived cases covering all 128
distinct role/metadata/animation/channel combinations across 17,651 caster-kit
references in 13,262 eligible spells. Named cases include every player Blizzard
rank, Frostbolt, Arcane Missiles, Mind Flay, Evocation, Tranquility, Rain of Fire,
Hurricane and instant buffs. It exercises the actual hook, field preservation,
channel ownership, no-animation/custom styles and silent missing-caster fallback.
Regenerate with `tools/build_casting_fixtures.py` and extracted build-5875 Spell,
SpellVisual and SpellVisualKit DBCs; input hashes are pinned in the fixture header.
The ranged visual/missile regression suite is also a mandatory check.
`tests/casting_activities.cpp` uses 30 complete real activity spell records to
verify native kit, hand props, visual output, sequence, sheath and origin paths
remain untouched, including after a weapon cast. Ten real magic spell controls
confirm that the exclusions do not disable genuine casting or channels.
The Lua tests exercise real look persistence, transmog choice, dirty/save/discard,
reload, disabled/missing bridges, navigation and 1.12-shaped widgets. They are
mandatory release checks. The Windows DLL also builds with warnings as errors.

These checks do not validate rendered appearance inside WoW. A full restart with
the rebuilt DLL is required for that check: test a staff and one-handed sword,
cast-time/instant/channel spells, cancellation, ranged attacks, changing weapon
or look, and disabling the feature. Spell kits without hand art do not gain art;
weapons without a usable attachment fall back to normal casting. Other clients
must not reuse these addresses without their own ABI audit.
