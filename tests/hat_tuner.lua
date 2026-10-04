dofile('tests/haircraft.lua')
local V=VanityStudio
-- Replace the bag-list test's state stub with the real persistence/dispatch.
dofile('addon/SaureksCloset/BagTuner.lua')
SaureksClosetSetHaircraft=function() return 1 end
SaureksClosetRendererVersion=function() return 40011 end
local native={}
SaureksClosetGetBagFitDefaults=function(target,race,sex) return 1,0,0,0,0,0,0,100 end
SaureksClosetSetBagFit=function(target,race,sex,enabled,left,inset,up,pitch,roll,yaw,scale)
    native[target..':'..race..':'..sex]=enabled==1 and {left=left,inset=inset,up=up,pitch=pitch,roll=roll,yaw=yaw,scale=scale} or nil
    return 1
end
V.SyncLiveBagFits=nil
V.Copy=function(self,t) local out={};for k,v in pairs(t or {}) do out[k]=type(v)=='table' and self:Copy(v) or v end;return out end
V.NativeBody=function() return {race=1,sex=0} end
VanityStudioCharacter.enabled=true;VanityStudioCharacter.body=nil
VanityStudioDB={};VanityStudioRaces={{'Human'},{'Orc'}}
V.RefreshPreview=function() end
V:InitializeBagTuning();V:RefreshHaircraftPage();V:SetTab('haircraft')
assert(V.haircraftHat.enabled)
V.haircraftHat.scripts.OnClick()
assert(V.tab=='haircraft' and V.bagTunerWindow:IsShown() and V.placementTunerSlot==111)
local state=V:GetBagTunerState()
assert(state.available and state.title=='Hat - Human Male' and state.values.scale==100)
assert(V:SetBagTunerValue('up',.075) and V:SetBagTunerValue('pitch',8) and V:SetBagTunerValue('scale',108))
assert(native['111:1:0'].up==.075 and native['111:1:0'].scale==108)
assert(not native['108:1:0'],'Hat adjustments cannot affect stowed weapons')
assert(V:SaveBagTunerFit());V:SetBagTunerValue('up',.15);V:LoadBagTunerFit()
assert(V:GetBagTunerState().values.up==.075)
V:ResetBagTunerFit();assert(native['111:1:0'].up==0 and native['111:1:0'].scale==100)
V:InitializeBagTuning();assert(native['111:1:0'].up==.075,'Saved fit survives reload')
VanityStudioCharacter.body={race=1,sex=1};assert(V:GetBagTunerState().values.up==0)
VanityStudioCharacter.body=nil;assert(V:GetBagTunerState().values.up==.075)
VanityStudioCharacter.enabled=false;V:SyncBagTuning();assert(not native['111:1:0'])
VanityStudioCharacter.enabled=true;V:SyncBagTuning();assert(native['111:1:0'].up==.075)
local defaults=SaureksClosetGetBagFitDefaults
SaureksClosetGetBagFitDefaults=function(target,race,sex) if target==111 then return -2 end;return defaults(target,race,sex) end
V:RefreshHaircraftPage();assert(not V.haircraftHat.enabled and not V:GetBagTunerState().available)
print('PASS: Haircraft hat tuner opens on its own page; live movement, rotation, size, save/load/default, race/sex isolation, reload, disable and old DLL gating')
