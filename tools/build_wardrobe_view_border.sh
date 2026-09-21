#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
source_image="$repo_dir/assets/wardrobe-view-border-source.png"
texture_dir="$repo_dir/addon/SaureksCloset/Textures"
normalized_image="$(mktemp --suffix=.png)"
alpha_image="$(mktemp --suffix=.png)"
contact_image="$(mktemp --suffix=.png)"
ambient_image="$(mktemp --suffix=.png)"
combined_image="$(mktemp --suffix=.png)"
shadow_image="$(mktemp --suffix=.png)"
trap 'rm -f "$normalized_image" "$alpha_image" "$contact_image" "$ambient_image" "$combined_image" "$shadow_image"' EXIT

# Outfit's combined source is 512x512 while the supplied border is 512x566.
# Remove its extra 54 rows from the top, as requested, instead of compressing
# them; the remaining pixels then receive Outfit's exact in-game stretch.
magick "$source_image" -crop 512x512+0+54 +repage "PNG32:$normalized_image"

make_part() {
    local name="$1" crop="$2"
    magick "$normalized_image" -crop "$crop" +repage -type TrueColorAlpha \
        -define tga:compression=none "$texture_dir/WardrobeFrameCrop$name.tga"
}

make_part TL 256x256+0+0
make_part TR 256x256+256+0
make_part BL 256x256+0+256
make_part BR 256x256+256+256

# Reproduce Outfit's separate contact/ambient silhouette shadow: a tight
# 3x3 expansion plus .9px and 2.5px soft passes, combined with Screen.
magick "$normalized_image" -alpha extract "$alpha_image"
magick "$alpha_image" -morphology Dilate Square:1 -gaussian-blur 0x.9 \
    -evaluate multiply .85 "$contact_image"
magick "$alpha_image" -morphology Dilate Square:1 -gaussian-blur 0x2.5 \
    -evaluate multiply .5 "$ambient_image"
magick "$contact_image" "$ambient_image" -compose Screen -composite "$combined_image"
magick "$combined_image" -alpha copy -channel RGB -evaluate set 0 +channel "$shadow_image"

make_shadow_part() {
    local name="$1" crop="$2"
    magick "$shadow_image" -crop "$crop" +repage -type TrueColorAlpha \
        -define tga:compression=none "$texture_dir/WardrobeFrameShadowCrop$name.tga"
}

make_shadow_part TL 256x256+0+0
make_shadow_part TR 256x256+256+0
make_shadow_part BL 256x256+0+256
make_shadow_part BR 256x256+256+256
