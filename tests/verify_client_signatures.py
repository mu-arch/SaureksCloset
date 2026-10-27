"""Read-only check of native hook prefixes against a supplied 32-bit client."""
from pathlib import Path
import re
import struct
import sys

binary = Path(sys.argv[1]).read_bytes()
pe = struct.unpack_from('<I', binary, 0x3c)[0]
count = struct.unpack_from('<H', binary, pe + 6)[0]
optional = pe + 24
size = struct.unpack_from('<H', binary, pe + 20)[0]
base = struct.unpack_from('<I', binary, optional + 28)[0]
sections = [struct.unpack_from('<8sIIIIIIHHI', binary, optional + size + 40*i)
            for i in range(count)]
source = (Path(__file__).resolve().parents[1] / 'native/BuildSignatures.h').read_text()
signatures = re.findall(r'\{(0x[0-9a-f]+),\{([^}]+)\}\}', source)
def client_bytes(address, count):
    rva = address - base
    for _, _, start, length, offset, *_ in sections:
        if start <= rva and rva + count <= start + length:
            return binary[offset + rva - start:offset + rva - start + count]
    raise AssertionError(f'Unmapped client address {address:#x}')

for address, values in signatures:
    expected = bytes(int(value, 16) for value in values.split(','))
    assert len(expected) == 12
    assert client_bytes(int(address, 16), 12) == expected, f'Incompatible client at {address}'

# Independent binary evidence for the native cape/tabard layout. Inventory slot
# order differs from compositor order: cape=10, tabard=9, not the reverse.
slot_map = client_bytes(0x478da4, 19)
for inventory, target, component_slot in [(14, 0x478d63, 10), (18, 0x478d54, 9)]:
    jump, = struct.unpack('<I', client_bytes(0x478d74 + 4*slot_map[inventory], 4))
    assert jump == target
    assert client_bytes(jump + 4, 2) == bytes([0x6a, component_slot])
assert client_bytes(0x478ae1, 7) == bytes.fromhex('89 84 bb a8 04 00 00')
print('PASS: native cape/tabard inventory dispatch and component-row layout match the regression fixtures')
print(f'PASS: all {len(signatures)} native executable signatures match the installed client')
