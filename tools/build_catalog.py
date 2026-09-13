"""Build the vanilla visible-equipment catalog from the pinned CMaNGOS SQL dump."""
import gzip
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISIBLE = {1, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 25, 26}
FIELDS = ('entry', 'name', 'InventoryType', 'Quality', 'class', 'subclass', 'ItemLevel', 'displayid', 'itemset')
# These markers describe developer, NPC, placeholder or explicitly retired items.
# Missing loot/vendor references alone are not evidence: starter gear, crafting
# and special rewards can come from other sources. Keep uncertain items visible.
# Word boundaries/case matter: Testament, Contest Winner and ordinary Old/Broken
# item names must never be treated as development markers.
UNAVAILABLE_PATTERNS = (
    ('explicit_markers', re.compile(r'\b(?:test|deprecated|placeholder|unused|debug|PH|gamemaster)\b', re.I)),
    ('deprecated_prefix', re.compile(r'^Deprecated', re.I)),
    ('monster_equipment', re.compile(r'^Monster\s*-', re.I)),
    ('old_marker', re.compile(r'^(?:OLD(?=[A-Z])|\(OLD\))')),
    ('joined_test_marker', re.compile(r'(?:^Test(?=[A-Z])|\(JEFFTEST\))')),
    ('durability_test_series', re.compile(r'^Durability\s', re.I)),
    ('numbered_test_series', re.compile(r'^\d+ (?:Epic|Green) (?:Warrior|Rogue|Frost)\b', re.I)),
    ('pvp_prototypes', re.compile(r'^PVP (?:Plate|Cloth) .+ (?:Alliance|Horde)$')),
    ('asset_code_prototypes', re.compile(r'\b(?:Leather|Mail|Plate|Cloth) [A-Z][0-9]{2}\b')),
    ('zz_placeholders', re.compile(r'^ZZZZ')),
    ('gm_weapons', re.compile(r"^(?:Martin (?:Fury|Thunder)|Indalamar's Sword)")),
)


def unavailable_reason(name):
    for reason, pattern in UNAVAILABLE_PATTERNS:
        if pattern.search(name):
            return reason
    return None


def read_table(sql, name):
    marker = 'CREATE TABLE `' + name + '` ('
    if marker not in sql:
        return
    schema = sql.split(marker, 1)[1].split(') ENGINE=', 1)[0]
    columns = re.findall(r'^  `([^`]+)`', schema, re.M)
    # SQL strings may contain commas, parentheses and escaped apostrophes.
    token = re.compile(r"'((?:\\.|[^'\\])*)'|([^,()]+)|([,()])")
    escapes = {'n': '\n', 'r': '\r', 't': '\t', '0': '\0', 'Z': '\x1a'}
    for statement in re.findall(r'^INSERT INTO `' + re.escape(name) + r'` VALUES (.*);$', sql, re.M):
        row = None
        for match in token.finditer(statement):
            string, number, punctuation = match.groups()
            if punctuation == '(':
                row = []
            elif punctuation == ')':
                assert len(row) == len(columns), (len(row), len(columns))
                yield dict(zip(columns, row))
                row = None
            elif string is not None:
                row.append(re.sub(r'\\(.)', lambda escape: escapes.get(escape[1], escape[1]), string))
            elif number is not None and row is not None:
                row.append(None if number == 'NULL' else float(number) if '.' in number or 'e' in number.lower() else int(number))


def read_item_templates(sql):
    return read_table(sql, 'item_template')


def quest_minimum_levels(records, quests, alternative_items):
    """Infer only known, non-tradable quest rewards with no alternative source."""
    alternatives = set(alternative_items)
    reward_levels = {}
    reward_fields = (['RewItemId' + str(index) for index in range(1, 5)] +
                     ['RewChoiceItemId' + str(index) for index in range(1, 7)])
    for quest in quests:
        # Quest-supplied equipment can be acquired before a reward is earned.
        if quest.get('SrcItemId'):
            alternatives.add(quest['SrcItemId'])
        for field in reward_fields:
            item_id = quest.get(field, 0)
            if item_id:
                level = quest.get('MinLevel', 0)
                reward_levels[item_id] = min(level, reward_levels.get(item_id, level))
    return {
        record['entry']: reward_levels[record['entry']]
        for record in records
        if record['RequiredLevel'] == 0 and record['bonding'] == 1
        and record['entry'] not in alternatives
        and reward_levels.get(record['entry'], 0) > 1
    }


def read_quest_minimums(sql, records):
    alternatives = set()
    for name in re.findall(r'CREATE TABLE `([^`]+)`', sql):
        field = None
        if name.endswith('_loot_template') or name in ('npc_vendor', 'npc_vendor_template', 'item_convert', 'item_expire_convert'):
            field = 'item'
        elif name == 'playercreateinfo_item':
            field = 'itemid'
        if field:
            alternatives.update(row[field] for row in read_table(sql, name) if row[field])
        elif name.startswith('dbscripts_'):
            # CMaNGOS SCRIPT_COMMAND_CREATE_ITEM (17), datalong = item entry.
            alternatives.update(row['datalong'] for row in read_table(sql, name)
                                if row['command'] == 17 and row['datalong'])
    return quest_minimum_levels(records, read_table(sql, 'quest_template'), alternatives)


def catalog_row(record, icon=None, quest_levels=None):
    # Preserve item level at index 7 and quiver icon overrides at index 10.
    # RequiredLevel is distinct from ItemLevel and drives tooltip-style text.
    row = [record[key] for key in FIELDS] + [icon, record['RequiredLevel']]
    if quest_levels and record['entry'] in quest_levels:
        row.append(quest_levels[record['entry']])
    return row


def read_items(sql, records=None, quest_levels=None):
    records = list(read_item_templates(sql) if records is None else records)
    if quest_levels is None:
        quest_levels = read_quest_minimums(sql, records)
    return {
        record['entry']: catalog_row(record, quest_levels=quest_levels)
        for record in records
        if record['InventoryType'] in VISIBLE and record['displayid'] > 0 and record['class'] in (2, 4)
    }


def render_row(item):
    return ','.join('nil' if value is None else json.dumps(value, ensure_ascii=False) for value in item)


def render_catalog(items):
    lines = [
        '-- Generated by tools/build_catalog.py; see CATALOG-LICENSE.md.',
        '-- id, name, inventory type, quality, class, subclass, item level, display ID, set ID, icon override, required level, optional quest minimum level',
        '-- Optional unobtainable flag keeps developer/unused items separate without changing their real quality.',
        'VanityStudioCatalog = {',
    ]
    for item in sorted(items.values(), key=lambda item: (item[1].lower(), item[0])):
        flag = ',unobtainable=true' if unavailable_reason(item[1]) else ''
        lines.append('{' + render_row(item) + flag + '},')
    return '\n'.join(lines + ['}', ''])


def main():
    source = ROOT / 'research/classic.sql.gz'
    with gzip.open(source, 'rt') as stream:
        items = read_items(stream.read())
    (ROOT / 'addon/SaureksCloset/Catalog.lua').write_text(render_catalog(items))
    reasons = Counter(filter(None, (unavailable_reason(item[1]) for item in items.values())))
    meta = {
        'source': 'https://github.com/cmangos/classic-db',
        'revision': json.loads((ROOT / 'research/db-tree.json').read_text())['sha'],
        'dump': 'Full_DB/ClassicDB_1_12_1_z2815.sql.gz',
        'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'count': len(items),
        'scope': 'Visible vanilla armor, shirts, tabards, shields and weapons; includes unused/developer entries present in the database. Server custom items are not included.',
        'unobtainable': {
            'count': sum(reasons.values()),
            'method': 'Explicit developer, NPC, test, placeholder and retired-item name markers. Missing acquisition records alone do not classify an item; uncertain items remain in normal rarities.',
            'reasons': dict(sorted(reasons.items())),
        },
        'quest_minimum_level': {
            'count': sum(len(item) > 11 for item in items.values()),
            'method': 'Lowest quest MinLevel among every fixed or choice reward source, only for bind-on-pickup equipment with no equip level requirement. Quest-supplied items and known loot, vendor, starter, conversion or script-created alternatives are excluded. ItemLevel and recommended QuestLevel are not eligibility requirements; unknown sources receive no inferred restriction.',
        },
    }
    (ROOT / 'addon/SaureksCloset/CATALOG-SOURCE.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(json.dumps(meta, indent=2))


if __name__ == '__main__':
    main()
