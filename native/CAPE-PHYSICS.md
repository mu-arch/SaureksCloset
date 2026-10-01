# Real-time cape physics

The Wardrobe → Physics page controls a character-specific, opt-in cape
simulation. It is independent of saved looks and bag animations. Disabling the
addon or cape physics restores the client's original cape draw immediately.
Only the current player's world model is simulated; wardrobe and Character
window previews retain their original animation.

## Geometry and integration

`CapeRenderer.h` reads the selected MD20 view and visible cape sections, including
their actual topology and skin weights. It requires both the 1500–1599 geoset
family and cape texture type 2: the geoset number alone also identifies Tauren
tails and some skin panels. Coincident, identically weighted seam vertices are
welded for simulation and mapped back to the original draw vertices. The top
seam and short collar sections follow the animated skeleton; free vertices are
not pulled toward a prerecorded cape animation.

Native optimized draw sections do not retain their authored geoset IDs. The
client copies texture units/sections into `model+0x3EC`/`model+0x3F4` and replaces
section IDs with compact group numbers. `model+0x3FC` maps each group to an
inclusive range of original texture units. Cape detection resolves that range
before checking original cape materials and visibility. GPU indices retain
their view positions; CPU batches concatenate the visible original sections
and rebase their indices. Simulation vertices are mapped to those draw spans.
This path is regression-tested separately from parsing the original M2 files.

`CapeCloth.h` integrates world-space gravity and inertia with a fixed 120 Hz
step, compliant edge-length and triangle-area constraints, and approximate
bending constraints across adjacent triangles. Free motion therefore reacts to
the real shoulder movement and contact geometry. The solver does not generate
a replacement rectangular plane or periodic sway animation.

Visible body and loaded attachment meshes supply animated collision triangles.
The world query supplies nearby terrain, WMO and M2 collision triangles. Contact
is two-sided, has finite thickness and friction, and sweeps moving character
triangles between samples. Additional barycentric surface samples improve
coverage between the original low-poly cape vertices. Normals are rebuilt from
the resulting cloth triangles.

Both CPU-skinned and GPU-skinned native draws use a private owned vertex
buffer. GPU draws undo the current weighted skin transform for the simulated
vertices, then let the normal client shader apply it. The native index buffer,
UVs, materials and texture selection are retained. The exact previous vertex
binding is restored after submission. Shared model geometry, player update
fields and other characters are never modified.

## Verified build 5875 interfaces

Entry points and layouts were checked against the executable recorded in
`CLIENT-BUILD.json`. Optional cape signatures are separate from the existing
wardrobe signatures so a graphics-path incompatibility does not disable the
rest of the addon.

| Address | Purpose |
| --- | --- |
| `0x70CB30` | Native model-batch draw scope (`thiscall`) |
| `0x58A830` | Primitive submission (`fastcall`, descriptor and count) |
| `0x58A7C0` | Vertex-buffer binding (`fastcall`, buffer and format) |
| `0x58A160`, `0x589F80` | Private graphics pool and vertex-buffer allocation |
| `0x58A080`, `0x58A0A0` | Map and unmap the owned buffer |
| `0x594550`, `0x58A1A0` | Release the owned buffer and graphics pool |
| `0x71A460`, `0x71A720`, `0x71A9E0` | Supported CPU skinning paths |
| `0x6721B0` | World AABB collision query (`fastcall`, two register arguments and two stack arguments) |
| `0x646430` | Client-heap free (`stdcall`, four arguments) |

The world query uses an AABB of two three-float vectors and two 16-byte native
arrays. Each face is 52 bytes: plane normal, plane distance, and three inline
world-space vertices. Its walking mask is `0x100111`, verified at `0x6315F0`;
the walkable-slope-only filter and liquid flags are deliberately excluded so
walls remain collidable. Native query allocations are released with the client
allocator, never the DLL's C++ allocator.

The renderer does **not** use `0x719DF2` as a vertex skinning hook: inspection
shows that address is in section-bounds computation, not final vertex output.
It also does not allocate from `0x58A140`, which reuses the client's shared
temporary descriptor. Separate owned pools preserve both the previous binding
and the original buffer's range. Direct owned-buffer cleanup avoids `0x58A040`,
whose default-buffer check itself mutates that shared temporary descriptor.

## Runtime contract and limits

- `SaureksClosetSetCapePhysics(0/1)` and `SaureksClosetResetCapePhysics()` return
  1 on acceptance, 0 if unavailable or invalid.
- `SaureksClosetCapePhysicsStatus()` returns 0 disabled, 1 waiting for a visible
  cape, 2 active, or 3 unavailable for the current model/rendering path.
- `/closet diagnose` includes `SaureksClosetInspectCapePhysics()` output:
  schema, bridge ready, enabled, status, player available, mesh readable,
  visible authored cape sections, simulation ready, vertex and triangle counts,
  GPU mode, optimized group count, contact count, and collision-budget status.
  These counts distinguish a genuinely hidden cape from a render-path problem;
  the report contains no native pointers or player identifiers.
- Teleports, model changes and long frame gaps reset the simulation. Invalid
  geometry, unsupported batches or exceeded work limits use the original cape
  draw instead of publishing partial or non-finite cloth state.
- Collision work, cape size, attachment traversal and query bounds are bounded.
  This is a one-player simulation, not scene-wide cloth on every character.
- World collisions follow the client's collision mesh, which can differ from
  visible artwork. Non-collidable scenery is not automatically a surface.
  World triangles are refreshed each update; unlike character attachments,
  they currently have no stable face IDs for swept moving-platform contact.
- Surface sampling reduces gaps but is not an exact continuous mesh/mesh
  collision guarantee. Self-contact separates particles; it is not full
  triangle/triangle untangling. Very thin objects and tight folds remain cases
  to test in-game.
- Original cape meshes are coarse (roughly 42–49 vertices in the inspected main
  panels). They can bend at their authored vertices; this does not add visual
  tessellation or high-resolution fabric wrinkles.

Tests cover the solver's motion and contacts, native geometry conversion,
render-buffer isolation and restoration, and the Lua page/lifecycle. These
offline checks and compilation do not substitute for visual verification in a
running client.

Read-only validation against the installed client also exercised all 80 cape
variants (eight races, both sexes, five lengths), using the real renderer's mesh
selection, topology and skin weights, then 120 solver steps for each. It found
and fixed disconnected short-cape collar pieces on Tauren females. All variants
then passed with matching CPU/GPU topology and no orphan vertices, duplicate or
degenerate triangles, or unanchored components. A separate reconstruction of the
client's optimized batches exercised all 80 variants in GPU and CPU mode (160
paths, 320 draws), verifying output triangle indices, UVs and binding restoration.
This reproduces native descriptors offline; it is not a live-frame capture. Raw
game assets are not included in the repository.

Initial engine navigation references:
[client collision research](https://github.com/samwhosung/wow-1121-client-internals)
and [model research](https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/models.md).
Function addresses and calling conventions above were independently checked
against the supported local executable.
