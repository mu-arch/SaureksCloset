from pathlib import Path
import os
import struct,hashlib,json
r=Path(__file__).resolve().parents[1]
p=Path(os.environ['WOW_EXE']);b=p.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0]
n=struct.unpack_from('<H',b,pe+6)[0];opt=pe+24;sz=struct.unpack_from('<H',b,pe+20)[0];base=struct.unpack_from('<I',b,opt+28)[0]
sections=[struct.unpack_from('<8sIIIIIIHHI',b,opt+sz+40*i) for i in range(n)]
addresses=[0x712cb0,0x47a0c0,0x47a070,0x60b590,0x60b770,0x712f00,0x7130a0,0x712f70,0x713020,0x4798c0,0x710390,0x605da0,0x611e10,0x704120,0x468550,0x468460,0x6f3810,0x6f34d0,0x6f3620,0x600320,0x476b90,0x60ae10,0x7106c0,0x710620,0x607bc3,0x60abe0,0x5fb200,0x707350,0x707400,0x476cb0,0x70e170,0x5059d5,0x5043d5,0x7103a0]
def at(va):
 rva=va-base
 for name,vsz,vaddr,rsz,ptr,*rest in sections:
  if vaddr<=rva<vaddr+rsz:return b[ptr+rva-vaddr:ptr+rva-vaddr+12]
 raise ValueError(hex(va))
(r/'native/BuildSignatures.h').write_text('// Verified executable prefixes. Incompatible clients leave the bridge inactive.\n#pragma once\nstruct BuildSignature { unsigned address; unsigned char bytes[12]; };\nstatic const BuildSignature signatures[] = {\n'+''.join('{0x%x,{%s}},\n'%(a,','.join('0x%02x'%x for x in at(a))) for a in addresses)+'};\n')
(r/'native/CLIENT-BUILD.json').write_text(json.dumps({'build':5875,'sha256':hashlib.sha256(b).hexdigest(),'image_base':base,'functions':[hex(a) for a in addresses]},indent=2))
