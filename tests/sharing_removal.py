"""Sharing must not be loadable from the addon or bundled renderer."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
addon = root / 'addon/SaureksCloset'
assert not (addon / 'Sharing.lua').exists()
assert 'Sharing.lua' not in (addon / 'SaureksCloset.toc').read_text()
retired = ('SaureksClosetConfigureSharing', 'SaureksClosetUpdateSharing',
           'SharingTransport', 'SharingProtocol', 'SharingRuntime',
           'sharedAppearances', 'sharedWeaponContexts', 'sharedFits',
           'sharedWeaponPhysics', 'SHARING_RELEASE_ENABLED', 'sharingWorldPaused',
           'CreateSharingSettings', 'InitializeSharing', 'UpdateSharing', 'StopSharingSession')
for folder, patterns in ((addon, ('*.lua',)), (root / 'native', ('*.h', '*.cpp'))):
    for pattern in patterns:
        for path in folder.glob(pattern):
            source = path.read_text()
            for symbol in retired:
                assert symbol not in source, (path, symbol)
for name in ('SharingTransport.h', 'SharingProtocol.h', 'SharingAppearance.h', 'SharingWorld.h', 'SharingRuntime.h'):
    assert not (root / 'native' / name).exists()
dll = (addon / 'Installation instructions/SaureksCloset.dll').read_bytes()
for symbol in ('SaureksClosetConfigureSharing', 'SaureksClosetUpdateSharing',
               'SaureksCloset-Sharing/1', 'WinHttpWebSocketCompleteUpgrade',
               'WinHttpWebSocketSend', 'WinHttpWebSocketReceive'):
    assert symbol.encode() not in dll and symbol.encode('utf-16le') not in dll, symbol
# The unrelated update checker and equipment UI protection remain available.
assert b'SaureksClosetStartUpdateCheck' in dll
assert 'equipmentUIVisibleItem(unit,slot,caller,localPlayer,visibleItemOriginal)' in (root / 'native/SaureksCloset.cpp').read_text()
print('PASS: sharing addon, transport, remote render branches and DLL entrypoints removed')
