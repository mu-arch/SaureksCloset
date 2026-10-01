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
for address, values in signatures:
    rva = int(address, 16) - base
    expected = bytes(int(value, 16) for value in values.split(','))
    assert len(expected) == 12
    for _, _, start, length, offset, *_ in sections:
        if start <= rva and rva + 12 <= start + length:
            actual = binary[offset + rva - start:offset + rva - start + 12]
            assert actual == expected, f'Incompatible client at {address}'
            break
    else:
        raise AssertionError(f'Unmapped signature {address}')
print(f'PASS: all {len(signatures)} native executable signatures match the installed client')
