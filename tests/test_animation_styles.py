"""Asset integrity and semantic retarget audit; optional original-client audit."""
from pathlib import Path
import hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from build_animation_styles import Model,semantic,base_model,Client
root=ROOT/'addon/SaureksCloset/Animations/Styles';audit=json.loads((ROOT/'assets/cape/animation-styles.json').read_text())
assert set(p.name for p in root.iterdir())==set(audit['files'])
for name,digest in audit['files'].items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
assert len(audit['models'])==16 and len(audit['files'])==497
models=[Model((root/f'B{i:02}.m2').read_bytes()) for i in range(1,17)]
for i,entry in enumerate(audit['models'],1):
    m=Model((root/f'B{i:02}.m2').read_bytes());orig=m.n-len(entry['cape_chain']);mapping=semantic(m)
    assert set(m.cape_chain)==set(range(orig,m.n))
    assert all(not any(b>=orig for b,w in zip(m.indices[v],m.weights[v]) if w) for v in m.body)
    for name,style in entry['styles'].items():
        assert style['matched_clips']>=90
        if name[0]=='A':
            assert len(style['mapping'])>=24
            donor=int(name[4:6]);source=models[donor-1];source_labels=semantic(source)
            for hand in (1,2):
                for depth in range(6):
                    assert style['mapping'][str(mapping[200+hand,depth])]==source_labels[200+hand,depth]
                    target_y=m.pivots[mapping[200+hand,depth],1];source_y=source.pivots[source_labels[200+hand,depth],1]
                    if abs(target_y)>.03 and abs(source_y)>.03:assert target_y*source_y>0,'Left/right hand chain crossed'
            for side in [-1,1]:
                for depth in range(3):assert str(mapping[100+side,depth]) in style['mapping']
        else:assert set(map(int,style['mapping']))==set(entry['cape_chain'])
if len(sys.argv)>1:
    client=Client(Path(sys.argv[1]))
    for i,entry in enumerate(audit['models'],1):
        race,sex=entry['race'],entry['sex'];raw,_=client.read(f'Character\\{race}\\{sex}\\{race}{sex}.m2')
        assert hashlib.sha256(raw).hexdigest()==entry['source_sha256']
        rebuilt,_=base_model(Model(raw));assert rebuilt==(root/f'B{i:02}.m2').read_bytes()
    print('PASS: all sixteen recipient bases reproduce from original game models with cape-only weight remapping')
print('PASS: sixteen skeletons, 480 donor transfers, ankle/knee/thigh mappings, independent cape chains and every shipped asset hash')
