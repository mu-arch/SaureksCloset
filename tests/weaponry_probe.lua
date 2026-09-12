-- Run with Lua 5.0.3, from VanityStudio.
local now,writes,messages=0,{},{}
local equipment={[16]=123,[1]=456,[18]=2507,[20]=2101}
GetTime=function() return now end
GetAddOnMetadata=function() return "test" end
GetInventoryItemLink=function(unit,slot) assert(unit=="player");return equipment[slot] and "item:"..equipment[slot]..":0:0:0" end
VanityStudioDB={}
VanityStudioCharacter={enabled=true,weapons={},selected={}}
VanityStudio={index={[123]={123,"Sword",13,1,2,7}},Message=function(self,text) table.insert(messages,text) end}
WriteFile=function(path,mode,report) assert(path=="SaureksCloset-weaponry.txt" and mode=="w");table.insert(writes,report) end
-- Any accidental gameplay or model mutation fails immediately.
SetUnitVisibleItemID=function() error("capture must not change appearance") end
SaureksClosetSetAppearance=SetUnitVisibleItemID
SaureksClosetSetWeapons=SetUnitVisibleItemID
SaureksClosetBeginPreview=SetUnitVisibleItemID
VanityStudio.Refresh=SetUnitVisibleItemID
VanityStudio.RequestItem=SetUnitVisibleItemID
VanityStudio.BodyDraft=SetUnitVisibleItemID
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
-- Selections must be recorded even when the native child list does not change.
VanityStudioCharacter.weapons[107]=2101
now=.31;V:UpdateWeaponryCapture();assert(table.getn(V.weaponryCapture.lines)==lines+2)
assert(string.find(V:WeaponrySnapshot(),"saved-weapons=0,0,0,0,0,0,2101",1,true))
V.draft={slot=106,id=2507}
V.model={weaponToken=7,IsVisible=function() return true end,GetAlpha=function() return 0 end}
SaureksClosetInspectPreview=function(token) assert(token==7);return 1,1,2,0,0,0,0,0,0,8 end
SaureksClosetRealBody=function() return 2,0,1,2,3,4,5 end
local snapshot=V:WeaponrySnapshot()
assert(string.find(snapshot,"draft=106,2507",1,true))
assert(string.find(snapshot,"preview=model,7,true,true,true,0",1,true))
assert(string.find(snapshot,"preview-body=model,true,1,1,2,0,0,0,0,0,0,8",1,true))
assert(string.find(snapshot,"native-body=true,2,0,1,2,3,4,5",1,true))
-- All morph slots and weapon positions are emitted, including cleared/hidden
-- overrides, pending changes, and independent wardrobe/outfit requests.
V.slotNames={[1]="Head",[106]="Ranged back"}
VanityStudioCharacter.selected={[1]=0,[3]=789}
VanityStudioCharacter.managed={[1]=true}
V.applied={[1]=0};V.pending={[3]={id=789,attempts=2}};V.errors={[3]="Waiting"}
V.PreviewItems=function() return {[1]=456,[3]=789,[18]=2507} end
V.PreviewWeapons=function() return {[106]=2507,[107]=2101} end
V.PreviewWeaponRoutes=function(self,weapons) return weapons[106] and {[18]=106} or {} end
V.OutfitPreviewItems=function(self,look) assert(look==V.detailPending.look);return {[1]=999,[18]=2507} end
V.detailPending={phase="dress",look={slots={[1]=999},weapons={[106]=2507}}}
V.editingBody={race=2,sex=0,skin=3,face=4,hairStyle=5,hairColor=6,facial=7}
V.bodyControlValues={race=1,sex=1,skin=0,face=0,hairStyle=0,hairColor=0,facial=0}
V.previewWaiting={[789]=true,[123]=true}
snapshot=V:WeaponrySnapshot()
for _,slot in ipairs({1,3,15,4,5,19,9,10,6,7,8,16,17,18}) do
    assert(string.find(snapshot,"morph-slot="..slot..",",1,true))
end
for slot=101,107 do assert(string.find(snapshot,"weapon-slot="..slot..",",1,true)) end
for slot=0,23 do assert(string.find(snapshot,"equipped="..slot..",",1,true)) end
assert(string.find(snapshot,"morph-slot=1,Head,456,0,0,0,true,nil,nil,nil,456,nil,999,nil",1,true))
assert(string.find(snapshot,"morph-slot=3,nil,0,789,789,nil,nil,789,2,Waiting,789,nil,nil,nil",1,true))
assert(string.find(snapshot,"weapon-slot=106,Ranged back,nil,nil,2507,2507",1,true))
assert(string.find(snapshot,"body-editing=2,0,3,4,5,6,7",1,true))
assert(string.find(snapshot,"body-controls=1,1,0,0,0,0,0",1,true))
assert(string.find(snapshot,"wardrobe-missing-items=123,789",1,true))
assert(VanityStudioCharacter.weapons[106]==nil and V.applied[1]==0 and V.pending[3].attempts==2)
VanityStudioCharacter.enabled=false
assert(string.find(V:WeaponrySnapshot(),"morph-slot=1,Head,456,0,456,0,true",1,true))
assert(string.find(V:WeaponrySnapshot(),"weapon-slot=107,nil,2101,nil,2101,nil",1,true))
VanityStudioCharacter.enabled=true
SaureksClosetInspectPreview=function() error("preview expired") end
assert(string.find(V:WeaponrySnapshot(),"preview expired",1,true))
now=30;V:UpdateWeaponryCapture();assert(V.weaponryCapture and table.getn(writes)==0)
now=90;V:UpdateWeaponryCapture();assert(not V.weaponryCapture and table.getn(writes)==1)
assert(string.find(writes[1],"equipped=16,123,13,2,7",1,true))
assert(string.find(writes[1],"TIME 0.20",1,true))
assert(VanityStudioDB.weaponryDiagnostics==writes[1])
assert(V:StartWeaponryCapture());V:StartWeaponryCapture();assert(not V.weaponryCapture)
SaureksClosetWeaponryProbe=nil;assert(V:StartWeaponryCapture()==false and not V.weaponryCapture)
SaureksClosetWeaponryProbe=function() error("unavailable") end
V:StartWeaponryCapture();assert(not V.weaponryCapture)
SaureksClosetWeaponryProbe=function() return -1 end
WriteFile=function() return false end
V:StartWeaponryCapture();V.weaponryCapture.bytes=524288;V.weaponryCapture.last=nil
now=90.2;V:UpdateWeaponryCapture();assert(not V.weaponryCapture)
assert(string.find(messages[table.getn(messages)],"SavedVariables",1,true))
print("Weapon capture: dormant state, transition deduplication, timeout, stop, missing DLL, errors, size cap and saved-variable fallback passed")
print("All weapon positions, armor morph slots, ammo/bags, hidden and disabled overrides, pending items, preview routes and body editing/control values passed")
