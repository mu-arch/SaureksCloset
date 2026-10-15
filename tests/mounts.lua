-- Real mount picker and look lifecycle, with a VanillaHelpers-like mount field
-- override simulator. The helper retains server values beneath its override.
dofile('tests/wardrobe_save.lua')
GetInventoryItemLink=function() return nil end
local V=VanityStudio
local helpers={}
for i=1,80 do local name,value=debug.getupvalue(V.CreateUI,i);if not name then break end;helpers[name]=value end
local function noop() end
local tooltipName,auras='',{}
local buffQueries=0
GetPlayerBuff=function(i,filter) assert(filter=='HELPFUL');buffQueries=buffQueries+1;return auras[i+1] and i or -1 end
GetPlayerBuffTexture=function(i) return auras[i+1].icon end
local serverMount,shownMount,override=0,0,nil
local calls,fail=0,false
SetUnitMountDisplayID=function(unit,id)
    assert(unit=='player');calls=calls+1
    if fail then error('temporarily unavailable') end
    override=id;shownMount=override or serverMount
end
UnitDisplayInfo=function(unit) assert(unit=='player');return 49,49,shownMount end
local taxi=false
UnitOnTaxi=function() return taxi end
local originalCreate=CreateFrame
function CreateFrame(kind,name,parent,template)
    local f=originalCreate(kind,name,parent,template)
    if name=='SaureksClosetMountAuraTip' then
        f.SetOwner=noop
        function f:ClearLines() tooltipName='' end
        function f:SetPlayerBuff(i) tooltipName=auras[i+1].name end
        _G[name..'TextLeft1']={GetText=function() return tooltipName end}
    end
    function f:HasFocus() return self.focused end
    function f:SetNormalTexture(value) self.normalTexture=value end
    function f:SetPushedTexture(value) self.pushedTexture=value end
    function f:SetDisabledTexture(value) self.disabledTexture=value end
    function f:GetHeight() return self.height or 16 end
    return f
end
dofile('addon/SaureksCloset/MountCatalog.lua')
dofile('addon/SaureksCloset/Mounts.lua')
local function run(frame,event)
    local old=this;this=frame;frame.scripts[event]();this=old
end
local function server(display)
    serverMount=display;shownMount=override or display
end
local function aura(name,icon)
    auras=name and {{name=name,icon=icon or 'Interface\\Icons\\Ability_Mount_RidingHorse'}} or {}
end
VanityStudioDB={outfits={}}
VanityStudioCharacter={enabled=true,selected={},weapons={}}
V:InitializeLookSettings();V.ready=true;V:InitializeMounts()
local page=CreateFrame('Frame',nil,V.frame)
V:CreateMountsPage(page,helpers.section,helpers.label,helpers.edit,helpers.button,helpers.enabled,helpers.arrow)
assert(#VanityStudioMountCatalog==109 and #V.mountButtons==24)
assert(V.mountOriginal.closetPanel and V.mountCurrent.text=='Original mount' and not V.mountOriginal.closetEnabled)
assert(calls==0,'Opening mount selection cannot create a mounted model')
assert(V.mountPageLabel.text=='1 / 5')
local byID=V:MountCatalogIndex()
assert(byID[2404].name=='Brown Horse' and byID[2404].icon=='Interface\\Icons\\Ability_Mount_RidingHorse')
local red=V:FilterMounts('  RED  ');assert(#red>0)
for _,entry in ipairs(red) do assert(string.find(string.lower(entry.name),'red',1,true)) end
V.mountSearch:SetText('not-a-real-mount');V:RefreshMountsPage()
assert(V.mountEmpty.text~='' and not V.mountNext.closetEnabled and not V.mountPrevious.closetEnabled)
for _,b in ipairs(V.mountButtons) do assert(not b:IsShown()) end
V.mountSearch:SetText('');V:RefreshMountsPage()
run(V.mountNext,'OnClick');assert(V.mountPage==2 and V.mountPrevious.closetEnabled)
V.mountPage=999;V:RefreshMountsPage();assert(V.mountPage==5 and not V.mountNext.closetEnabled)
V.mountPage=1;V:RefreshMountsPage()
local first=V.mountButtons[1]
run(first,'OnClick')
assert(VanityStudioCharacter.mount==first.mount.id and V:HasUnsavedLookChanges())
assert(calls==0 and shownMount==0,'Selecting a mount while on foot must not summon a cosmetic mount')
V:RefreshMountsPage();assert(first.selected:IsShown() and V.mountOriginal.closetEnabled)
assert(V:SaveOutfit('Wolf',false))
assert(VanityStudioDB.outfits.Wolf.settings.mount==first.mount.id)
-- Mount activation changes only the local mount display and is idempotent.
server(2404);aura('Brown Horse');V:SyncMount()
assert(override==first.mount.id and VanityStudioCharacter.mountManaged)
local prior=calls;for i=1,20 do V:SyncMount() end;assert(calls==prior)
-- Mount appearance can change in place, and different looks retain their choice.
assert(V:SetMountAppearance(2404));assert(override==2404 and V:SaveOutfit('Horse',false))
assert(V:LoadOutfit('Wolf',true));assert(override==first.mount.id)
assert(V:LoadOutfit('Horse',true));assert(override==2404)
-- Unknown selection is rejected without mutating the look or helper state.
assert(not V:SetMountAppearance(999999) and VanityStudioCharacter.mount==2404)
-- Dismount updates arrive beneath the helper override; remove our override,
-- never write an explicit zero, so the helper's real mount state is restored.
server(0);aura(nil);V:SyncMount()
assert(not override and shownMount==0 and not VanityStudioCharacter.mountManaged)
server(1166);aura('Arctic Wolf','Interface\\Icons\\Ability_Mount_WhiteDireWolf');V:SyncMount();assert(override==2404)
-- Taxi transitions must release the override and restore it when the taxi ends.
taxi=true;V:SyncMount();assert(not override and shownMount==1166)
taxi=false;V:SyncMount();assert(override==2404)
-- Addon disable/enable and original selection restore the server value.
VanityStudioCharacter.enabled=false;V:SyncMount();assert(not override and shownMount==1166)
VanityStudioCharacter.enabled=true;V:SyncMount();assert(override==2404)
assert(V:SetMountAppearance(nil));assert(not override and shownMount==1166)
-- Shared nonmount icons cannot create a mount, even while a display is present.
server(0);aura('Power Word: Shield','Interface\\Icons\\Spell_Holy_PowerWordShield')
assert(V:SetMountAppearance(2404));assert(not override)
-- Shared-icon event mounts use exact names; tooltip reads do not modify GameTooltip.
aura('Reindeer','Interface\\Icons\\INV_Misc_QuestionMark');server(2404);V:SyncMount();assert(override==2404)
aura('Fake Reindeer Speed','Interface\\Icons\\INV_Misc_QuestionMark');V:SyncMount();assert(not override)
-- Missing helper produces a useful disabled picker and retains saved selections.
local helper=SetUnitMountDisplayID;SetUnitMountDisplayID=nil
V:RefreshMountsPage();assert(not V.mountButtons[1].closetEnabled and V.mountStatus.text~='')
assert(not V:SetMountAppearance(1166) and VanityStudioCharacter.mount==2404)
SetUnitMountDisplayID=helper
-- A failed apply retries, without losing the chosen look.
aura('Brown Horse');server(2404);fail=true;V:SyncMount();assert(V.mountError and not override)
fail=false;V:SyncMount();assert(not V.mountError and override==2404)
-- Reload clears a previously managed mount if the new look uses Original.
VanityStudioCharacter.mount=nil;V.mountApplied=nil;V:SyncMount();assert(not override)
-- Re-entering/zoning respects the saved look without affecting other units.
assert(V:SetMountAppearance(1166));event='PLAYER_LEAVING_WORLD';run(V.mountEvents,'OnEvent')
assert(V.mountSuspended and not override)
event='PLAYER_ENTERING_WORLD';run(V.mountEvents,'OnEvent');assert(not V.mountSuspended and override==1166)
-- Blank looks clear mounts; stored inactive looks keep theirs.
V:ClearAll();assert(not override and not VanityStudioCharacter.mount)
assert(VanityStudioDB.outfits.Horse.settings.mount==2404)
print('PASS: mount catalog, icon picker/search/paging, look saves, mounted-only overrides, dismount/taxi/disable/reload/zoning, idempotence, errors and helper gating')
