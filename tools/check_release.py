"""Mandatory focused checks before building an installable release."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
lua_tests = [
    'haircraft', 'hat_tuner',
    'physics', 'sheet_close_alignment', 'body_renderer_version', 'body_arrow_loading', 'body_preview_equipment', 'preview_item_loading',
    'bags_list_ui', 'bag_instances', 'bag_tuner', 'wardrobe_save', 'preview_drag',
    'weapon_full_page', 'held_weapon_tuner', 'tuner_tooltips', 'tuner_controls', 'sharing', 'sharing_ui', 'updates', 'settings_donations', 'armor_recovery', 'bag_placement_editor',
]
for name in lua_tests:
    subprocess.run(['lua5.1', f'tests/{name}.lua'], cwd=root, check=True)
# Offline Blender experiments retain their individual tests, but are no longer
# installed assets or release prerequisites. Check the actual shipped rig.
for name in ['cape_assets', 'physics_removal', 'release_version', 'body_arrow_pixels', 'test_ui_assets', 'test_cloth_bag_shape', 'test_bag_deformation', 'test_bag_catalog']:
    subprocess.run([sys.executable, f'tests/{name}.py'], cwd=root, check=True)
with tempfile.TemporaryDirectory(prefix='closet-native-check-') as temporary:
    for name in ['hat_placement', 'haircraft', 'tabard_visibility', 'armor_inspection', 'equipment_ui', 'cape_motion', 'weapon_physics', 'bag_amplitude', 'sharing_protocol', 'sharing_appearance', 'sharing_weapon_renderer', 'weapon_renderer', 'website_links', 'bag_motion', 'bag_response', 'bag_jiggle', 'bag_size_motion', 'bag_model_mass', 'bag_phase_size',
                 'bag_body_binding', 'bag_hip_contact', 'bag_body_rocking', 'bag_running_bob', 'bag_rigid_body', 'bag_mount_flop', 'bag_tuning']:
        executable = str(Path(temporary)/name)
        subprocess.run(['c++', '-std=c++17', '-O0', f'tests/{name}.cpp', '-o', executable], cwd=root, check=True)
        subprocess.run([executable], cwd=root, check=True)
print('PASS: all mandatory release checks')
