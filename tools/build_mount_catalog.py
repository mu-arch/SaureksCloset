"""Build mount appearances from build-5875 DBCs and the existing pinned classic DB.

Input DBC files are read from an extracted client directory; game assets are not
copied. Creature IDs in mounted auras resolve through creature_template.ModelId.
Catalog data has the same notices as CATALOG-LICENSE/CATALOG-COPYRIGHT.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import struct
from build_catalog import read_table

ROOT = Path(__file__).resolve().parents[1]
REVISION = '22b51464f1625f6ef6275771de1f5466c6f5d19e'
SQL_SHA256 = '4f92db520868ab4e566726f68b5b2e380ae781209beaf22237b4f7f04600d0c0'


def dbc(path, fields):
    data = path.read_bytes()
    magic, count, actual, size, strings = struct.unpack_from('<4s4I', data)
    assert magic == b'WDBC' and actual == fields and size == fields * 4
    assert len(data) == 20 + count * size + strings
    block = data[20 + count * size:]
    def text(offset):
        assert offset < len(block)
        return block[offset:block.index(b'\0', offset)].decode('utf-8')
    return list(struct.iter_unpack('<%dI' % fields, data[20:20 + count * size])), text


def build(directory, source):
    assert hashlib.sha256(source.read_bytes()).hexdigest() == SQL_SHA256
    creatures = {r['Entry']: r for r in read_table(gzip.decompress(source.read_bytes()).decode(), 'creature_template')}
    spells, text = dbc(directory / 'Spell.dbc', 173)
    icon_rows, icon_text = dbc(directory / 'SpellIcon.dbc', 2)
    icons = {r[0]: icon_text(r[1]) for r in icon_rows}
    display_rows, _ = dbc(directory / 'CreatureDisplayInfo.dbc', 12)
    displays = {r[0]: r for r in display_rows}
    model_rows, model_text = dbc(directory / 'CreatureModelData.dbc', 16)
    models = {r[0]: model_text(r[2]) for r in model_rows}
    appearances, auras = {}, set()
    for spell in spells:
        for effect in range(3):
            if spell[61 + effect] != 6 or spell[91 + effect] != 78:
                continue  # SPELL_EFFECT_APPLY_AURA / SPELL_AURA_MOUNTED
            name = text(spell[120])
            auras.add(name)
            creature = creatures.get(spell[106 + effect])
            if not creature:
                raise ValueError('Missing mount creature: ' + str(spell[0]))
            for field in ['ModelId1', 'ModelId2', 'ModelId3', 'ModelId4']:
                display = creature[field]
                if not display:
                    continue
                assert display in displays, (spell[0], display)
                model = models[displays[display][1]]
                assert model.lower().endswith(('.mdx', '.m2'))
                icon = icons.get(spell[117]) or 'Interface\\Icons\\Ability_Mount_RidingHorse'
                label = name[7:] if name.startswith('Summon ') else name
                entry = appearances.setdefault(display, dict(id=display, name=label, icon=icon, model=model, spells=[]))
                # Prefer the later spell's current name over retired duplicates.
                entry['name'] = label
                entry['spells'].append(spell[0])
    mount_icons, other_aura_icons = set(), set()
    for spell in spells:
        icon = icons.get(spell[117])
        if any(spell[61+i] == 6 and spell[91+i] == 78 for i in range(3)):
            if icon: mount_icons.add(icon)
        elif any(spell[61+i] == 6 for i in range(3)):
            if icon: other_aura_icons.add(icon)
    return sorted(appearances.values(), key=lambda r: (r['name'].lower(), r['id'])), sorted(auras), sorted(mount_icons), sorted(icon for icon in mount_icons - other_aura_icons if icon.lower().startswith('interface\\icons\\ability_mount_'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, required=True)
    parser.add_argument('--sql', type=Path, required=True)
    args = parser.parse_args()
    entries, auras, candidate_icons, safe_icons = build(args.dbc, args.sql)
    quote = lambda value: json.dumps(value, ensure_ascii=True)
    lines = ['-- Generated vanilla mount appearances. See MOUNTS-SOURCE.json and CATALOG-LICENSE.md.',
             '-- IDs are creature DISPLAY IDs, not creature entries or mount item IDs.', 'VanityStudioMountCatalog={']
    for entry in entries:
        lines.append('{id=%d,name=%s,icon=%s},' % (entry['id'], quote(entry['name']), quote(entry['icon'])))
    lines += ['}', '-- Exact mounted-aura names from Spell.dbc; independent of item-cache loading.', 'VanityStudioMountAuras={']
    lines += ['[%s]=true,' % quote(name) for name in auras]
    lines += ['}', 'VanityStudioMountCandidateIcons={']
    lines += ['[%s]=true,' % quote(icon.lower()) for icon in candidate_icons]
    lines += ['}', '-- Icons used only by mounted auras (locale-independent fallback).', 'VanityStudioMountAuraIcons={']
    lines += ['[%s]=true,' % quote(icon.lower()) for icon in safe_icons]
    lines += ['}', '']
    (ROOT / 'addon/SaureksCloset/MountCatalog.lua').write_text('\n'.join(lines))
    metadata = dict(schema=1, client='Windows 1.12.1 build 5875', count=len(entries),
                    source='https://github.com/cmangos/classic-db', revision=REVISION,
                    dump='Full_DB/ClassicDB_1_12_1_z2815.sql.gz', sha256=SQL_SHA256,
                    scope='Every SPELL_AURA_MOUNTED appearance in the client, including retired and event/test mounts; deduplicated by display ID. No later-expansion or server-custom models.',
                    dbc={name: hashlib.sha256((args.dbc / name).read_bytes()).hexdigest() for name in ['Spell.dbc','SpellIcon.dbc','CreatureDisplayInfo.dbc','CreatureModelData.dbc']},
                    entries=entries, auraNames=auras, candidateMountIcons=candidate_icons, exclusiveMountIcons=safe_icons)
    (ROOT / 'addon/SaureksCloset/MOUNTS-SOURCE.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print('Generated %d mount appearances and %d mounted-aura names' % (len(entries), len(auras)))


if __name__ == '__main__':
    main()
