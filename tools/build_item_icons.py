"""Resolve catalog display IDs to native icon paths without requiring server item-cache entries."""
import struct,re,json,ctypes as C
import read_client_data as client
from pathlib import Path
root=Path(__file__).resolve().parents[1]
b=(root/'research/client-data/ItemDisplayInfo.dbc').read_bytes()
magic,n,fields,size,ss=struct.unpack_from('<4s4I',b);assert magic==b'WDBC' and fields==23 and size==92
strings=b[20+n*size:];icons={}
for i in range(n):
 row=struct.unpack_from('<23I',b,20+i*size)
 icon=strings[row[5]:].split(b'\0')[0].decode('ascii')
 if icon:icons[row[0]]=icon
catalog=(root/'addon/SaureksCloset/Catalog.lua').read_text()
displays=set()
for line in catalog.splitlines():
 if not line.startswith('{'):continue
 item=json.loads('['+line[1:].rstrip(',}')+']');displays.add(item[7])
available=set()
for archive in ['interface.MPQ','patch.MPQ','patch-2.MPQ']:
 h=C.c_void_p();assert client.lib.SFileOpenArchive(str(client.game/archive).encode(),0,0x100,C.byref(h))
 client.archives=[h]
 available.update(line.lower() for line in (client.read('(listfile)') or b'').decode(errors='replace').splitlines())
 client.lib.SFileCloseArchive(h)
client.archives=[]
resolved={}
for d in displays:
 name=re.sub(r"\.(?:tga|blp)$","",icons.get(d,""),flags=re.I)
 if ('Interface\\Icons\\'+name+'.blp').lower() in available:resolved[d]=name
out=root/'addon/SaureksCloset/ItemIcons.lua'
out.write_text('-- Native ItemDisplayInfo icon names, keyed by display ID. No item-cache query needed.\nVanityStudioItemIcons = {\n'+''.join('['+str(d)+']='+json.dumps(name)+',\n' for d,name in sorted(resolved.items()))+'}\n')
print('Resolved',len(resolved),'of',len(displays),'catalog display IDs; missing:',sorted(displays-set(resolved)))
