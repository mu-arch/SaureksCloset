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

## Independent race animation styles

The Cape and Character animation sheets each have their own toggle, race/gender
selectors and Default. Choices are per-character, separate from appearance and
bag fits. Selecting a race enables that channel. Turning it off retains the
selection; Default clears it. The existing gentle human-female cape is an
alternative cape choice and never changes the character channel.

`tools/build_animation_styles.py --client GAME` builds sixteen recipient bases
and independent track patches for the other fifteen race/gender sources. Meshes,
textures, pivots, skin weights outside the cape, attachment records, scale tracks,
clip IDs and durations are retained. Actions absent in a donor keep native tracks.
Character transfers use semantic torso joints and the authored hand attachment
chains, rather than assuming equal arm key-bone IDs (Tauren/Troll differ). Local
joint translations and native finger/grip tracks remain intact. Root translation
is scaled to recipient height. Cape chain motion is distributed over the recipient
chain; private cape bones keep it independent of character/tabard tracks.

`AnimationStyles.h` validates and combines the selected patches once on selection
or first use of a recipient model. Playback uses the game's animation system.
A deterministic bounded LZ container reduces patch size; decompression and model
assembly never run per animation frame. Generated models live in Animations/Cache;
Default loads the original model. Cache names include a content checksum. Only
strictly formatted generated model paths are admitted by the loose-file resolver.
Missing/corrupt assets or unwritable cache fail back to the original model.

`animation-styles.json` records every shipped hash and bone correspondence.
`animation_styles.cpp` checks all 480 transfers, independent composition, unchanged
geometry/attachments/timing, malformed inputs and real cache generation.
`test_animation_styles.py` checks semantic hand/leg mappings and optionally
reproduces every base from the installed client's original MPQ models.
The tests do not establish in-game visual quality for every action and pairing;
different body proportions can still change foot contact or equipment clearance.
Race-specific appendages and fingers retain their recipient animations.
Sharing wire version 3 includes both independent style IDs (0 original, 1–16 donor).
