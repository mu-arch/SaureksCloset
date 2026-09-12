"""Install Saurek's Closet, migrating the legacy folder/settings with rollback."""
import argparse,datetime,hashlib,json,shutil
from pathlib import Path
ADDON='SaureksCloset'
DLL='SaureksCloset.dll'
LEGACY_ADDON='VanityStudio'
NUMBERED_ADDON='saureks_closet.3.3.34'
LEGACY_ADDONS=(NUMBERED_ADDON,LEGACY_ADDON)
LEGACY_DLL='VanillaCloset.dll'  # Recognize and retire the old loader entry.
MISNAMED_DLL='saureks_closer.dll'  # Retire the previous misspelled release filename.
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def install(game,source,bridge,backup_root,receipt_path,expected_client_sha):
    game=game.resolve()
    assert (game/'WoW.exe').is_file(),'Expected a Windows WoW client folder'
    assert digest(game/'WoW.exe')==expected_client_sha,'Client executable does not match the verified build 5875'
    assert (game/'VanillaHelpers.dll').is_file(),'Existing VanillaHelpers.dll is required for world armor'
    assert source.is_dir() and bridge.is_file() and bridge.read_bytes()[:2]==b'MZ'
    addons=game/'Interface/AddOns';assert addons.is_dir() and not addons.is_symlink()
    target=addons/ADDON;legacy_folders=[addons/name for name in LEGACY_ADDONS];native=game/DLL;old_native=game/LEGACY_DLL;misnamed_native=game/MISNAMED_DLL;config=game/'dlls.txt'
    assert config.is_file(),'Expected the existing dlls.txt loader configuration'
    original=config.read_bytes();lines=original.splitlines(keepends=True)
    assert any(line.strip().lower()==b'vanillahelpers.dll' for line in lines),'Existing armor helper must be configured'
    newline=b'\r\n' if b'\r\n' in original else b'\n'
    own={name.lower().encode() for name in (DLL,LEGACY_DLL,MISNAMED_DLL)}
    updated=b''.join(line for line in lines if line.strip().lower() not in own)
    if updated and not updated.endswith((b'\r',b'\n')):updated+=newline
    updated+=DLL.encode()+newline
    migrations=[];queued=set()
    if (game/'WTF').is_dir():
        # Prefer the numbered release's settings over the older pre-rebrand copy.
        for name in LEGACY_ADDONS:
            for old in (game/'WTF').rglob(name+'.lua'):
                if old.parent.name=='SavedVariables':
                    new=old.with_name(ADDON+'.lua')
                    if not new.exists() and new not in queued:
                        migrations.append((old,new));queued.add(new)
    affected=[target,native,old_native,misnamed_native,config]+legacy_folders+[new for old,new in migrations]
    for path in affected+[old for old,new in migrations]:
        assert not path.is_symlink(),path
        assert not any(parent.is_symlink() for parent in path.parents if parent!=game),path
        if path.is_dir():assert not any(p.is_symlink() for p in path.rglob('*')),'Addon contains symlinks'
    backup=backup_root/datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f');backup.mkdir(parents=True)
    existed={p:p.exists() for p in affected}
    def backup_file(p):
        dest=backup/'game'/p.relative_to(game);dest.parent.mkdir(parents=True,exist_ok=True)
        if p.is_dir():shutil.copytree(p,dest)
        else:shutil.copy2(p,dest)
    for p in affected:
        if existed[p]:backup_file(p)
    for old,new in migrations:backup_file(old)
    try:
        # Keep user files in an existing installation; retire the legacy addon entirely.
        shutil.copytree(source,target,dirs_exist_ok=True)
        # Remove retired side-tab assets after the existing addon has been backed up.
        for name in ('TabBody.tga','TabOutfit.tga','SlotShadow.tga'):
            retired=target/'Textures'/name
            if retired.is_file():retired.unlink()
        shutil.copy2(bridge,native)
        for old,new in migrations:
            shutil.copy2(old,new);assert new.read_bytes()==old.read_bytes()
        config.write_bytes(updated)
        for legacy in legacy_folders:
            if legacy.exists():shutil.rmtree(legacy)
        if old_native.exists():old_native.unlink()
        if misnamed_native.exists():misnamed_native.unlink()
        files={}
        for f in source.rglob('*'):
            if f.is_file():
                rel=f.relative_to(source);assert (target/rel).read_bytes()==f.read_bytes()
                files['Interface/AddOns/'+ADDON+'/'+rel.as_posix()]=digest(f)
        bundled=target/'Installation instructions'/DLL
        bundled.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(bridge,bundled)
        assert digest(bundled)==digest(bridge)
        files['Interface/AddOns/'+ADDON+'/Installation instructions/'+DLL]=digest(bundled)
        assert digest(native)==digest(bridge) and config.read_bytes()==updated
        assert digest(game/'WoW.exe')==expected_client_sha
        files[DLL]=digest(native)
        receipt={'name':"Saurek's Closet",'version':'3.4.34','installed_to':str(target),'backup':str(backup),
                 'body_bridge':'Addon 3.4.34 requires renderer 30433. Replace both addon and DLL, then fully restart through VanillaFixes. Settings contains automatic update checks and loaded-DLL compatibility details.',
                 'loader_updated':updated!=original,'migrated_saved_variables':[str(new) for old,new in migrations],
                 'files_sha256':files}
        receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')
    except Exception:
        for p in affected:
            if p.is_dir():shutil.rmtree(p)
            elif p.exists():p.unlink()
            if existed[p]:
                saved=backup/'game'/p.relative_to(game);p.parent.mkdir(parents=True,exist_ok=True)
                if saved.is_dir():shutil.copytree(saved,p)
                else:shutil.copy2(saved,p)
        raise
    print(json.dumps(receipt,indent=2));return receipt
if __name__=='__main__':
    p=argparse.ArgumentParser(description="Install Saurek's Closet with WoW fully closed.")
    p.add_argument('game_directory',type=Path);a=p.parse_args()
    here=Path(__file__).resolve().parent
    if (here/'Interface/AddOns'/ADDON).is_dir():
        root=here;source=root/'Interface/AddOns'/ADDON;bridge=root/DLL;manifest=root/'CLIENT-BUILD.json'
    else:
        root=here.parent;source=root/'addon'/ADDON;bridge=root/'native'/DLL;manifest=root/'native/CLIENT-BUILD.json'
    install(a.game_directory,source,bridge,root/'backups',root/'INSTALL-RECEIPT.json',json.loads(manifest.read_text())['sha256'])
