"""Exercise the renamed addon's migration and rollback against a temporary client."""
from pathlib import Path
import importlib.util,tempfile,contextlib,io
spec=importlib.util.spec_from_file_location('installer',Path(__file__).resolve().parents[1]/'tools/install.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
root=Path(__file__).resolve().parents[1];source=root/'addon'/m.ADDON;bridge=root/'native'/m.DLL
with tempfile.TemporaryDirectory() as tmp:
    home=Path(tmp).resolve();game=home/'game';game.mkdir()
    for name in ['WoW.exe','VanillaHelpers.dll',m.LEGACY_DLL,m.MISNAMED_DLL,'Other.dll']:(game/name).write_bytes(name.encode())
    legacy=game/'Interface/AddOns'/m.LEGACY_ADDON;legacy.mkdir(parents=True);(legacy/'Core.lua').write_text('old addon')
    numbered=game/'Interface/AddOns'/m.NUMBERED_ADDON;numbered.mkdir();(numbered/'Core.lua').write_text('numbered addon')
    current=game/'Interface/AddOns'/m.ADDON/'Textures';current.mkdir(parents=True)
    (current/'ArmorShadowTL.tga').write_bytes(b'retired shadow')
    (current/'Main.tga').write_bytes(b'retired backdrop')
    (current/'PersonalTexture.tga').write_bytes(b'personal artwork')
    original=b'# personal config\r\nVanillaHelpers.dll\r\nVanillaCloset.dll\r\nOther.dll\r\n VanillaCloset.DLL \r\n'
    original+=m.MISNAMED_DLL.encode()+b'\r\n'
    (game/'dlls.txt').write_bytes(original)
    saved=[]
    for directory in ['WTF/Account/Test/SavedVariables','WTF/Account/Test/Realm/Character/SavedVariables']:
        p=game/directory/(m.NUMBERED_ADDON+'.lua');p.parent.mkdir(parents=True);p.write_text('newest looks '+directory);saved.append(p)
        p.with_name(m.LEGACY_ADDON+'.lua').write_text('outdated original looks')
    p=game/'WTF/Account/LegacyOnly/SavedVariables'/(m.LEGACY_ADDON+'.lua');p.parent.mkdir(parents=True);p.write_text('legacy-only looks');saved.append(p)
    def snapshot():return {str(p.relative_to(game)):p.read_bytes() for p in game.rglob('*') if p.is_file()}
    retired=game/'Interface/AddOns'/m.ADDON/'Textures';retired.mkdir(parents=True,exist_ok=True)
    (retired/'TabBody.tga').write_bytes(b'old side tab')
    (retired/'TabOutfit.tga').write_bytes(b'old side tab')
    initial=snapshot()
    # Fail after all mutations, including migration and retiring the old addon/DLL.
    bad_receipt=home/'directory';bad_receipt.mkdir()
    try:
        m.install(game,source,bridge,home/'backups',bad_receipt,m.digest(game/'WoW.exe'))
        raise AssertionError('Expected receipt failure')
    except IsADirectoryError:pass
    assert snapshot()==initial,'Rollback restores old addon, DLL, settings and loader after late failure'
    with contextlib.redirect_stdout(io.StringIO()):r=m.install(game,source,bridge,home/'backups',home/'receipt.json',m.digest(game/'WoW.exe'))
    target=game/'Interface/AddOns'/m.ADDON
    assert not numbered.exists() and not legacy.exists() and not (game/m.LEGACY_DLL).exists() and not (game/m.MISNAMED_DLL).exists()
    assert (target/(m.ADDON+'.toc')).is_file()
    assert not (retired/'TabBody.tga').exists() and not (retired/'TabOutfit.tga').exists()
    assert not (current/'ArmorShadowTL.tga').exists() and not (current/'Main.tga').exists()
    assert (current/'PersonalTexture.tga').read_bytes()==b'personal artwork'
    assert (game/m.DLL).read_bytes()==bridge.read_bytes()
    assert (game/'dlls.txt').read_bytes()==b'# personal config\r\nVanillaHelpers.dll\r\nOther.dll\r\nSaureksCloset.dll\r\n'
    for old in saved:
        new=old.with_name(m.ADDON+'.lua');assert new.read_bytes()==old.read_bytes()
        assert old.read_bytes()==initial[str(old.relative_to(game))]
        new.write_text('newer settings')
    assert len(r['migrated_saved_variables'])==3
    assert (Path(r['backup'])/'game/Interface/AddOns/VanityStudio/Core.lua').read_text()=='old addon'
    assert (game/'Other.dll').read_bytes()==b'Other.dll'
    with contextlib.redirect_stdout(io.StringIO()):r=m.install(game,source,bridge,home/'backups',home/'receipt.json',m.digest(game/'WoW.exe'))
    assert not r['loader_updated'] and not r['migrated_saved_variables']
    for old in saved:assert old.with_name(m.ADDON+'.lua').read_text()=='newer settings'
    before=snapshot()
    try:
        m.install(game,source,bridge,home/'backups',home/'receipt.json','wrong hash')
        raise RuntimeError('Wrong client accepted')
    except AssertionError as e:assert 'does not match' in str(e)
    assert snapshot()==before
print('PASS: folder/DLL rename, loader deduplication, account + character migration, preservation, idempotence, wrong-client rejection and complete late-failure rollback')
