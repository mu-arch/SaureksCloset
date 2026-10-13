-- Real look lifecycle, placement/physics controllers and renderer dispatch.
-- Only game/UI APIs are mocked; snapshots and migration run unchanged.
local noop=function() end
GetInventoryItemLink=function() return nil end
SlashCmdList={}
CreateFrame=function() return {RegisterEvent=noop,SetScript=noop} end
function time() return 10 end
function date() return 'test' end
SaureksClosetWeaponAssets={};SaureksClosetQuivers={}
dofile('addon/SaureksCloset/Core.lua')
dofile('addon/SaureksCloset/BagCatalog.lua')
dofile('addon/SaureksCloset/Weaponry.lua')
dofile('addon/SaureksCloset/Bags.lua')
dofile('addon/SaureksCloset/BagTuner.lua')
dofile('addon/SaureksCloset/Physics.lua')
dofile('addon/SaureksCloset/CapePhysics.lua')
local V=VanityStudio
VanityStudioRaces={};for race=1,8 do VanityStudioRaces[race]={'Race '..race} end
local native,slots,weapon,cape={},{},nil,nil
SaureksClosetRendererVersion=function() return 40101 end
SaureksClosetGetBagFitDefaults=function(target) return 1,0,0,0,0,0,0,target>=201 and 85 or 100 end
SaureksClosetSetBagFit=function(target,race,sex,on,left,inset,up,pitch,roll,yaw,scale)
    native[target..':'..race..':'..sex]=on==1 and {left=left,up=up,scale=scale} or nil;return 1
end
SaureksClosetPhysicsVersion=function() return 4 end
SaureksClosetSetWeaponPhysics=function(...) weapon={...};return 1 end
SaureksClosetSetWeaponSlotPhysics=function(slot,...) slots[slot]={...};return 1 end
SaureksClosetSetCapeMotion=function(...) cape={...};return 1 end
for _,name in ipairs({'Refresh','CancelDraft','InvalidatePreviewModel','Sync','SyncLiveBagFits','SyncWeapons'}) do V[name]=noop end
V.NativeBody=function() return {race=1,sex=1} end
V.NormalizeBody=function(self,b) return self:Copy(b) end
V.SyncBody=function() return true end
V.BodyAvailable=function() return true end
V.HatTuningAvailable=function() return true end
V.WeaponRendererAvailable=function() return true end
V.BagRendererAvailable=function() return true end
V.MultiBagRendererAvailable=function() return true end
V.weaponPhysicsStatus={SetText=noop};V.capePhysicsStatus={SetText=noop};V.capeAdvancedStatus={SetText=noop}
local function same(a,b)
    if type(a)~=type(b) then return false end
    if type(a)~='table' then return a==b end
    for k,v in pairs(a) do if not same(v,b[k]) then return false end end
    for k in pairs(b) do if a[k]==nil then return false end end
    return true
end
local legacyFit={schema=1,bag=110,race=1,sex=1,values={left=.12,inset=0,up=.15,pitch=0,roll=0,yaw=0,scale=100}}
VanityStudioDB={bagTuning={schema=1,enabled=true,fits={['110:1:1']=legacyFit}},outfits={
    A={version=3,slots={},weapons={bags={{id=1,slot=1,model=12,mount='back',fits={}}}}},
    B={version=3,slots={},weapons={bags={{id=1,slot=1,model=12,mount='back',fits={}}}}}
}}
VanityStudioCharacter={selected={},weapons={},enabled=true,weaponPhysics=true,weaponBounce=77,capeMotion={enabled=true,run=55},activeOutfit='A'}
V:InitializeLookSettings();V:InitializeBagTuning()
local a,b=VanityStudioDB.outfits.A,VanityStudioDB.outfits.B
assert(a.version==4 and a.settings.weaponBounce==77 and b.settings.capeMotion.run==55)
assert(a.settings~=b.settings and a.settings.placements.fits['110:1:1']~=legacyFit)
assert(V:LoadOutfit('A',true))
assert(native['110:1:1'].up==.15 and weapon[1]==1 and weapon[2]==77 and cape[3]==55)
assert(not V:HasUnsavedLookChanges())
V.placementTunerSlot=110
assert(not V:CanRevertLookItemPlacement(V:GetBagTunerState()))
assert(V:SetBagTunerValue('up',.4))
assert(V:CanRevertLookItemPlacement(V:GetBagTunerState()))
assert(V:HasUnsavedLookChanges() and V:ActiveSavedLookName()=='A')
assert(a.settings.placements.fits['110:1:1'].values.up==.15 and b.settings.placements.fits['110:1:1'].values.up==.15)
assert(VanityStudioCharacter.unsaved.settings.placements.fits['110:1:1'].values.up==.4)
assert(V:SetWeaponPhysicsAmount('weaponBounce',120))
V.weaponPhysicsSlot=108;assert(V:SetWeaponPhysicsCustom(true));assert(V:SetWeaponPhysicsAmount('weaponRocking',180))
V.capePhysicsDraft=V:CapePhysicsConfig();V.capePhysicsDraft.run=20;V.capePhysicsDraft.side=40
assert(V:HasCapePhysicsDraft() and V:ApplyCapePhysics())
V.placementTunerSlot=111;assert(V:SetBagTunerValue('up',.2))
V.placementTunerSlot=nil;V.placementTunerBag=1
assert(V:SetBagTunerValue('pitch',25));assert(V:SetBagPhysics(1,false,35))
-- Saving captures every draft, not only the item currently open in the tuner.
assert(V:SaveOutfit('A',true));a=VanityStudioDB.outfits.A
assert(not V:HasUnsavedLookChanges())
assert(a.settings.placements.fits['110:1:1'].values.up==.4)
assert(a.settings.placements.fits['111:1:1'].values.up==.2)
assert(a.settings.weaponBounce==120 and a.settings.weaponPhysicsSlots[108].weaponRocking==180)
assert(a.settings.capeMotion.run==20 and a.settings.capeMotion.side==40)
assert(a.weapons.bags[1].fits['1:1'].pitch==25 and a.weapons.bags[1].amplitude==35 and not a.weapons.bags[1].physics)
assert(same(legacyFit,VanityStudioDB.bagTuning.fits['110:1:1']))
-- The main Save action also applies an uncommitted cape-panel draft.
V.capePhysicsDraft=V:CapePhysicsConfig();V.capePhysicsDraft.tip=75
assert(V:HasUnsavedLookChanges() and V:SaveOutfit('A',true))
a=VanityStudioDB.outfits.A;assert(a.settings.capeMotion.tip==75 and not V:HasUnsavedLookChanges())
local savedA=V:Copy(a)
assert(V:LoadOutfit('B',true))
assert(native['110:1:1'].up==.15 and native['111:1:1']==nil)
assert(weapon[2]==77 and slots[108][1]==0 and cape[3]==55)
assert(V:BagInstance(1).amplitude==100 and V:BagInstance(1).physics)
-- Defaults and per-slot custom resets only edit the current look.
V.weaponPhysicsSlot=0;assert(V:ResetWeaponPhysics())
V.capePhysicsDraft=V:CapePhysicsConfig();assert(V:ApplyCapePhysics(true))
V.placementTunerSlot=110;assert(V:ResetBagTunerFit());assert(V:SetBagTunerEnabled(false))
assert(V:SaveOutfit('B',true));b=VanityStudioDB.outfits.B
assert(same(a,savedA) and not b.settings.weaponPhysics and not b.settings.capeMotion.enabled)
assert(V:LoadOutfit('A',true));assert(weapon[2]==120 and slots[108][1]==2 and cape[3]==20)
-- Revert restores the named look's item, not a global fit or another slot.
V.placementTunerSlot=110;assert(V:SetBagTunerValue('up',.6));assert(V:RevertLookItemPlacement())
assert(native['110:1:1'].up==.4 and native['111:1:1'].up==.2)
assert(not V:CanRevertLookItemPlacement(V:GetBagTunerState()))
local revertState=V:GetBagTunerState()
local pendingEditor={editing=true,targetKey=revertState.key,field={key='up'},relativeOffset=.4,GetText=function() return '0' end}
V.bagTunerWindow={rows={{editor=pendingEditor}}}
assert(not V:CanRevertLookItemPlacement(revertState),'Relative zero matches the saved fit')
pendingEditor.GetText=function() return '.1' end
assert(V:CanRevertLookItemPlacement(revertState),'Uncommitted position edits enable Revert')
pendingEditor.editing=false
assert(not V:CanRevertLookItemPlacement(revertState))
V.bagTunerWindow=nil
-- Reload preserves unsaved edits (including bag drafts), without saving over A.
V.placementTunerSlot=111;assert(V:SetBagTunerValue('up',.35))
V.placementTunerSlot=nil;V.placementTunerBag=1;assert(V:SetBagTunerValue('up',.3))
VanityStudioCharacter=V:Copy(VanityStudioCharacter);VanityStudioDB=V:Copy(VanityStudioDB)
V.bagTunerDrafts={};V:InitializeLookSettings();V:InitializeBagTuning()
assert(VanityStudioCharacter.placements.fits['111:1:1'].values.up==.35)
assert(V:BagInstance(1).fits['1:1'].up==.3)
assert(VanityStudioDB.outfits.A.settings.placements.fits['111:1:1'].values.up==.2)
-- An inactive-copy operation takes settings from the selected snapshot.
assert(V:SaveOutfit('Copy',false,'B'))
assert(same(VanityStudioDB.outfits.Copy.settings,VanityStudioDB.outfits.B.settings))
-- Switching prompts before discarding; editor focus loss must never write into B.
local prompt=false
V.ConfirmLookChange=function() prompt=true end
local editor={editing=true,ClearFocus=function(self) assert(not self.editing,'Stale edit committed') end}
V.weaponPhysicsRows={{editor=editor}}
V.capePhysicsDraft=V:CapePhysicsConfig();V.capePhysicsDraft.run=199
local ok,why=V:LoadOutfit('B');assert(not ok and why=='pending' and prompt and editor.editing)
assert(V:LoadOutfit('B',true));assert(not editor.editing and not V.capePhysicsDraft and not next(V.bagTunerDrafts))
assert(weapon[1]==0 and cape[1]==0 and slots[108][1]==0 and native['111:1:1']==nil and not next(native))
assert(not V:HasUnsavedLookChanges())
-- Focusing unchanged text does not enable Save; typing does, without needing blur.
editor.editing=true;editor.initialText='40';editor.text='40';editor.GetText=function(self) return self.text end
assert(not V:HasUnsavedLookChanges())
editor.text='60';assert(V:HasUnsavedLookChanges())
editor.editing=nil
V.weaponPhysicsRows=nil
-- An empty/new look gets built-ins rather than carrying previous look settings.
V:ClearAll();assert(not VanityStudioCharacter.weaponPhysics and next(VanityStudioCharacter.placements.fits)==nil)
assert(not V:CanRevertLookItemPlacement({available=true,bag=110}))
assert(V:SaveOutfit('Fresh',false));assert(next(VanityStudioDB.outfits.Fresh.settings.placements.fits)==nil)
-- Migration never refreshes existing snapshots from another character/global fit.
local preserved=V:Copy(VanityStudioDB.outfits)
VanityStudioCharacter={selected={},weapons={},enabled=true,weaponBounce=190}
V:InitializeLookSettings();assert(same(preserved,VanityStudioDB.outfits))
print('PASS: per-look physics/placements, legacy migration, named/unsaved saves, reload, revert/default, deep copies, switch confirmation, stale editors and native reset dispatch')
