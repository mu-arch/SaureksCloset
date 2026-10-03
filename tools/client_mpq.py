"""Read-only build-5875 MPQ access for offline asset generation."""
import bz2, struct, zlib
from pathlib import Path

CRYPT = [0] * 1280
seed = 0x100001
for aa in range(256):
    for bb in range(5):
        seed = (seed * 125 + 3) % 0x2AAAAB
        high = (seed & 0xffff) << 16
        seed = (seed * 125 + 3) % 0x2AAAAB
        CRYPT[aa + bb * 256] = high | (seed & 0xffff)

def mpq_hash(value, kind):
    a, b = 0x7fed7fed, 0xeeeeeeee
    for x in value.upper().encode():
        a = (CRYPT[(kind << 8) + x] ^ (a + b)) & 0xffffffff
        b = (x + a + b + (b << 5) + 3) & 0xffffffff
    return a

def decrypt(data, key):
    seed, result = 0xeeeeeeee, []
    for x in struct.unpack('<%dI' % (len(data) // 4), data):
        seed = (seed + CRYPT[0x400 + (key & 255)]) & 0xffffffff
        x ^= (key + seed) & 0xffffffff
        key = ((~key << 21) + 0x11111111 | (key >> 11)) & 0xffffffff
        seed = (x + seed + (seed << 5) + 3) & 0xffffffff
        result.append(x)
    return struct.pack('<%dI' % len(result), *result)

def decode(data, size):
    if len(data) == size:
        return data
    if data[0] == 2:
        result = zlib.decompress(data[1:])
    elif data[0] == 16:
        result = bz2.decompress(data[1:])
    else:
        raise ValueError(f'Unsupported MPQ compression mask {data[0]}')
    assert len(result) == size
    return result

class MPQ:
    def __init__(self, path):
        self.path, self.file = path, path.open('rb')
        magic, _, _, version, shift, ht, bt, nh, nb = struct.unpack('<4sIIHHIIII', self.file.read(32))
        assert magic == b'MPQ\x1a' and version in (0,1)
        if version == 1:
            extended, ht_high, bt_high = struct.unpack('<QHH', self.file.read(12))
            assert extended == 0 and ht_high == 0 and bt_high == 0
        self.sector = 512 << shift
        self.file.seek(ht)
        self.hashes = list(struct.iter_unpack('<IIHHI', decrypt(self.file.read(16 * nh), mpq_hash('(hash table)', 3))))
        self.file.seek(bt)
        self.blocks = list(struct.iter_unpack('<IIII', decrypt(self.file.read(16 * nb), mpq_hash('(block table)', 3))))
    def read(self, name):
        a, b = mpq_hash(name, 1), mpq_hash(name, 2)
        matches = [entry for entry in self.hashes if entry[0:2] == (a, b) and entry[4] < len(self.blocks)]
        if not matches:
            return None
        matches.sort(key=lambda row: row[2] != 0)
        off, packed, size, flags = self.blocks[matches[0][4]]
        assert not flags & 0x10000, 'Encrypted MPQ entry unsupported'
        self.file.seek(off)
        data = self.file.read(packed)
        if not flags & 0x200:
            assert len(data) >= size
            return data[:size]
        if flags & 0x1000000:
            return decode(data, size)
        count = (size + self.sector - 1) // self.sector
        offsets = struct.unpack('<%dI' % (count + 1), data[:4 * (count + 1)])
        return b''.join(decode(data[offsets[i]:offsets[i + 1]], min(self.sector, size - i * self.sector)) for i in range(count))

class Client:
    def __init__(self, game):
        self.archives = [MPQ(game / 'Data' / n) for n in ['patch-2.MPQ', 'patch.MPQ', 'model.MPQ', 'dbc.MPQ']]
    def read(self, name):
        for archive in self.archives:
            data = archive.read(name)
            if data is not None:
                return data, archive.path.name
        raise FileNotFoundError(name)
