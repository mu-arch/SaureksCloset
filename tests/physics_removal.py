"""Retired race/cape features must not return in UI, renderer or package."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
addon=root/'addon/SaureksCloset'
assert 'Physics.lua' in (addon/'SaureksCloset.toc').read_text()
assert 'Animations.lua' not in (addon/'SaureksCloset.toc').read_text()
assert not (addon/'Animations.lua').exists()
assert not list((addon/'Animations').rglob('*.*'))
ui=(addon/'UI.lua').read_text();assert 'choice("Physics","physics")' in ui and '"animations"' not in ui
native=(root/'native/SaureksCloset.cpp').read_text()
for retired in ['SetAnimationStyles','SetCapeAnimation','localAnimationModel','styledAnimationModel','AnimationVersion']:assert retired not in native
assert 'SaureksClosetSetWeaponPhysics' in native and 'SaureksClosetPhysicsVersion' in native
resolver=(root/'native/BagAssetFiles.h').read_text();assert 'capeAnimationAsset' not in resolver and 'AnimationStyles' not in resolver
package=(root/'tools/package.py').read_text();assert 'animation_styles' not in package and 'human-female-calm' not in package
print('PASS: native cape/race playback restored, retired assets excluded, Physics navigation and new weapon bridge')
