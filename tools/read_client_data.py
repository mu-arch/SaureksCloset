import ctypes as C
from pathlib import Path
import os
root=Path(__file__).resolve().parents[1]
lib=C.CDLL(str(root/'research/storm-build/storm.framework/storm'))
lib.SFileOpenArchive.argtypes=[C.c_char_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_void_p)]
lib.SFileOpenFileEx.argtypes=[C.c_void_p,C.c_char_p,C.c_uint32,C.POINTER(C.c_void_p)]
lib.SFileGetFileSize.argtypes=[C.c_void_p,C.POINTER(C.c_uint32)];lib.SFileGetFileSize.restype=C.c_uint32
lib.SFileReadFile.argtypes=[C.c_void_p,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32),C.c_void_p]
lib.SFileCloseFile.argtypes=[C.c_void_p];lib.SFileCloseArchive.argtypes=[C.c_void_p]
game=Path(os.environ['WOW_DATA'])
archives=[]
def read(name):
 for h in archives:
  f=C.c_void_p()
  if lib.SFileOpenFileEx(h,name.encode(),0,C.byref(f)):
   size=lib.SFileGetFileSize(f,None);buf=C.create_string_buffer(size);n=C.c_uint32()
   assert lib.SFileReadFile(f,buf,size,C.byref(n),None)
   lib.SFileCloseFile(f);return buf.raw
 return None
if __name__=='__main__':
 for name in ['patch-2.MPQ','patch.MPQ','dbc.MPQ','interface.MPQ']:
  h=C.c_void_p();assert lib.SFileOpenArchive(str(game/name).encode(),0,0x100,C.byref(h)),name
  archives.append(h)
 for name in ['DBFilesClient\\CharSections.dbc','DBFilesClient\\CharacterFacialHairStyles.dbc','DBFilesClient\\CharHairGeosets.dbc','DBFilesClient\\ChrRaces.dbc','Interface\\FrameXML\\DressUpFrame.xml','Interface\\FrameXML\\DressUpFrames.lua','Interface\\FrameXML\\DressUpFrames.xml','Interface\\FrameXML\\UIPanelTemplates.xml']:
  data=read(name)
  if data:
   (root/'research/client-data'/name.split('\\')[-1]).write_bytes(data);print(name,len(data))
  else: print('MISSING',name)
 for h in archives:lib.SFileCloseArchive(h)
