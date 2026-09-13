"""Verify the pinned quiver's center and rigid geometry without writing assets."""
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import re
import struct

import read_client_data as client


root = Path(__file__).resolve().parents[1]
expected_model = r'Item\ObjectComponents\Quiver\Quiver_A.mdx'
expected_sha = '007b74a78356ce7f1307a996042b3b41e9cd441783151440367db4b09c2c06ed'
expected_min = (-0.17494499683380127, -0.024836329743266106, -0.09142405539751053)
expected_max = (0.878679096698761, 0.050592340528964996, 0.12136653810739517)

# WeaponAssets.h uses JSON-compatible string escaping and primitive fields.
quivers = []
for line in (root / 'native/WeaponAssets.h').read_text().splitlines():
    if re.match(r'^\{\d+,', line):
        row = json.loads('[' + line.rstrip(',')[1:-1] + ']')
        if row[2] == 5:
            quivers.append(row)
assert quivers and {row[6] for row in quivers} == {expected_model}, \
    'A new quiver mesh needs its own measured center.'

try:
    for archive in ['patch-2.MPQ', 'patch.MPQ', 'model.MPQ']:
        handle = C.c_void_p()
        assert client.lib.SFileOpenArchive(
            str(client.game / archive).encode(), 0, 0x100, C.byref(handle)), archive
        client.archives.append(handle)
    data = client.read(expected_model[:-4] + '.m2')
    back_parents = []
    for race in ['Human', 'Orc', 'Dwarf', 'NightElf', 'Scourge', 'Tauren', 'Gnome', 'Troll']:
        for sex in ['Male', 'Female']:
            filename = f'Character\\{race}\\{sex}\\{race}{sex}.m2'
            character = client.read(filename)
            assert character and character[:8] == b'MD20\x00\x01\x00\x00', filename
            attachment_count, attachment_offset = struct.unpack_from('<II', character, 0x104)
            lookup_count, lookup_offset = struct.unpack_from('<II', character, 0x10C)
            bone_count, bone_offset = struct.unpack_from('<II', character, 0x34)
            assert 28 < lookup_count <= 512 and lookup_offset + 2 * lookup_count <= len(character), filename
            assert 0 < attachment_count <= 512 and attachment_offset + 48 * attachment_count <= len(character), filename
            assert 0 < bone_count <= 2048 and bone_offset + 108 * bone_count <= len(character), filename
            index = struct.unpack_from('<H', character, lookup_offset + 2 * 28)[0]
            assert index < attachment_count, filename
            actual, bone = struct.unpack_from('<IH', character, attachment_offset + 48 * index)
            assert actual == 28 and bone < bone_count, filename
            parent = struct.unpack_from('<h', character, bone_offset + 108 * bone + 8)[0]
            assert 0 <= parent < bone_count and parent != bone, filename
            back_parents.append(parent)
    fixture = (
        '// Immediate parents of point-28 bones in all 16 build-5875 character models.\n'
        '// Same race/sex order as bowFixtures; verified by tools/audit_quiver_model.py.\n'
        'static constexpr std::array<int,16> quiverBackParents{{\n'
    )
    fixture += ''.join(f'{back_parents[i]},{back_parents[i + 1]},\n' for i in range(0, 16, 2))
    fixture += '}};\n'
    assert (root / 'tests/quiver_attachment_fixtures.h').read_text() == fixture, \
        'Installed back attachment parents differ from regression fixtures; inspect before updating.'
finally:
    for handle in client.archives:
        client.lib.SFileCloseArchive(handle)

assert data and data[:8] == b'MD20\x00\x01\x00\x00', 'Expected a build-5875 M2.'
digest = hashlib.sha256(data).hexdigest()
assert digest == expected_sha, 'Installed quiver mesh differs from the pinned asset.'
vertex_count, vertex_offset = struct.unpack_from('<II', data, 0x44)
assert vertex_count == 122 and vertex_offset + 48 * vertex_count <= len(data)
vertices = [struct.unpack_from('<3f', data, vertex_offset + 48 * i)
            for i in range(vertex_count)]
assert all(math.isfinite(value) for vertex in vertices for value in vertex)
mesh_min = tuple(min(vertex[i] for vertex in vertices) for i in range(3))
mesh_max = tuple(max(vertex[i] for vertex in vertices) for i in range(3))
assert mesh_min == expected_min and mesh_max == expected_max
assert struct.unpack_from('<3f', data, 0xB4) == mesh_min
assert struct.unpack_from('<3f', data, 0xC0) == mesh_max
center = tuple((low + high) / 2 for low, high in zip(mesh_min, mesh_max))

# Every vertex is fully weighted to the unanimated root bone. Its authored
# pivot is not an origin offset: without tracks, its bone transform is identity.
for i in range(vertex_count):
    weights_and_bones = struct.unpack_from('<8B', data, vertex_offset + 48 * i + 12)
    assert weights_and_bones == (255, 0, 0, 0, 0, 0, 0, 0)
bone_count, bone_offset = struct.unpack_from('<II', data, 0x34)
assert bone_count == 2 and bone_offset + 108 * bone_count <= len(data)
assert struct.unpack_from('<h', data, bone_offset + 8)[0] == -1
for track_offset in [12, 40, 68]:
    _, _, ranges, _, times, _, keys, _ = struct.unpack_from(
        '<Hh6I', data, bone_offset + track_offset)
    assert ranges == times == keys == 0, 'Animated geometry needs a dynamic center.'

renderer = (root / 'native/WeaponRenderer.h').read_text()
match = re.search(r'\bquiverCenter\s*(?:=\s*)?\{\{?([^}]+)', renderer)
assert match, 'Renderer must declare the measured quiverCenter.'
configured = tuple(float(value.strip().rstrip('fF')) for value in match[1].split(','))
assert len(configured) == 3 and all(abs(a - b) < 1e-7 for a, b in zip(center, configured)), \
    'Renderer center does not match the installed quiver mesh.'
print(f'PASS: {len(quivers)} quivers share the verified rigid mesh; bounds and renderer center agree. SHA256 {digest}')
print('PASS: point-28 source bones and immediate parent fixtures verified against all 16 installed models.')
