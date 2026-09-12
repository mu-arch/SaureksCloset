"""Find candidate direct x86 CALL references in the pinned executable; inspect disassembly to confirm boundaries."""
from pathlib import Path
import os
import struct,sys
p=Path(os.environ['WOW_EXE']);b=p.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0]
n=struct.unpack_from('<H',b,pe+6)[0];opt=pe+24;sz=struct.unpack_from('<H',b,pe+20)[0];base=struct.unpack_from('<I',b,opt+28)[0]
sections=[struct.unpack_from('<8sIIIIIIHHI',b,opt+sz+40*i) for i in range(n)]
targets={int(s,16):[] for s in sys.argv[1:]}
for name,vsz,vaddr,rsz,ptr,*rest in sections:
 if not rest[-1]&0x20000000:continue
 data=b[ptr:ptr+rsz]
 for i in range(len(data)-5):
  if data[i]!=0xe8:continue
  dest=(base+vaddr+i+5+struct.unpack_from('<i',data,i+1)[0])&0xffffffff
  if dest in targets:targets[dest].append(hex(base+vaddr+i))
for target,refs in targets.items():print(hex(target),','.join(refs))
