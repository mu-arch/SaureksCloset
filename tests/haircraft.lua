dofile('tests/bags_list_ui.lua')
local V=VanityStudio
dofile('addon/SaureksCloset/Haircraft.lua')
local helpers={}
for i=1,80 do local name,value=debug.getupvalue(V.CreateUI,i);if not name then break end;helpers[name]=value end
local calls={};local result=1
SaureksClosetSetHaircraft=function(value) table.insert(calls,value);return result end
local c=VanityStudioCharacter;c.enabled=true;c.keepHairWithHat=nil
local invalidated=0;V.InvalidatePreviewModel=function() invalidated=invalidated+1 end
V.pagesByName.haircraft=CreateFrame('Frame',nil,V.frame)
V:CreateHaircraftPage(V.pagesByName.haircraft,helpers.label,helpers.button,helpers.enabled)
assert(#calls==0 and V.haircraftToggle:GetText()=='Keep hair: Off')
V:SetTab('haircraft')
assert(V.tab=='haircraft' and V.wardrobePage=='haircraft')
assert(V.pagesByName.armor:IsShown() and V.pagesByName.haircraft:IsShown())
assert(not V.pagesByName.bags:IsShown() and not V.pagesByName.body:IsShown())
assert(V.rotationControls:IsShown() and V.wardrobeSelectorLabel:GetText()=='Haircraft')
local baseline=invalidated
V.haircraftToggle.scripts.OnClick()
assert(c.keepHairWithHat and calls[#calls]==1 and invalidated==baseline+1)
assert(V.haircraftToggle:GetText()=='Keep hair: On')
for i=1,4 do V:RefreshHaircraftPage() end
assert(#calls==1,'Refreshing the page must not change appearance')
c.enabled=false;assert(V:SyncHaircraft() and calls[#calls]==0 and c.keepHairWithHat)
c.enabled=true;assert(V:SyncHaircraft() and calls[#calls]==1)
V.sharingWorldPaused=true;local count=#calls;assert(not V:SyncHaircraft() and #calls==count);V.sharingWorldPaused=nil
result=-1;assert(not V:SetHaircraft(false) and c.keepHairWithHat and invalidated==baseline+1)
result=1;V.haircraftDefault.scripts.OnClick()
assert(not c.keepHairWithHat and calls[#calls]==0 and invalidated==baseline+2)
SaureksClosetSetHaircraft=nil;V:RefreshHaircraftPage()
assert(not V.haircraftToggle.enabled and not V.haircraftDefault.enabled)
assert(not V:SetHaircraft(true) and not c.keepHairWithHat)
V:SetTab('bags');assert(not V.pagesByName.haircraft:IsShown() and not V.haircraftToggle:IsVisible())
print('PASS: Haircraft navigation, preview, toggles, defaults, persistence, disable, failed apply and old DLL gating')
