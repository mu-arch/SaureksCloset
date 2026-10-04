# Cape motion controls

`CapeMotion.h` adjusts authored lower-cape rotation tracks when Apply is pressed.
There is no per-frame cloth solver, collision simulation, race retargeting or
replacement character movement. Walking (4/13), running (5), standing (0), and
airborne/landing (37/38/39/40/187) amplitudes are independent, from 0–200%.
Advanced controls independently scale forward/back swing, side swing and twist,
set a -30 to +30 degree resting lean, soften direction reversals (0–100%), and
scale the bottom joint separately (0–200%). The Soft motion preset stages
55% forward/back, 85% side, 60% twist, 5 degrees outward lean, 65% smoothing,
and 60% hem motion; it preserves activity amounts until Apply.

Each clip rotates around its quaternion mean. 0% holds that clip's mean lower-cape
pose; 100% retains the authored animation. Native transitions and timestamps stay
unchanged. Directional changes operate on quaternion rotation vectors about each
clip mean; two-sided exponential filtering smooths reversals without retiming
character motion. Only the first movable cape joint receives resting lean. The upper cape bone, body/tail/tabard bones, scale and translation tracks
are never edited. Reset returns to the original model path.

The sixteen base models in `CapeMotion/` are the independently verified private-cape
bases from commit ae1a50f, recorded in `assets/cape/motion.json`. They contain each
race/sex's own animation only. No cross-race animation patches are installed.
Private bone indices remap only cape geosets; original body indices are unchanged.
The existing assets license applies, including the rights of third-party owners.

Customized model files are cached locally under `CapeMotion/Cache/` by body,
settings and content checksum. Only this strictly validated filename pattern can
use loose-file loading. Generated cache files are excluded from release packages.
Failed preparation leaves the previous applied setting intact. The engine receives
a separate model resource, never mutated shared animation data.

Checks: `tests/cape_motion.cpp` validates every body and slider category, unchanged
body/upper-cape records and timestamps, normalized rotations, malformed input and
asset-path isolation. `tests/cape_assets.py` checks source hashes and verifies that
no non-cape geoset uses the private bones. `tests/physics.lua` checks staging,
Apply/reset, failed-Apply rollback and panel exclusivity.
