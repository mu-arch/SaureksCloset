"""Measure staff shafts through the torso contact band; never copy model geometry.

Only four transverse bounds over model-local X[-1.0,0.15] are retained in
StaffShafts.h. This conservative envelope includes lower-shaft decorations
near the torso instead of assuming the grip is the shaft's widest section.
The x=0 grip section is also printed for comparison. By default
verify that table against local build-5875 models; --write regenerates it.
Set WOW_DATA or pass --data for the client's Data directory. A standalone mpyq
module can be supplied with --mpyq; otherwise the installed mpyq is used.
"""
import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import struct
import sys


ROOT = Path(__file__).resolve().parents[1]
CONTACT_MIN_X = -1.0
CONTACT_MAX_X = 0.15


def staff_models():
    """Map each selected staff model to the item IDs which use it."""
    models = {}
    for line in (ROOT / 'native/WeaponAssets.h').read_text().splitlines():
        if not re.match(r'^\{\d+,', line):
            continue
        row = json.loads('[' + line.rstrip(',')[1:-1] + ']')
        if tuple(row[2:6]) == (2, 10, 17, 2):
            models.setdefault(row[6], []).append(row[0])
    if not models:
        raise ValueError('No two-handed staff models found in WeaponAssets.h')
    return models


def mesh(data):
    """Read the build-5875 M2's first-view triangles using its vertex lookup."""
    if len(data) < 0x54 or data[:8] != b'MD20\x00\x01\x00\x00':
        raise ValueError('Expected a build-5875 M2')
    count, offset = struct.unpack_from('<II', data, 0x44)
    if not count or offset + 48 * count > len(data):
        raise ValueError('Invalid vertex array')
    vertices = [struct.unpack_from('<3f', data, offset + 48 * i)
                for i in range(count)]
    if not all(math.isfinite(value) for vertex in vertices for value in vertex):
        raise ValueError('Non-finite vertex coordinate')
    views, view = struct.unpack_from('<II', data, 0x4C)
    if not views or view + 44 > len(data):
        raise ValueError('Missing first skin view')
    count, offset = struct.unpack_from('<II', data, view)
    if not count or offset + 2 * count > len(data):
        raise ValueError('Invalid view vertex lookup')
    indices = struct.unpack_from('<' + 'H' * count, data, offset)
    count, offset = struct.unpack_from('<II', data, view + 8)
    if not count or count % 3 or offset + 2 * count > len(data):
        raise ValueError('Invalid view triangle array')
    triangles = struct.unpack_from('<' + 'H' * count, data, offset)
    if any(index >= len(vertices) for index in indices):
        raise ValueError('Vertex lookup references an absent vertex')
    if any(index >= len(indices) for index in triangles):
        raise ValueError('Triangle references an absent lookup entry')
    faces = [tuple(indices[triangles[i + j]] for j in range(3))
             for i in range(0, count, 3)]
    return vertices, faces


def section_bounds(vertices, faces, min_x, max_x):
    """Clip triangle edges to an X slab and bound the resulting Y/Z coordinates.

    A low-polygon shaft can have no vertices near its grip. Interpolating the
    triangle edges at both slab planes captures that shaft. Vertices contained
    within the slab retain every local flare or ornament. The clipped triangle
    is convex, so its extrema occur at those contained vertices/intersections.
    Setting min_x == max_x measures one cross-section without invented padding.
    """
    if min_x > max_x:
        raise ValueError('Invalid shaft measurement slab')
    hits = set()
    for face in faces:
        points = [vertices[index] for index in face]
        for point in points:
            if min_x <= point[0] <= max_x:
                hits.add((point[1], point[2]))
        for a, b in zip(points, points[1:] + points[:1]):
            for plane in (min_x, max_x):
                if (a[0] < plane < b[0]) or (b[0] < plane < a[0]):
                    fraction = (plane - a[0]) / (b[0] - a[0])
                    hits.add(tuple(a[axis] + fraction * (b[axis] - a[axis])
                                   for axis in (1, 2)))
    if not hits:
        raise ValueError(f'No triangle geometry in X[{min_x},{max_x}]')
    min_y, max_y = min(p[0] for p in hits), max(p[0] for p in hits)
    min_z, max_z = min(p[1] for p in hits), max(p[1] for p in hits)
    if min_y >= max_y or min_z >= max_z:
        raise ValueError(f'Degenerate shaft envelope in X[{min_x},{max_x}]')
    return (min_y, max_y, min_z, max_z), len(hits)


def shaft_bounds(vertices, faces):
    return section_bounds(vertices, faces, CONTACT_MIN_X, CONTACT_MAX_X)


def make_header(rows):
    header = (
        '// Generated numeric staff envelope over model-local X[-1.0,0.15].\n'
        '// Triangle edges are clipped to this torso contact band; contained\n'
        '// vertices retain lower-shaft flares instead of measuring only x=0.\n'
        '// Verify with tools/audit_staff_shafts.py against build 5875.\n'
        '// Paths and four bounds only; no client model geometry.\n'
        '#pragma once\n'
        '#include <cstring>\n'
        'struct StaffShaft { const char* model; float minY,maxY,minZ,maxZ; };\n'
        'static constexpr StaffShaft staffShafts[]={\n'
    )
    for model, bounds in rows:
        header += '    {' + json.dumps(model) + ','
        header += ','.join(f'{value:.9f}f' for value in bounds) + '},\n'
    header += (
        '};\n'
        'inline const StaffShaft* staffShaftFor(const char* model){\n'
        '    if(!model)return nullptr;\n'
        '    for(const auto& shaft:staffShafts)\n'
        '        if(std::strcmp(model,shaft.model)==0)return &shaft;\n'
        '    return nullptr;\n'
        '}\n'
    )
    return header


def load_mpyq(path):
    if path:
        spec = importlib.util.spec_from_file_location('staff_audit_mpyq', path)
        if not spec or not spec.loader:
            raise ValueError(f'Cannot import MPQ reader: {path}')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    import mpyq
    return mpyq


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, default=os.environ.get('WOW_DATA'),
                        help='Local build-5875 Data directory (or set WOW_DATA)')
    parser.add_argument('--mpyq', type=Path, help='Path to a standalone mpyq.py')
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    if not args.data:
        parser.error('Provide --data or set WOW_DATA to the client Data directory')
    models = staff_models()
    mpyq = load_mpyq(args.mpyq)
    archives, rows, failures = [], [], []
    try:
        for name in ['patch-2.MPQ', 'patch.MPQ', 'model.MPQ']:
            path = args.data / name
            if path.is_file():
                archives.append((name, mpyq.MPQArchive(str(path), listfile=False)))
        if not archives:
            raise ValueError(f'No model archives in {args.data}')
        for model in sorted(models):
            path = re.sub(r'\.mdx$', '.m2', model, flags=re.IGNORECASE)
            try:
                data, source = None, None
                for name, archive in archives:
                    data = archive.read_file(path)
                    if data is not None:
                        source = name
                        break
                if not data:
                    raise ValueError('Model path missing from local archives')
                vertices, faces = mesh(data)
                bounds, hits = shaft_bounds(vertices, faces)
                grip, grip_hits = section_bounds(vertices, faces, 0.0, 0.0)
                rows.append((model, bounds))
                print(f'{model}: {len(models[model])} items; '
                      f'{hits} band points over X[{CONTACT_MIN_X},{CONTACT_MAX_X}]; '
                      f'Y=[{bounds[0]:.9f},{bounds[1]:.9f}] '
                      f'Z=[{bounds[2]:.9f},{bounds[3]:.9f}]; '
                      f'x=0 ({grip_hits} points) '
                      f'Y=[{grip[0]:.9f},{grip[1]:.9f}] '
                      f'Z=[{grip[2]:.9f},{grip[3]:.9f}]; '
                      f'{source}; SHA256 {hashlib.sha256(data).hexdigest()}')
            except (ValueError, RuntimeError, NotImplementedError, struct.error) as error:
                failures.append((model, str(error)))
                print(f'UNMEASURED {model}: {error}', file=sys.stderr)
    finally:
        for _, archive in archives:
            archive.close()
    if failures:
        print(f'FAIL: {len(failures)} of {len(models)} staff models unmeasured; '
              'the generated table was not changed.', file=sys.stderr)
        return 1
    header = make_header(rows)
    destination = ROOT / 'native/StaffShafts.h'
    if args.write:
        destination.write_text(header)
    elif not destination.is_file() or destination.read_text() != header:
        print('FAIL: staff shaft bounds differ; review the installed models '
              'before regenerating with --write.', file=sys.stderr)
        return 1
    print(f'PASS: measured {len(rows)} staff shafts used by '
          f'{sum(len(items) for items in models.values())} items; '
          f'table SHA256 {hashlib.sha256(header.encode()).hexdigest()}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
