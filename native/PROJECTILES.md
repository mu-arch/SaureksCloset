# Build 5875 missile visual ABI audit

Audited against the supported Windows 1.12.1 build 5875 executable. Addresses are virtual addresses, image base 0x400000. Native signature checks reject incompatible clients.

## Missile constructor

`0x60A3D0` is a `void __thiscall` unit method. ECX is the caster unit; eleven 32-bit stack arguments. Both exits end with `ret 0x2C` at 0x60A6F8 / 0x60A708. Function prefix (12 bytes):

`55 8B EC 83 EC 10 8B 45 28 85 C0 53`

Suggested C++ signature:

```cpp
using UnitMissile = void (__thiscall *)(void* unit,
    const void* targetGuid, const void* castData,
    unsigned ammoDisplay, unsigned ammoInventory,
    unsigned arg5, unsigned hitResult, unsigned arg7,
    const unsigned* visual, unsigned spellId,
    const void* targets, const void* targetFlags);
```

| Argument | EBP offset | Observed use |
| --- | --- | --- |
| 1 targetGuid | +08 | Read as two uints, saved to CMissile+10/+14 |
| 2 castData | +0C | Packet targeting flags at +14; positions +30/+3C |
| 3 ammoDisplay | +10 | ItemDisplayInfo table index when visual has no missile effect |
| 4 ammoInventory | +14 | Passed to 0x479F40; 24 means ammo, 25 means thrown weapon |
| 5 arg5 | +18 | Sets missile flag bit1 when nonzero |
| 6 hitResult | +1C | Saved to CMissile+48, later used for miss/deflect branches |
| 7 arg7 | +20 | Saved to CMissile+4C |
| 8 visual | +24 | Immediate reads only: +1C, +24, +28; pointer never retained |
| 9 spellId | +28 | Resolves real spell from C0D788/maxC0D78C; retained at CMissile+18 |
| 10 targets | +2C | Target collection copied to allocated per-missile storage |
| 11 targetFlags | +30 | Optional result collection copied likewise |

Only direct callsites are 0x6E8B84 and 0x6E8BD4, inside 0x6E8A50. That is reached from the SpellGo visual processing path, after packet decoding and caster resolution (0x6E823A/0x6E8279). Ammo display and inventory arguments originate in optional packet fields under flag0x20 (0x6E7CB8/0x6E7CC3). Changes to call arguments here affect local art, not packet data, inventory, damage, or spell requirements.

At 0x60A4C7 the original spell ID is retained; at 0x60A4CC the real spell's +94 speed is read and saved to CMissile+1C. Neither should be changed.

## Selecting the projectile model

At 0x60A409 `visual[7]` (+1C) is resolved via SpellVisualEffectName table C0D760/maxC0D764; model filename is loaded from effect row+08 by 0x707350. If visual[7] is zero, the original method uses ammoDisplay and ammoInventory with 0x479F40.

0x479F40 explicitly accepts inventory 0x18 (24) or 0x19 (25). At 0x479F84 the default prefix is the string at 0x838D70, `Item\\ObjectComponents\\Ammo\\`; only inventory25 replaces it with 0x838D50, `Item\\ObjectComponents\\Weapon\\`. Thus inventory24 is correct for both arrow and bullet display IDs.

`visual[9]` (+24) selects target attachment through table0x860A18 and must be preserved. `visual[10]` (+28) is saved to CMissile+40, then at 0x61E795 fed to sound creation0x4589A0; CMissile+44 owns the returned sound and updates it at0x61E77C. Thus replacing field10 with chosen weapon visual's sound is also cosmetic, though model-only scope can preserve it.

## Authentic selected wand art

0x60D450 is the native dynamic SpellVisual merger. If original Spell.Attributes(+18)&2, it queries actual ranged display via unit vtable+A0(role2), resolves ItemDisplayInfo tableC0DC10/maxC0DC14, reads display+28 (field10 SpellVisualID), resolves SpellVisual tableC0D738/maxC0D73C. Selected wand display's field10 and that row's field7 therefore give its authentic bolt.

The merger proves SpellVisual is exactly64bytes: 0x60D558 sets ECX16, 0x60D55F executes REP MOVSD. At0x60D504 it requires weaponVisual[6] (HasMissile) before filling output field7. Other fields are independently merged. To override only new missiles, copy64bytes of argument8 on the hook's stack, alter field7 (and optionally field10) and pass that private buffer into the synchronous original method. No pointer is stored, so buffer lifetime is sufficient. Do not modify global DBC rows.

## Ranged attack gate

Verified with local Spell.dbc and native requirement checks0x6E4E13..0x6E4E38:

- Spell[6] &2, native offset18, marks ranged/ammo use.
- Spell[58] ==2, native offsetE8, requires weapon class.
- Nonzero Spell[59] (offsetEC) must be a subset of0xC000C: bow2, gun3, crossbow18, wand19.

All168 matching installed rows include AutoShot75, Shoot5019, ShootBow2480, ShootGun7918, ShootCrossbow7919 and every Arcane/Multi/Aimed/Serpent/etc rank. This excludes Frostbolt, Fireball, ShadowBolt, thrown weapons and unrestricted NPC spells. Additional checks should enforce local live unit, native display, owned world context, active chosen ranged route and valid selected asset. No equipment requirement checker should be hooked or fed cosmetic metadata.

## Arrow persistence consumer

0x60A4F8 calls unit role2 metadata; return0x60A4FE feeds0x60A710, which recognizes bow2/crossbow18/thrown16, setting CMissile flags+38 bit4. At0x61DD10 this enables0x61E7C0(target,5000), attaching the projectile to its target. Add this return address to the cosmetic metadata path only during a successful projectile override; otherwise bow-to-wand or bow-to-gun could retain arrow-style persistence. It is purely visual and should preserve stock fallback on failed lookup.

Additional signature prefixes:

- 60A4F8: `FF 90 98 00 00 00 8B C8 E8 0B 02 00`
- 60D450: `55 8B EC 53 56 8B 75 08 57 8B F9 8B`
- 60D493: `8B 40 28 85 C0 74 17 7C 13 3B 05 3C`
- 6E8B84: `E8 47 18 F2 FF 8B 06 47 3B F8 89 7D`
- 6E8BD4: `E8 F7 17 F2 FF 5F 5E 8B E5 5D 8B E3`

## Implementation in renderer 30702

`ProjectileRenderer.h` prepares a private SpellVisual copy for the local player’s
active attacking ranged appearance, then calls the original missile constructor
with its original spell, targets, results, and timing. Failed validation keeps
the original visual arguments.

Bow/crossbow appearances use arrow ItemDisplayInfo2414; guns use bullet
ItemDisplayInfo2418, both with ammo inventory24. Wands resolve the chosen item’s
own SpellVisual missile effect. Decorative wand displays with no firing visual
use the stock Lesser Magic Wand bolt (SpellVisual2799). Missile flight sound
follows the selected weapon visual. Arrow persistence metadata is scoped to successful replacements.

The renderer does not rewrite the global DBC tables, the player’s equipment,
network messages, spell requirements, damage, or other characters’ projectiles.
The DLL update requires a full game restart; reloading the addon cannot replace
the renderer already loaded in the process.

## Firing visuals in renderer 30704

The missile-only replacement in 30702/30703 ran after WoW had already chosen
precast and firing kits. It therefore retained the real wand's load/fire sounds.
30704 also intercepts the dynamic merger at `0x60D450`:

```cpp
unsigned* __thiscall(void* unit, const unsigned* spellRow, unsigned* output64);
```

The original returns the caller-owned 64-byte buffer (or null), with `ret 8`.
The detour calls it first, verifies that exact return pointer, validates the real
spell row identity and the same local-world ranged-appearance gates, then
changes fields 1 (precast kit), 2 (fire kit), 6 (has missile), 7 (missile), 10
(flight sound), and 14 (reload sound) in that caller-owned buffer. Zero sounds
replace inherited sounds too. No global DBC row or spell object is changed.
Ability-specific impact/state fields stay native.

Audited callers are `0x6EC29F` (precast), `0x6E802E` (SpellGo/fire), `0x62FAF8`
(reload sound), and `0x6EC807` (attachment position). Their buffers outlive all
visual use. All four callsite prefixes and the upstream dispatch gate are
included in the executable compatibility checks.

Gun visual224 uses precast kit161/sound1147 and fire kit167/sound1148. Bow
visual5 uses kits7/164 with sounds1144/1146; crossbow visual743 uses kits803/804.
Wands retain their selected elemental bolt and wand kits (e.g. visual2799,
kits372/2973). This also fixes a concrete animation difference: wand precast
kit372 has animation111, which the previous generic remap made HoldRifle110;
the gun's actual precast kit161 uses LoadRifle106.

The private merged visual always sets HasMissile=1. Raw physical weapon visuals
normally have this field zero and depend on packet ammo. A real wand sends no
physical ammo: without this guard `0x6E8A7C` would skip the constructor before
it can supply the chosen bullet/arrow. The late hook still provides display2418
for bullets or2414 for arrows with inventory24. Both physical model files and
textures were verified present in the installed client's MPQs.

Missile release remains with WoW's animation events. `0x60A3D0` queues a missile
at unit+AC; rifle animations can initially hide it. `0x60C940` restores alpha and
launches it after `$BWR`/cast release events. This path uses the current animation,
not the original equipped weapon subclass. The selected firing kits and visual
ammo are now consistent across that path. Offline regression checks cover the
full merger → dispatch gate → constructor sequence, not live game rendering.
The earlier missing-bullet report was not reproduced in a running game.
