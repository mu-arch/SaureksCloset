dofile('tests/haircraft.lua')
local V=VanityStudio
SaureksClosetSetHaircraft=function() return 1 end
SaureksClosetHairMaskVersion=function() return 3 end
local generation,status,changed,calls=0,1,12,{}
SaureksClosetSetHairMask=function(...) table.insert(calls,arg);return status,changed,generation end
local c=VanityStudioCharacter
c.enabled=true;c.keepHairWithHat=true;c.selected={[1]=501};c.body={race=1,sex=1,hairStyle=3}
c.hairMasks={['1:1:3:501']={enabled=true,height=0,pitch=80,roll=-80}}
V:SyncHairMask()
assert(calls[#calls][1]==1 and calls[#calls][2]==3 and #calls[#calls]==2)
assert(not V.haircraftMask and not V.hairMaskWindow and not V.CreateHairMaskWindow and not V.OpenHairMask and not V.haircraftDefault)
assert(V:HairMaskKey()=='1:1:3:501')
-- Automatic fitting follows current hat without requiring a saved config.
c.selected[1]=502;V:SyncHairMask();assert(calls[#calls][1]==1 and V:HairMaskKey()=='1:1:3:502')
c.body.hairStyle=4;V:SyncHairMask();assert(calls[#calls][1]==1 and V:HairMaskKey()=='1:1:4:502')
local invalidations=0;V.InvalidatePreviewModel=function() invalidations=invalidations+1 end
status=0;assert(V:SyncHairMask());assert(string.find(V.hairMaskStatus,'Waiting',1,true))
status=3;assert(V:SyncHairMask());assert(string.find(V.hairMaskStatus,'Loading',1,true))
status=4;assert(V:SyncHairMask());assert(string.find(V.hairMaskStatus,"Hat surface unavailable",1,true))
status=-6;assert(not V:SyncHairMask());assert(string.find(V.hairMaskStatus,'did not load',1,true))
status=1;generation=2;V:SyncHairMask();V:SyncHairMask();assert(invalidations==1)
changed=0;V:SyncHairMask();assert(string.find(V.hairMaskStatus,'preserved',1,true))
c.enabled=false;V:SyncHairMask();assert(calls[#calls][1]==0)
c.enabled=true;c.keepHairWithHat=false;V:SyncHairMask();assert(calls[#calls][1]==0)
c.keepHairWithHat=true;c.selected[1]=0;V:SyncHairMask();assert(calls[#calls][1]==0)
-- An old DLL must be actively told to discard its already-applied plane bake.
SaureksClosetHairMaskVersion=function() return 2 end
assert(not V:HairMaskAvailable() and not V:SyncHairMask());assert(calls[#calls][1]==0 and calls[#calls][2]==90)
local previous=#calls;V.sharingWorldPaused=true;assert(not V:SyncHairMask() and #calls==previous)
print('PASS: automatic fitting, no trim menu, ignored destructive presets, hat/style changes, pending loads, disable/hidden and old DLL cleanup')
