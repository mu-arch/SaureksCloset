-- Run with Lua 5.0.3, from VanityStudio.
local now,writes,messages=0,{},{}
GetTime=function() return now end
GetAddOnMetadata=function() return "test" end
GetInventoryItemLink=function(unit,slot) assert(unit=="player");return slot==16 and "item:123:0:0:0" end
VanityStudioDB={}
VanityStudio={index={[123]={123,"Sword",13,1,2,7}},Message=function(self,text) table.insert(messages,text) end}
WriteFile=function(path,mode,report) assert(path=="SaureksCloset-weaponry.txt" and mode=="w");table.insert(writes,report) end
-- Any accidental gameplay or model mutation fails immediately.
SetUnitVisibleItemID=function() error("capture must not change appearance") end
SaureksClosetSetAppearance=SetUnitVisibleItemID
local attachment=26
SaureksClosetWeaponryProbe=function(selector)
    if selector==-1 then return 1,100,1,0,49,49,0,0,0,0,0,0,0,0,0 end
    if selector==0 then return 1,200,100,attachment,4,1,2,0 end
    return 0
end
dofile("addon/SaureksCloset/WeaponryProbe.lua")
local V=VanityStudio
V:UpdateWeaponryCapture();assert(table.getn(writes)==0)
assert(V:StartWeaponryCapture());local lines=table.getn(V.weaponryCapture.lines)
now=.1;V:UpdateWeaponryCapture();assert(table.getn(V.weaponryCapture.lines)==lines)
attachment=1;now=.2;V:UpdateWeaponryCapture();assert(table.getn(V.weaponryCapture.lines)==lines+1)
now=30;V:UpdateWeaponryCapture();assert(not V.weaponryCapture and table.getn(writes)==1)
assert(string.find(writes[1],"equipped=16,123,13,2,7",1,true))
assert(string.find(writes[1],"TIME 0.20",1,true))
assert(VanityStudioDB.weaponryDiagnostics==writes[1])
assert(V:StartWeaponryCapture());V:StartWeaponryCapture();assert(not V.weaponryCapture)
SaureksClosetWeaponryProbe=nil;assert(V:StartWeaponryCapture()==false and not V.weaponryCapture)
SaureksClosetWeaponryProbe=function() error("unavailable") end
V:StartWeaponryCapture();assert(not V.weaponryCapture)
SaureksClosetWeaponryProbe=function() return -1 end
WriteFile=function() return false end
V:StartWeaponryCapture();V.weaponryCapture.bytes=131072;V.weaponryCapture.last=nil
now=30.2;V:UpdateWeaponryCapture();assert(not V.weaponryCapture)
assert(string.find(messages[table.getn(messages)],"SavedVariables",1,true))
print("Weapon capture: dormant state, transition deduplication, timeout, stop, missing DLL, errors, size cap and saved-variable fallback passed")
