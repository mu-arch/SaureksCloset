-- Exercise actual look persistence, casting controllers and appearance choice.
dofile('tests/look_settings.lua')
dofile('addon/SaureksCloset/Casting.lua')
local V=VanityStudio
local calls,status={},1
SaureksClosetCastingVersion=function() return 1 end
SaureksClosetSetCasting=function(...) table.insert(calls,{...});return status end
SaureksClosetWeaponAssets={[35]={2,10},[25]={1,7},[647]={2,8},[100]={4,2}}
local equipped=35
GetInventoryItemLink=function(_,slot) return slot==16 and equipped and '|Hitem:'..equipped..':0|h[Test]|h' or nil end
VanityStudioDB.outfits={Normal={version=4,slots={},weapons={},settings={keepHairWithHat=false}}}
VanityStudioCharacter={enabled=true,selected={},weapons={},lookSettingsVersion=1}
assert(V:LoadOutfit('Normal',true) and calls[#calls][1]==0)
assert(V:SetCasting('enabled',false) and not V:HasUnsavedLookChanges(),'An unchanged default must not dirty the look')
assert(V:SetCasting('enabled',true) and calls[#calls][1]==1 and calls[#calls][4]==35)
assert(V:HasUnsavedLookChanges() and V:NeedsLookChangeConfirmation('Normal'))
assert(VanityStudioDB.outfits.Normal.settings.casting==nil)
assert(V:SetCasting('effects',false) and V:SetCasting('swing',false))
assert(calls[#calls][2]==0 and calls[#calls][3]==0)
assert(V:SaveOutfit('Weapon cast',false))
local saved=VanityStudioDB.outfits['Weapon cast'].settings.casting
assert(saved.enabled and not saved.effects and not saved.swing and not V:HasUnsavedLookChanges())
assert(V:LoadOutfit('Normal',true) and calls[#calls][1]==0)
assert(V:LoadOutfit('Weapon cast',true) and calls[#calls][1]==1 and calls[#calls][2]==0)
assert(V:SetCasting('effects',true));local snapshot=V:Copy(VanityStudioCharacter)
VanityStudioCharacter=snapshot;V:InitializeLookSettings();V:SyncLookSettings()
assert(calls[#calls][2]==1 and not saved.effects,'Unsaved changes survive reload separately from the saved look')
assert(V:LoadOutfit('Weapon cast',true) and calls[#calls][2]==0)
status=-1;assert(not V:SetCasting('enabled',false));assert(V:CastingConfig().enabled and not V:HasUnsavedLookChanges())
status=1
VanityStudioCharacter.weapons={independent=true,[108]=25,[101]=647}
assert(V:CastingWeapon()==25,'Cast with the drawn transmog, not a carried decoration')
VanityStudioCharacter.weapons={independent=true,[101]=647};assert(V:CastingWeapon()==35)
VanityStudioCharacter.selected[16]=0;assert(V:CastingWeapon()==0,'Never revive an explicitly hidden weapon')
VanityStudioCharacter.selected[16]=nil;equipped=nil;assert(V:CastingWeapon()==0)
equipped=100;assert(V:CastingWeapon()==0,'Ranged items cannot be treated as a staff')
equipped=35;VanityStudioCharacter.enabled=false;assert(V:SyncCasting() and calls[#calls][1]==0)
VanityStudioCharacter.enabled=true;assert(V:SyncCasting() and calls[#calls][1]==1)
V.sharingWorldPaused=true;local count=#calls;assert(not V:SyncCasting() and #calls==count);V.sharingWorldPaused=nil
SaureksClosetCastingVersion=nil;assert(not V:SetCasting('enabled',false) and V:CastingConfig().enabled)
SaureksClosetCastingVersion=function() return 1 end
V:ClearAll();assert(not V:CastingConfig().enabled and calls[#calls][1]==0)
assert(not V:SetCasting('invalid',true))
print('PASS: per-look casting defaults, dirty/save/discard/reload, failures, transmog selection, hidden/unsupported weapons and addon toggle')
