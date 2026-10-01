# Real-time cape physics

Wardrobe → Physics enables NVIDIA NvCloth CPU simulation for the current
player's world cape. The actual selected cape mesh is simulated; no rectangular
replacement or periodic sway is generated. Previews, other players and bags are
unaffected. Disabling the option restores the ordinary native draw.

## Worker and rendering

`CapeWorker.cpp` owns one persistent simulation thread. Only copied positions,
triangle indices, pin indices, collision primitives and numeric settings cross
that boundary. The worker never reads native game memory, invokes game/Lua
functions or touches graphics buffers. Fabric cooking, NvCloth stepping,
contacts and fit limiting happen there.

The renderer submits through a try-lock and never waits for the solver. There
is one replaceable pending job and one latest result, not an unbounded frame
queue. Job time is accumulated so dropped intermediate snapshots do not slow
the simulation. Catch-up is limited to three 60 Hz steps. Mesh/reset/teleport
changes create a new generation; old jobs cannot publish into that generation.
Results more than 250 ms old expire. The normal cape remains visible while a
new simulation is preparing or recovering.

Completed deformation offsets are applied relative to the current animated
cape and rotated with the current player frame. Shoulder/collar vertices always
use the current native coordinates. Consequently an asynchronous result does
not visibly lag the whole garment behind a moving player. The current-frame
fit check is also applied before submitting vertices. CPU/GPU draws use private
buffers; the original shared model, UVs, indices and materials are untouched.

Windows pins the module when first starting the worker, outside DllMain. The
worker and its service live until process exit; there is no thread join under
the loader lock or during a cape reset. Offline tests shut their worker down
normally. The shipped DLL statically links the CPU dependency, with no CUDA or
NVIDIA GPU requirement and no extra DLL to install.

## Simulation and collision

`CapeNvCloth.cpp` cooks topology and geodesic tethers once per mesh/reset.
Pinned particles have zero inverse mass. NvCloth's vertical, horizontal and
shear constraints are stiff; bending is softer. Solver frequency is 240 Hz,
advanced in bounded 60 Hz steps. Damping is applied, aerodynamic lift/drag are
disabled, and tethers resist stretching. Simulation coordinates stay near the
shoulder origin to preserve precision at large world positions.

Conservative animated body/equipment boxes select per-particle NvCloth
separation constraints. These backstop spheres begin at the native fitted cape
surface, rather than pushing the entire cape behind a box's furthest corner.
This avoids the previous reset loop where the native fit overlapped a coarse
box, was pushed outward and then failed shape validation. Existing authored
clipping is not repaired by this allowance. Body collision is an approximation;
it is not a proof that every triangle clears all visible surfaces.

Nearby native scenery collision triangles are cached for up to 100 ms and
limited to 128 faces. NvCloth processes these triangles on the worker. Snapshot
acquisition, body-bound posing, the native world query and drawing still run
on the game thread. Moving simulation off-thread reduces its contribution to
render stalls; it does not make all cape work free.

Motion constraints bound free displacement around the current skinned cape.
`CapeFit.h` checks exact pins, at most 0.18 units of displacement, 5% edge-length
error (or 1 mm allowance), triangle area and orientation. A bounded line search
can retain a safe fraction of the computed deformation and attenuate momentum
instead of resetting the cloth. This prioritizes preserving the garment when a
collision would require stretching. It can reduce collision corrections and
is not an all-surface clearance guarantee. Thin surfaces can pass between the
coarse original cape vertices; self-collision is disabled.

## Native integration (build 5875)

Cape sections require both geosets 1500–1599 and texture type 2; Tauren tail
panels with texture type 8 are excluded. Coincident, equally weighted vertices
are welded. Top seams and disconnected short collars remain attached.

Optimized client groups at model+0x3EC/+0x3F4 use compact IDs. Inclusive source
texture-unit ranges at +0x3FC recover the original cape sections. GPU draws keep
view indices; CPU32/CPU40 groups concatenate/rebase the original ranges. GPU48
vertices are inverse-skinned before the ordinary shader applies its palette.
Camera/palette/visibility refreshes do not recook immutable topology.

Verified hooks are model scope 0x70CB30, submit 0x58A830, bind 0x58A7C0;
private buffer functions 0x58A160/0x589F80/0x58A080/0x58A0A0 and cleanup
0x594550/0x58A1A0. CPU skinning paths are 0x71A460/0x71A720/0x71A9E0.
World AABB query 0x6721B0 returns 52-byte faces with inline vertices, using mask
0x100111 and allocator free 0x646430. Build signatures guard these interfaces.
GPU work always stays on the original render thread with its binding restored.

Status: 0 off, 1 waiting for a visible cape, 2 NvCloth active, 3 unsupported,
4 temporary native fallback with an automatic retry, 5 preparing the worker.
Reset is available for states 2, 4 and 5. `/closet diagnose` retains schema 2:
bridge, preference, status, player/mesh/visibility, initialized state, mesh
counts, graphics mode, optimized groups and last completed worker counters.
The contact count measures active backstops; it is not an exact penetration
count. No native pointers or player identifiers are exported.

## Validation and limits

The mandatory check runs the real NvCloth CPU library, renderer mocks in both
synchronous and asynchronous configurations, GPU48/CPU32/CPU40 mapping,
nonzero deformation, pin/shape guards, world-floor collision, teleports and
invalid inputs. Worker regressions deliberately delay simulation and check
nonblocking submission, replacement queues, generations and mesh changes.
The older generic solver tests remain isolated references; that solver is no
longer selected by the live cape renderer.

Read-only offline validation also uses all 80 original cape variants across
240 rendering paths, with 120 root-motion updates per path. These use actual
mesh topology and skin weights but synthetic root transforms, not captured
in-game limb animation. Raw game assets are not redistributed. These checks
cannot substitute for visual/performance verification in the running client.

Dependency version/license/portability notes are in
`vendor/nvcloth/PROVENANCE.md`. NVIDIA's API documentation is at
https://nvidiagameworks.github.io/NvCloth/1.1/UserGuide/Index.html.
