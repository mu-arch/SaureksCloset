"""Convert the original Dark Schoolbag GLB to a rigid build-5875 M2 and mipmapped BLP.

No client meshes are used. Vanilla layout: wowdev/pywowlib m2_format.py and
skin_format.py, cross-checked against the installed 1.12.1 loader/rigid props.
The original mesh and painted atlas are preserved; no second texture bake.
"""
import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import struct as S
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def read_glb(path):
    data = path.read_bytes()
    assert data[:4] == b'glTF' and S.unpack_from('<II', data, 4) == (2, len(data))
    size, kind = S.unpack_from('<II', data, 12)
    assert kind == 0x4e4f534a
    doc = json.loads(data[20:20+size])
    length, kind = S.unpack_from('<II', data, 20+size)
    assert kind == 0x004e4942
    binary = data[28+size:28+size+length]
    assert len(doc['nodes']) == 1 and doc['nodes'][0].get('mesh') == 0
    assert not any(k in doc['nodes'][0] for k in ('matrix', 'translation', 'rotation', 'scale', 'skin'))

    def accessor(index):
        a = doc['accessors'][index]; v = doc['bufferViews'][a['bufferView']]
        assert 'sparse' not in a and not a.get('normalized') and v.get('buffer', 0) == 0
        width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3}[a['type']]
        fmt = '<' + {5123: 'H', 5126: 'f'}[a['componentType']] * width
        stride = v.get('byteStride', S.calcsize(fmt))
        offset = v.get('byteOffset', 0) + a.get('byteOffset', 0)
        return [S.unpack_from(fmt, binary, offset+i*stride) for i in range(a['count'])]

    vertices, triangles = [], []
    for p in doc['meshes'][0]['primitives']:
        assert p.get('mode', 4) == 4
        material = doc['materials'][p['material']]
        tex = doc['textures'][material['pbrMetallicRoughness']['baseColorTexture']['index']]
        assert tex['source'] == 0, 'All materials must share the original painted atlas'
        pos, normals, uv = [accessor(p['attributes'][k]) for k in ('POSITION', 'NORMAL', 'TEXCOORD_0')]
        assert len(pos) == len(normals) == len(uv)
        start = len(vertices)
        for xyz, normal, coord in zip(pos, normals, uv):
            # glTF +Y up/+Z outward -> WoW +Z up/-X outward. +Y is anatomical left.
            vertices.append(((-xyz[2], -xyz[0], xyz[1]), (-normal[2], -normal[0], normal[1]), coord))
        indices = [i[0] for i in accessor(p['indices'])]
        assert len(indices) % 3 == 0 and max(indices) < len(pos)
        triangles.extend(start+i for i in indices)
    rear = max(p[0] for p, n, uv in vertices)
    mid_y = (max(p[1] for p,n,uv in vertices)+min(p[1] for p,n,uv in vertices))/2
    low_z=min(p[2] for p,n,uv in vertices);high_z=max(p[2] for p,n,uv in vertices)
    mount_z=(low_z+high_z)/2
    # The origin is the center of the back panel that rests against the player.
    vertices = [((p[0]-rear, p[1]-mid_y, p[2]-mount_z), n, uv) for p,n,uv in vertices]
    assert len(vertices) < 65536 and len(triangles) < 65536
    v = doc['bufferViews'][doc['images'][0]['bufferView']]
    atlas = binary[v['byteOffset']:v['byteOffset']+v['byteLength']]
    return vertices, triangles, atlas


def write_m2(vertices, triangles, path):
    data = bytearray(0x144)
    S.pack_into('<4sI', data, 0, b'MD20', 256)

    def block(blob):
        data.extend(b'\0' * (-len(data) % 16)); offset = len(data); data.extend(blob); return offset

    def array(at, count, blob):
        S.pack_into('<II', data, at, count, block(blob) if count else 0)

    low = tuple(min(v[0][i] for v in vertices) for i in range(3))
    high = tuple(max(v[0][i] for v in vertices) for i in range(3))
    radius = max(math.sqrt(sum(c*c for c in v[0])) for v in vertices)
    bounds = S.pack('<7f', *low, *high, radius)
    data[0xb4:0xd0] = bounds
    model_name=b'DarkSchoolbag\0'
    array(8, len(model_name), model_name)
    sequence = S.pack('<HHIIfIhHIII', 0, 0, 0, 1000, 0, 0, 32767, 0, 0, 0, 0) + bounds + S.pack('<hH', -1, 0)
    assert len(sequence) == 68
    array(0x1c, 1, sequence)
    array(0x24, 1, S.pack('<h', 0))
    array(0x2c, 203, S.pack('<HH', 0, 0)*203)
    track = S.pack('<Hh6I', 0, -1, 0, 0, 0, 0, 0, 0)
    bone = S.pack('<iIhH', -1, 0, -1, 0) + track*3 + S.pack('<3f', 0, 0, 0)
    assert len(bone) == 108
    array(0x34, 1, bone)
    array(0x3c, 1, S.pack('<h', -1))
    array(0x44, len(vertices), b''.join(S.pack('<3f8B3f4f', *p, 255,0,0,0, 0,0,0,0, *n, *uv, 0,0) for p,n,uv in vertices))
    view = block(bytes(44)); S.pack_into('<II', data, 0x4c, 1, view)
    array(view, len(vertices), S.pack('<'+'H'*len(vertices), *range(len(vertices))))
    array(view+8, len(triangles), S.pack('<'+'H'*len(triangles), *triangles))
    array(view+16, len(vertices), bytes(4*len(vertices)))
    center = [(a+b)/2 for a,b in zip(low,high)]
    array(view+24, 1, S.pack('<10H3f', 0,0,0,len(vertices),0,len(triangles),1,0,1,0,*center))
    array(view+32, 1, S.pack('<4Hh7H', 16,0,0,0,-1,0,0,1,0,0,0,0))
    S.pack_into('<I', data, view+40, 21)
    texture = b'Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.blp\0'
    name = block(texture)
    array(0x5c, 1, S.pack('<4I', 2,0,len(texture),name))
    ranges = block(S.pack('<2I',0,0)); times = block(S.pack('<I',0)); values = block(S.pack('<h',32767))
    array(0x64, 1, S.pack('<Hh6I',0,-1,1,ranges,1,times,1,values))
    array(0x7c, 3, S.pack('<3h',-1,-1,0))
    array(0x84, 1, S.pack('<HH',4,0))  # Lit, opaque, two-sided like the source.
    for at, value in [(0x8c,0),(0x94,0),(0x9c,0),(0xa4,0),(0xac,-1)]:
        array(at, 1, S.pack('<h',value))
    path.write_bytes(data)
    return dict(vertices=len(vertices),triangles=len(triangles)//3,bounds=[low,high],bytes=len(data))


def write_blp(atlas, path):
    image = Image.open(io.BytesIO(atlas)).convert('RGB')
    assert image.size == (256,256)
    offsets, sizes, chunks = [0]*16, [0]*16, []
    offset = 1172
    for i in range(8):
        mip = image.resize((max(1,128>>i),)*2, Image.Resampling.LANCZOS)
        stream = io.BytesIO();mip.save(stream,format='DDS',pixel_format='DXT1')
        encoded = stream.getvalue();assert encoded[84:88] == b'DXT1'
        chunk = encoded[128:];offsets[i]=offset;sizes[i]=len(chunk);offset+=len(chunk);chunks.append(chunk)
    header = S.pack('<4sI4BII',b'BLP2',1,2,0,0,1,128,128)
    path.write_bytes(header+S.pack('<16I',*offsets)+S.pack('<16I',*sizes)+bytes(1024)+b''.join(chunks))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'assets/DarkSchoolbag.glb')
    args = parser.parse_args()
    output = ROOT/'addon/SaureksCloset/Models';output.mkdir(exist_ok=True)
    vertices, triangles, atlas = read_glb(args.source)
    report = write_m2(vertices,triangles,output/'DarkSchoolbag.m2')
    write_blp(atlas,output/'DarkSchoolbag.blp')
    report.update(source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),texture_sha256=hashlib.sha256(atlas).hexdigest(),blp_bytes=(output/'DarkSchoolbag.blp').stat().st_size)
    (ROOT/'native/BAG-ASSETS.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
