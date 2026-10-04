"""Check the shipped cape-only bases and package allowlist, never generated caches."""
import hashlib,json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
entries=json.loads((root/'assets/cape/motion.json').read_text())
folder=root/'addon/SaureksCloset/CapeMotion'
assert len(entries)==16
assert {p.name for p in folder.iterdir() if p.is_file()}=={e['file'] for e in entries}|{'ASSETS-LICENSE'}
for entry in entries:
    data=(folder/entry['file']).read_bytes()
    assert hashlib.sha256(data).hexdigest()==entry['sha256']
    n,bo=struct.unpack_from('<II',data,52)
    first=entry['original_bones'];bones=entry['cape_bones']
    assert bones==list(range(first,n))
    assert 2<=len(bones)<=4
    # Private chains cannot be ancestors of any original body/tail/weapon bone.
    for bone in range(first):assert struct.unpack_from('<h',data,bo+108*bone+8)[0]<first
    for bone in bones:assert struct.unpack_from('<i',data,bo+108*bone)[0]==-1
    nv,vo=struct.unpack_from('<II',data,68)
    view_count,views=struct.unpack_from('<II',data,76)
    for view in range(view_count):
        at=views+44*view;ni,indices=struct.unpack_from('<II',data,at)
        ns,sections=struct.unpack_from('<II',data,at+24)
        for section in range(ns):
            geoset,_,start,count=struct.unpack_from('<4H',data,sections+32*section)
            if 1500<=geoset<1600:continue
            for k in range(start,start+count):
                vertex=struct.unpack_from('<H',data,indices+2*k)[0];assert vertex<nv
                for weight,bone in zip(data[vo+48*vertex+12:vo+48*vertex+16],data[vo+48*vertex+16:vo+48*vertex+20]):
                    assert not weight or bone<first, 'Private cape bones cannot deform body, tail, tabard or equipment'
assert (folder/'ASSETS-LICENSE').read_bytes()==(root/'ASSETS-LICENSE').read_bytes()
print('PASS: all 16 shipped cape bases match audited hashes and private bones affect only cape geosets')
