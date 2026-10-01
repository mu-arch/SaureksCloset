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

The runtime uses `CapeCloth.h` at a fixed 60 Hz, at most three substeps and
eight constraint iterations, with world-space gravity and inertia, compliant
edge-length and triangle-area constraints, and approximate
bending constraints across adjacent triangles. Free motion therefore reacts to
the real shoulder movement and contact geometry. The solver does not generate
a replacement rectangular plane or periodic sway animation.

`CapeBodyBounds.h` fits conservative oriented boxes to complete visible body
and equipment triangles when their geometry/visibility changes. Weighted joint
faces remain inside one bound: cached per-influence intervals enclose their
skinned vertices, and a convex bound encloses the complete triangle. Spatial
splits tighten the fit. Animation updates pose these compact intervals instead
of skinning and traversing the full body mesh. The body uses at most 32 boxes,
each attachment at most four, with at most 64 relevant boxes in the solver.

The live runtime fixes the shoulder/collar vertices at their exact native
skinned positions, including during collision recovery. Those vertices are
passed through unchanged in GPU and CPU output. Contacts cannot move the
attachment or translate the entire garment. Faces incident to sewn vertices
are excluded from coarse box contact; these boxes are not a reliable model of
the actual neckline. Free faces still use solid contact checks. Contacts and
length/area constraints alternate before the final rendering check.

The motion reference is the current skinned cape, not the unskinned mesh at the
model origin. Before submission, `capePoseFits` checks exact seam positions,
vertex displacement, every edge (5% plus 1 mm numerical allowance), triangle
area and orientation. An incompatible result uses the original native cape draw
and reports status 4. The fallback remains until Reset cape motion, toggling
physics, or changing the cape/model; it does not alternate between normal and
simulated draws each frame. This protects attachment and shape. It does NOT
establish collision-free cloth for an incompatible body/equipment fit.

Immutable mesh arrays and prepared GPU bytes are cached. Only visibility,
bone palettes and camera transforms refresh for ordinary animation. Detailed
world contact is separate: nearby terrain/WMO/M2 triangles are cached for up to
100 ms in an expanded region, refreshed immediately when the cape leaves it,
and limited to 128 nearby faces. The runtime disables the old auxiliary contact
sample cloud and particle self-contact. Unavailable scenery or an exhausted
scenery budget cannot disable the solid body limits. Normals are rebuilt from
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
| `0x58A830` | Primitive submission (`fastcall`, descriptor and indexed flag) |
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
  cape, 2 active, 3 unavailable, or 4 normal cape motion because the simulated
  fit was rejected. The page names that fallback explicitly and permits reset.
- `/closet diagnose` includes `SaureksClosetInspectCapePhysics()` output:
  schema, bridge ready, enabled, status, player available, mesh readable,
  visible authored cape sections, simulation ready, vertex and triangle counts,
  GPU mode, optimized group count, contact count, collision-budget status,
  active bound count, bound tests, world tests and rejected-bound status (schema 2).
  These counts distinguish a genuinely hidden cape from a render-path problem;
  the report contains no native pointers or player identifiers.
- Teleports and model changes reset the simulation. Slow frames limit catch-up
  work. Shape/attachment failures use native motion, never a skipped cape draw,
  an offset seam or a globally translated collision solution.
- Collision work, cape size, attachment traversal and query bounds are bounded.
  This is a one-player simulation, not scene-wide cloth on every character.
- World collisions follow the client's collision mesh, which can differ from
  visible artwork. Non-collidable scenery is not automatically a surface.
  Cached world geometry is approximate for moving platforms and other moving
  scenery; it has no stable native face IDs for continuous motion tracking.
- Final whole-face clearance applies to simulated free faces and fitted bounds.
  Sewn faces and native fallback are not covered by that guarantee.
  Swept detection conservatively tests relative bounds; it is not a general
  continuous deforming-mesh proof. Detailed world contact remains vertex-based,
  so thin scenery can pass between coarse cape vertices. Self-collision and
  fabric untangling are disabled in this cheaper runtime mode.
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

The cheaper path was additionally checked against all 80 actual capes with
body bounds for 120 simulation steps each, plus GPU48/CPU32/CPU40 optimized
submissions (240 paths, 480 draws). A static HumanFemale benchmark measured
0.432 ms/frame for the previous solver and 0.134 ms/frame for the bounded solver;
posing/caching plus two mocked GPU draws measured 0.165 ms/frame. These host
measurements exclude live graphics-driver and native world-query time and are
not an in-game FPS claim.

Initial engine navigation references:
[client collision research](https://github.com/samwhosung/wow-1121-client-internals)
and [model research](https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/models.md).
Function addresses and calling conventions above were independently checked
against the supported local executable.

Stability regressions cover collision corrections without launch impulses,
overlapping bounds, continuous drawing on budget exhaustion, bounded motion,
and slow frames without authored-pose resets. An offline sequence of root turns,
translations, vertical movement and frame stalls exercised 240 actual-model
render paths for 120 updates each. This does not reproduce live limb animations
or establish that all in-game scenery clipping is eliminated.

Attachment regression validation: all 240 GPU48/CPU32/CPU40 paths rendered a
cape with no missing submissions; 192 of 480 material draws used accepted
simulation, and 288 used the explicit native fallback. The current coarse
bounds are still incompatible with many authored fits. Tests now reject
shoulder translation, elongated edges and collapsed faces, and check that a
small valid flex remains simulated. This is fit protection, not a completed
replacement for the client's cape animation on every model.
