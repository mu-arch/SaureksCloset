"""Read canonical playable race models and baseline scales from this client's DBCs."""
import struct,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def table(name):
 b=(root/'research/client-data'/ (name+'.dbc')).read_bytes()
 magic,n,f,size,ss=struct.unpack_from('<4s4I',b);assert magic==b'WDBC'
 return {struct.unpack_from('<I',b,20+i*size)[0]:struct.unpack_from('<'+'I'*f,b,20+i*size) for i in range(n)},b[20+n*size:]
def fl(i):return struct.unpack('<f',struct.pack('<I',i))[0]
races,_=table('ChrRaces');displays,_=table('CreatureDisplayInfo');models,strings=table('CreatureModelData')
# ChrRaces 5875: MaleDisplayID/FemaleDisplayID columns 4/5.
result=[]
for race in range(1,9):
 for sex in (0,1):
  display=races[race][4+sex];d=displays[display];m=models[d[1]]
  filename=strings[m[2]:].split(b'\0')[0].decode('ascii')
  assert filename.lower().startswith('character\\') and m[1]&4 and not d[3]
  result.append((race,sex,display,fl(d[4])*fl(m[4]),filename))
(root/'native/RaceModels.h').write_text('// Generated from build 5875 ChrRaces, CreatureDisplayInfo and CreatureModelData.\n#pragma once\nstruct RaceModel { unsigned race,sex,display; float scale; const char* filename; };\nstatic constexpr RaceModel raceModels[] = {\n'+''.join('{%d,%d,%d,%.9ff,%s},\n'%(a,b,c,d,json.dumps(e)) for a,b,c,d,e in result)+'};\n')
print(result)
