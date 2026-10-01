# Real-time cape physics

Wardrobe → Physics → Cape opens a separate configuration window for NVIDIA
NvCloth CPU simulation for the current
player's world cape. The actual selected cape mesh is simulated; no rectangular
replacement or periodic sway is generated. Previews and other players are unaffected. The Bags and Weapons configuration
buttons are reserved and disabled. The cape menu offers independent bag and
weapon collision checkboxes (bags off by default, weapons on); these never
change bag or weapon animation. Body contact stays enabled. Cloth weight
(0.25–3×), stiffness (0–100%) and air resistance (0–100%) are character
preferences, independent of saved looks. Restore defaults changes only these
three tuning values. Older DLLs disable unsupported tuning controls. Disabling the option restores the ordinary native draw.

## Worker and rendering

`CapeWorker.cpp` owns one persistent simulation thread. Only copied positions,
triangle indices, pin indices, cached immutable collision meshes, copied bone
matrices, collision primitives and numeric settings cross
that boundary. The worker never reads native game memory, invokes game/Lua
functions or touches graphics buffers. Fabric cooking, NvCloth stepping,
skinning the collision meshes, contacts and fabric validation happen there.

The renderer submits through a try-lock and never waits for the solver. There
is one replaceable pending job and one latest result, not an unbounded frame
queue. Job time is accumulated so dropped intermediate snapshots do not slow
the simulation. Catch-up is limited to three 60 Hz steps. Mesh/reset/teleport
changes create a new generation; old jobs cannot publish into that generation.
Results more than 250 ms old expire. The normal cape remains visible while a
new simulation is preparing or recovering.

Completed particle coordinates are stored relative to the shoulder anchor and
rotated with the current player frame. Free vertices do not inherit the native
cape animation or get blended back toward it. Shoulder/collar vertices always
use the current native coordinates. Consequently an asynchronous result does
not visibly lag the whole garment behind a moving player. The current-frame
fabric check is also applied before submitting vertices. CPU/GPU draws use private
buffers; the original shared model, UVs, indices and materials are untouched.

Windows pins the module when first starting the worker, outside DllMain. The
worker and its service live until process exit; there is no thread join under
the loader lock or during a cape reset. Offline tests shut their worker down
normally. The shipped DLL statically links the CPU dependency, with no CUDA or
NVIDIA GPU requirement and no extra DLL to install.

## Simulation and collision

`CapeNvCloth.cpp` cooks topology and geodesic tethers once per mesh/reset.
Pinned particles have zero inverse mass. Free particle mass is derived from
triangle area and cloth density (0.35 kg/m² at 1×), with a small numerical mass
floor. The weight option affects response to air rather than multiplying
gravity. Vertical, horizontal and shear constraints are stiff; the stiffness
option controls bending. Solver frequency is 240 Hz, advanced in bounded 60 Hz
steps. Damping removes residual motion. Air resistance scales aerodynamic drag
from zero to 0.0008; lift is disabled. These are NvCloth coefficients, not
physical drag coefficients. Tethers resist stretching.

NvCloth's translating local frame keeps coordinates near the shoulder and
accounts for root acceleration. Velocity limiting acts on relative particle
motion, not the player's world speed. Only the seam follows the native pose;
there are no native-pose motion spheres on the free cape. At rest an unobstructed
cape settles downward under gravity. Surface contact can change that shape.

The renderer snapshots visible body/equipment geometry and current bone
matrices. The worker skins it, excludes the cape itself, culls distant triangles
and retains at most 512 nearby faces. Bags and weapon attachments are filtered
before loading their geometry according to the saved collision options. Each
free particle gets a finite local exclusion sphere derived from its nearest
actual mesh surface. Raw NvCloth triangle colliders extrapolate open meshes into
infinite planes; using finite contact neighborhoods avoids those launches.
Contact acquisition limits correction to 1 cm and ignores distant back faces,
so existing deep authored clipping is not forcibly ejected. This is a local
contact approximation, not an all-surface clearance guarantee.

Nearby native scenery triangles are cached for up to 100 ms and limited to
128 faces. NvCloth processes these on the worker. Snapshot acquisition,
body-bound posing, the native world query and drawing still run on the game
thread. Moving simulation off-thread does not make all cape work free.

`CapeFit.h` validates finite output, exact shoulder pins and a maximum edge
elongation of 25% plus 3 mm relative to the initial fabric; that is a catastrophic
failure guard, not the intended stretch. NvCloth's stiff distance constraints
and tethers enforce the actual fabric length. Folded and rotated triangles are
valid cloth. Invalid output triggers a temporary native fallback and retry,
not a partial blend that makes the cape look animated normally. Thin objects
can pass between the coarse original vertices; self-collision is disabled.

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
The contact count measures active mesh backstops; it is not an exact penetration
count. No native pointers or player identifiers are exported.

## Validation and limits

The mandatory check runs the real NvCloth CPU library, renderer mocks in both
synchronous and asynchronous configurations, GPU48/CPU32/CPU40 mapping,
gravity settling independent of native free-vertex animation, weight/air
response, collision option filtering, finite skinned-mesh contacts, pin/fabric
guards, world-floor collision, teleports and
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
