# Animation controls

Wardrobe → Animations opens separate Bag Physics and Cape Animation sheets.
Each bag stores `physics` (default true) and `amplitude` (0–200%, default 100)
with its saved look. IDs remain stable when bags are moved, recolored or removed.
Default restores those original values; placement, scale and other bags are not
changed. The all-bags Default applies the same reset to the current look.
The placement tuner's temporary Pause checkbox has been removed.

The native fit bridge accepts an optional fourteenth argument, amplitude percent.
Old callers retain 100%. The amplitude changes secondary translation/rotation
and soft-bag deformation, not the animated body transform, motion frequency or
rigid geometry. Quaternion interpolation preserves rigid dimensions. Zero/off
clears dynamic history and follows the exact moving fitted contact. Soft output
amplitude is applied once when emitting control matrices, avoiding accumulation
on repeated draws. Existing deformation travel bounds still apply.

Cape: a per-character opt-in selects a separate baked HumanFemale model resource.
`tools/build_cape_animation.py --client GAME` reproduces it from build-5875 MPQs.
The original lower cape bones also influence tabards, so the variant duplicates
only bones 26 and 47 and rewires cape-only vertex/ GPU palette references to those
copies. Original bones, body/tabard weights, attachments and upper cape weights
are unchanged. Walk, Run and WalkBackwards lower-cape quaternion excursions are
reduced around the authored mean pose. Other animation tracks and all timing
remain unchanged. No cloth simulation or mesh edits run during gameplay.

`assets/cape/human-female-calm.json` records source/output hashes and edited key
ranges. `tests/test_cape_animation.py` validates CPU/GPU palette agreement,
attachment preservation and actual skinned full-length hem movement. Relative
to the moving mounting bone, lower-edge RMS movement is 60.6% of original for
walking/backwards and 47.3% for running. These are offline metrics, not an
in-game visual approval. Other races/genders retain their native model.
The optional local source argument additionally checks reproducibility and all
117 original bone records byte-for-byte.

Cape Default restores the original game model; missing assets fail closed.
The new settings are included in sharing protocol version 2, with a matching
relay parser. The fixed snapshot size is unchanged. No release version bump or
public relay deployment is part of these local animation changes.
