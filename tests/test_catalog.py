"""Keep development equipment separate without losing or recoloring catalog items."""
import gzip
import importlib.util
import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / 'addon/SaureksCloset/Catalog.lua'
SOURCE_METADATA = ROOT / 'addon/SaureksCloset/CATALOG-SOURCE.json'
WEAPON_DATA = ROOT / 'addon/SaureksCloset/WeaponData.lua'


def snapshot(path):
    return path.read_bytes(), path.stat().st_mtime_ns


class CatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        before = {p: snapshot(p) for p in (CATALOG, SOURCE_METADATA)}
        spec = importlib.util.spec_from_file_location('build_catalog', ROOT / 'tools/build_catalog.py')
        cls.builder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.builder)
        assert all(snapshot(p) == state for p, state in before.items()), 'Import must not regenerate shipped files'
        with gzip.open(ROOT / 'research/classic.sql.gz', 'rt') as source:
            sql = source.read()
        cls.records = {record['entry']: record for record in cls.builder.read_item_templates(sql)}
        cls.quest_levels = cls.builder.read_quest_minimums(sql, cls.records.values())
        cls.items = cls.builder.read_items(sql, cls.records.values(), cls.quest_levels)

    def test_catalog_keeps_all_item_records_and_original_metadata(self):
        self.assertEqual(len(self.items), 9665)
        self.assertTrue(all(len(row) in (11, 12) and row[0] == item_id for item_id, row in self.items.items()))
        fixtures = {
            19019: [19019, 'Thunderfury, Blessed Blade of the Windseeker', 13, 5, 2, 7, 80, 30606, 0],
            25: [25, 'Worn Shortsword', 21, 1, 2, 7, 2, 1542, 0],
            19169: [19169, 'Nightfall', 17, 4, 2, 1, 70, 31735, 0],
            6255: [6255, 'Fishing Pole (JEFFTEST)', 26, 2, 2, 3, 19, 6590, 0],
            17: [17, 'Martin Fury', 4, 6, 4, 4, 5, 7016, 0],
        }
        for item_id, expected in fixtures.items():
            with self.subTest(item_id=item_id):
                self.assertEqual(self.items[item_id][:9], expected)

    def test_required_level_is_separate_from_item_level_for_every_item(self):
        for item_id, row in self.items.items():
            with self.subTest(item_id=item_id):
                self.assertIsNone(row[9], 'Normal equipment must preserve the reserved quiver icon index')
                self.assertEqual(row[10], self.records[item_id]['RequiredLevel'])
                self.assertIsInstance(row[10], int)
                self.assertGreaterEqual(row[10], 0)
        fixtures = {19019: (80, 60), 25: (2, 1), 19169: (70, 60), 6255: (19, 14), 17: (5, 0), 7937: (49, 44), 10504: (49, 0)}
        for item_id, levels in fixtures.items():
            with self.subTest(item_id=item_id):
                self.assertEqual((self.items[item_id][6], self.items[item_id][10]), levels)

    def test_quivers_keep_icons_and_include_their_source_required_level(self):
        rows = WEAPON_DATA.read_text().split('SaureksClosetQuivers={\n', 1)[1]
        decoded = {}
        for line in rows.splitlines():
            if not line.startswith('{'):
                continue
            row = json.loads('[' + line[1:-2] + ']')
            self.assertIn(len(row), (11, 12))
            self.assertNotIn(row[0], decoded)
            self.assertTrue(row[9].startswith('INV_Misc_Quiver_'))
            self.assertEqual(row, self.builder.catalog_row(self.records[row[0]], row[9], self.quest_levels))
            self.assertEqual(line, '{' + self.builder.render_row(row) + '},')
            decoded[row[0]] = row
        expected = {2101: 1, 2662: 50, 3573: 0, 3605: 0, 5439: 1, 7278: 1, 7371: 30, 8217: 40, 11362: 10, 18714: 60, 19319: 55}
        self.assertEqual({item_id: row[10] for item_id, row in decoded.items()}, expected)
        self.assertEqual(decoded[2662][6], 55)
        self.assertEqual(decoded[2662][9], 'INV_Misc_Quiver_06')

    def test_quest_rewards_use_minimum_eligibility_not_recommended_or_item_level(self):
        # Blackfathom Villainy is recommended at 27, but can be accepted at 18.
        self.assertEqual(self.items[7001][6], 29)
        self.assertEqual(self.items[7001][10:], [0, 18])
        # The lantern has separate quest sources starting at 17 and 19.
        self.assertEqual(self.items[5323][10:], [0, 17])
        self.assertEqual(self.items[19822][10:], [0, 60])
        inferred = [row for row in self.items.values() if len(row) == 12]
        self.assertEqual(len(inferred), 1143)
        self.assertTrue(all(row[10] == 0 and row[11] > 1 for row in inferred))
        meta = json.loads(SOURCE_METADATA.read_text())['quest_minimum_level']
        self.assertEqual(meta['count'], len(inferred))

    def test_uncertain_or_alternate_acquisition_never_gets_a_quest_restriction(self):
        for item_id in (2575, 19295, 3277, 1382, 10515, 10504, 17):
            with self.subTest(item_id=item_id):
                # Tradable shirt/flower, vendor, loot, supplied quest prop,
                # skill-only crafted goggles and test equipment remain unrestricted.
                self.assertEqual(self.items[item_id][10], 0)
                self.assertEqual(len(self.items[item_id]), 11)

    def test_quest_inference_uses_all_reward_paths_and_preserves_unknowns(self):
        records = [{'entry': i, 'RequiredLevel': 0, 'bonding': 1} for i in range(1, 10)]
        records[3]['bonding'] = 0
        records[4]['RequiredLevel'] = 20
        quests = [
            {'MinLevel': 40, 'QuestLevel': 60, 'RewChoiceItemId1': 1, 'RewItemId1': 2},
            {'MinLevel': 18, 'QuestLevel': 27, 'RewChoiceItemId6': 1},
            {'MinLevel': 0, 'QuestLevel': 55, 'RewItemId4': 2},
            {'MinLevel': 30, 'RewItemId1': 3, 'RewItemId2': 4, 'RewItemId3': 5},
            {'MinLevel': 35, 'RewItemId1': 6, 'RewItemId2': 7, 'RewItemId3': 8},
            {'MinLevel': 10, 'SrcItemId': 6},
            {'MinLevel': 1, 'RewChoiceItemId1': 9},
        ]
        self.assertEqual(self.builder.quest_minimum_levels(records, quests, {7}), {1: 18, 3: 30, 8: 35})

    def test_explicit_development_and_obsolete_markers(self):
        names = [
            'Mail Helmet A (Test)', 'Deprecated Rogue\'s Vest',
            'Monster - Sword, Katana 2H', '[PH] Rising Dawn Gloves',
            'Blackwing Mace1H[PH]', 'OLDRecruit\'s Belt', '(OLD)Medium Throwing Knife',
            'Fishing Pole (JEFFTEST)', 'TestBoots - Puffed Mail Green',
            'Durability Chestpiece', '90 Epic Warrior Bracelets', '63 Green Frost Wand',
            'PVP Plate Helm Alliance', 'Red Leather C03 Breastplate',
            'ZZZZZ sword 3', 'Martin Fury', 'Indalamar\'s Sword of Pwnage',
        ]
        for name in names:
            with self.subTest(name=name):
                reason = self.builder.unavailable_reason(name)
                self.assertIsInstance(reason, str)
                self.assertTrue(reason)

    def test_normal_items_are_not_hidden_by_ambiguous_name_fragments(self):
        names = [
            'Testament of Hope', 'Contest Winner\'s Tabard',
            'Old Greatsword', 'Old Blanchy\'s Blanket', 'Old Leather Belt', 'Old Blunderbuss',
            'Broken Blade', 'Worn Shortsword', 'Bent Staff', 'Dress Shoes',
            'Nightfall', 'Arcanite Reaper', 'Lionheart Helm', 'Robe of the Archmage',
            'Black Dragonscale Breastplate', 'Belt of the Archmage',
            'Fist of Stone', 'Tabard of the Scarlet Crusade',
        ]
        for name in names:
            with self.subTest(name=name):
                self.assertIsNone(self.builder.unavailable_reason(name))

    def test_shipped_catalog_matches_source_and_adds_only_named_flags(self):
        rendered = self.builder.render_catalog(self.items)
        self.assertEqual(rendered, CATALOG.read_text())
        decoded = {}
        marked = set()
        for line in rendered.splitlines():
            if not line.startswith('{'):
                continue
            self.assertTrue(line.endswith('},'), line)
            body = line[1:-2]
            flag = re.search(r',\s*unobtainable\s*=\s*true\s*$', body)
            if flag:
                body = body[:flag.start()]
            self.assertRegex(body, r',nil,\d+(?:,\d+)?$', 'Use Lua nil for the reserved icon field, not JSON null')
            row = json.loads('[' + body.replace(',nil,', ',null,') + ']')
            self.assertIn(len(row), (11, 12), 'A named flag must not shift positional metadata')
            self.assertNotIn(row[0], decoded)
            decoded[row[0]] = row
            if flag:
                marked.add(row[0])
        self.assertEqual(decoded, self.items)
        expected = {item_id for item_id, row in self.items.items() if self.builder.unavailable_reason(row[1])}
        self.assertEqual(marked, expected)
        self.assertEqual(len(marked), 1238)
        self.assertEqual(json.loads(SOURCE_METADATA.read_text())['count'], len(self.items))


if __name__ == '__main__':
    unittest.main()
