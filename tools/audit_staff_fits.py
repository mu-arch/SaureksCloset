"""Measure vanilla torso clearance along each authored sheathed-staff shaft.

WOW_DATA or --data selects the installed build-5875 Data directory. Archives
are read in patch precedence; no client meshes are written or bundled. Install
mpyq, or pass --mpyq PATH to a local mpyq.py. --write regenerates StaffFits.h;
the default verifies the numeric fit table against the installed assets.
"""
import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import struct


RACES = ("Human", "Orc", "Dwarf", "NightElf", "Scourge", "Tauren", "Gnome", "Troll")
PATCH_Y = (-.02, 0., .02)
PATCH_Z = (-.04, -.02, 0., .02, .04)
SHAFT_SAMPLES = tuple(-1.2 + .01 * i for i in range(241))


def body_geometry(data):
    """Use the same base body/geoset selection as audit_bag_fits.py."""
    assert data[:8] == b"MD20\0\1\0\0", "Expected a build-5875 M2 model"
    count, offset = struct.unpack_from("<II", data, 0x44)
    vertices = [struct.unpack_from("<3f", data, offset + 48 * i) for i in range(count)]
    view = struct.unpack_from("<I", data, 0x50)[0]
    count, offset = struct.unpack_from("<II", data, view)
    indices = struct.unpack_from("<" + "H" * count, data, offset)
    count, offset = struct.unpack_from("<II", data, view + 8)
    triangles = struct.unpack_from("<" + "H" * count, data, offset)
    count, offset = struct.unpack_from("<II", data, view + 24)
    sections = [struct.unpack_from("<10H3f", data, offset + 32 * i) for i in range(count)]
    groups = {}
    for section in sections:
        group = section[0] // 100
        if group and group != 15:
            groups[group] = min(groups.get(group, section[0]), section[0])
    faces = []
    for section in sections:
        if section[0] == 0 or section[0] in groups.values():
            faces.extend(tuple(vertices[indices[triangles[t + j]]] for j in range(3))
                         for t in range(section[4], section[4] + section[5], 3))
    return faces


def attachment(data, point):
    count, offset = struct.unpack_from("<II", data, 0x104)
    lookup_count, lookup = struct.unpack_from("<II", data, 0x10C)
    assert point < lookup_count <= 512 and 0 < count <= 512
    index = struct.unpack_from("<H", data, lookup + 2 * point)[0]
    assert index < count
    actual, bone, _, x, y, z = struct.unpack_from("<IHH3f", data, offset + 48 * index)
    assert actual == point and bone < struct.unpack_from("<I", data, 0x34)[0]
    return (x, y, z)


def shaft_direction(data, point):
    """The staff's long axis is local X; attachment helper poses are constant.

    Follow the authored quaternion and scale instead of guessing the diagonal
    from the grip position. Torso ancestors animate the body and staff together;
    this audit measures their common neutral model space, not arbitrary poses.
    """
    offset = struct.unpack_from("<I", data, 0x108)[0]
    lookup = struct.unpack_from("<I", data, 0x110)[0]
    index = struct.unpack_from("<H", data, lookup + 2 * point)[0]
    bone = struct.unpack_from("<H", data, offset + 48 * index + 4)[0]
    bone_offset = struct.unpack_from("<I", data, 0x38)[0] + bone * 108
    pivot = struct.unpack_from("<3f", data, bone_offset + 96)
    assert all(abs(a - p) < .00001 for a, p in zip(attachment(data, point), pivot))
    translation = struct.unpack_from("<Hh6I", data, bone_offset + 12)
    rotation = struct.unpack_from("<Hh6I", data, bone_offset + 40)
    scale = struct.unpack_from("<Hh6I", data, bone_offset + 68)
    assert translation[-2] == 0, "Unexpected staff-helper translation track"
    assert rotation[-2] == scale[-2] == 1, "Expected constant staff-helper rotation/scale"
    x, y, z, w = struct.unpack_from("<4f", data, rotation[-1])
    sx, sy, sz = struct.unpack_from("<3f", data, scale[-1])
    assert abs(x*x + y*y + z*z + w*w - 1.) < .002
    assert math.isfinite(sx) and 0 < sx < 2 and abs(sx - sy) < .00001 and abs(sx - sz) < .00001
    return ((1 - 2*(y*y + z*z))*sx, 2*(x*y + z*w)*sx, 2*(x*z - y*w)*sx)


def rear_surface(faces, y, z):
    hits = []
    for a, b, c in faces:
        u = (b[1] - a[1], b[2] - a[2])
        v = (c[1] - a[1], c[2] - a[2])
        determinant = u[0] * v[1] - u[1] * v[0]
        if abs(determinant) < 1e-9:
            continue
        p = ((y - a[1]) * v[1] - (z - a[2]) * v[0]) / determinant
        q = (u[0] * (z - a[2]) - u[1] * (y - a[1])) / determinant
        if p >= 0 and q >= 0 and p + q <= 1:
            hits.append(a[0] + p * (b[0] - a[0]) + q * (c[0] - a[0]))
    return min(hits) if hits else None


def contact(faces, anchor):
    surfaces = [rear_surface(faces, anchor[1] + dy, anchor[2] + dz)
                for dy in PATCH_Y for dz in PATCH_Z]
    present = [value for value in surfaces if value is not None]
    # The rearmost point of this small patch is conservative for body contact.
    # Runtime must still leave room for the selected shaft's radius and a gap.
    return (min(present) - anchor[0] if present else None), len(present)


def shaft_contact(faces, anchor, direction):
    samples = []
    for distance in SHAFT_SAMPLES:
        point = tuple(anchor[axis] + direction[axis] * distance for axis in range(3))
        # Include the full torso below the grip. Grip-only measurements can
        # greatly overestimate clearance where the neck curves away from it.
        if not anchor[2] - .6 <= point[2] <= anchor[2] + .04:
            continue
        inward, coverage = contact(faces, point)
        if inward is not None:
            samples.append((inward, distance, point, coverage))
    return min(samples) if samples else None


def render_header(rows, provenance):
    header = ["// Generated numeric sheathed-staff contact fits; no client model geometry.",
              "// Verify with tools/audit_staff_fits.py against build 5875.",
              "// Neutral-space +X points inward. Scan the authored shaft across the torso:",
              "// grip Z -0.6..+0.04, patches Y +/-0.02 and Z +/-0.04, shaft step 0.01.",
              "// Minimum clearance reaches the rear body surface; subtract shaft radius/gap.",
              "#pragma once", "#include <array>",
              "struct StaffFit { unsigned race,sex,point; std::array<float,3> anchor; float inward; };",
              "static constexpr StaffFit staffFits[]={"]
    for row in rows:
        assert row["inward"] is not None, f"No surface coverage: {row['name']} point {row['point']}"
        coords = ",".join(f"{value:.9f}f" for value in row["anchor"])
        header.append("    {%d,%d,%d,{{%s}},%.9ff}, // %s" %
                      (row["race"], row["sex"], row["point"], coords,
                       row["inward"], row["name"]))
    header.extend(["};", "// SHA-256 of the local source models:"])
    for name, digest in provenance:
        header.append(f"// {name}: {digest}")
    return "\n".join(header) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, default=os.environ.get("WOW_DATA"))
    parser.add_argument("--mpyq", type=Path)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    if not args.data:
        parser.error("Set WOW_DATA or pass --data with the installed WoW Data directory")
    if args.mpyq:
        spec = importlib.util.spec_from_file_location("staff_audit_mpyq", args.mpyq)
        mpyq = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mpyq)
    else:
        try:
            import mpyq
        except ImportError:
            parser.error("mpyq is unavailable; pass --mpyq with an existing local mpyq.py")
    archives = []
    rows, provenance = [], []
    try:
        for name in ("patch-2.MPQ", "patch.MPQ", "model.MPQ"):
            archives.append(mpyq.MPQArchive(str(args.data / name), listfile=False))
        for race, name in enumerate(RACES, 1):
            for sex, label in enumerate(("Male", "Female")):
                filename = f"Character\\{name}\\{label}\\{name}{label}.m2"
                data = next((data for archive in archives
                             if (data := archive.read_file(filename)) is not None), None)
                assert data, f"Missing model: {filename}"
                provenance.append((name + " " + label, hashlib.sha256(data).hexdigest()))
                faces = body_geometry(data)
                for point in (30, 31):
                    anchor = attachment(data, point)
                    grip_inward, grip_samples = contact(faces, anchor)
                    direction = shaft_direction(data, point)
                    closest = shaft_contact(faces, anchor, direction)
                    rows.append(dict(race=race, sex=sex, name=name + " " + label,
                                     point=point, anchor=anchor, direction=direction,
                                     inward=closest[0] if closest else None,
                                     samples=closest[3] if closest else 0,
                                     shaft_distance=closest[1] if closest else None,
                                     contact_position=closest[2] if closest else None,
                                     grip_inward=grip_inward, grip_samples=grip_samples))
    finally:
        for archive in archives:
            archive.file.close()
    for row in rows:
        depth = "MISSING" if row["inward"] is None else f"{row['inward']:+.6f}"
        grip = "MISSING" if row["grip_inward"] is None else f"{row['grip_inward']:+.6f}"
        distance = "MISSING" if row["shaft_distance"] is None else f"{row['shaft_distance']:+.2f}"
        notes = []
        if row["samples"] < len(PATCH_Y) * len(PATCH_Z):
            notes.append("partial surface coverage")
        if row["inward"] is not None and row["inward"] < 0:
            notes.append("shaft already inward of rear body surface")
        print(f"{row['name']:17s} point {row['point']} inward={depth} "
              f"samples={row['samples']}/15 grip={grip} "
              f"shaftX={distance}" + ("; " + "; ".join(notes) if notes else ""))
    output = render_header(rows, provenance)
    path = Path(__file__).resolve().parents[1] / "native/StaffFits.h"
    if args.write:
        path.write_text(output)
    else:
        assert path.read_text() == output, "Staff fits differ; inspect installed models before --write"
    if args.json:
        print(json.dumps({"rows": rows, "sha256": dict(provenance)}, indent=2))
    print("PASS: 32 staff-shaft body clearances verified against all 16 installed character models.")


if __name__ == "__main__":
    main()
