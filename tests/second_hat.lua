dofile('tests/look_settings.lua')
dofile('addon/SaureksCloset/Haircraft.lua')
dofile('addon/SaureksCloset/Preview.lua')
local V=VanityStudio
V.InvalidatePreviewModel=function() end
V.index={[70001]={70001,'Hat',1,1,4,1,1,75},[70002]={70002,'Bandana',1,1,4,1,1,76},[70003]={70003,'Sword',13,1,2,7,1,77}}
local calls,status={},1
SaureksClosetSecondHatVersion=function() return 1 end
SaureksClosetSetSecondHat=function(token,display) table.insert(calls,{token,display});return status end
-- Virtual slot 112 must never enter the real inventory override API.
SetUnitVisibleItemID=function(_,slot) assert(slot~=112) end
VanityStudioDB.outfits={}
VanityStudioCharacter={enabled=true,selected={[1]=70001},managed={},weapons={},placements={schema=1,enabled=true,fits={}}}
V:InitializeLookSettings()
assert(V:Select(112,70002) and VanityStudioCharacter.secondHat==70002)
assert(VanityStudioCharacter.selected[1]==70001 and V:SlotSelection(112)==70002)
assert(calls[#calls][1]==0 and calls[#calls][2]==76)
assert(not V:Select(112,70003) and VanityStudioCharacter.secondHat==70002)
assert(V:SaveOutfit('Two hats',false))
assert(VanityStudioDB.outfits['Two hats'].settings.secondHat==70002)
assert(not V:HasUnsavedLookChanges())
assert(V:ClearSlot(112) and calls[#calls][2]==0 and V:HasUnsavedLookChanges())
assert(VanityStudioCharacter.selected[1]==70001)
assert(V:SaveOutfit('One hat',false) and V:LoadOutfit('Two hats',true))
assert(VanityStudioCharacter.secondHat==70002 and calls[#calls][2]==76)
assert(not V:HasUnsavedLookChanges())
-- Live previews, cancel, main Save and commit share the same working setting.
assert(V:DraftSlot(112,70001) and calls[#calls][2]==75 and VanityStudioCharacter.secondHat==70002)
assert(not VanityStudioCharacter.managed[112] and not V.applied[112])
V:CancelDraft();assert(calls[#calls][2]==76 and VanityStudioCharacter.secondHat==70002)
assert(V:DraftSlot(112,70001) and V:CommitDraft() and VanityStudioCharacter.secondHat==70001)
assert(V:HasUnsavedLookChanges() and VanityStudioDB.outfits['Two hats'].settings.secondHat==70002)
assert(V:SaveOutfit('Two hats',true) and not V:HasUnsavedLookChanges())
status=-2;assert(not V:SetSecondHat(70002) and VanityStudioCharacter.secondHat==70001);status=1
assert(not V:HasUnsavedLookChanges(),'Failed native changes must not dirty the look')
VanityStudioCharacter.enabled=false;assert(V:SyncSecondHat() and calls[#calls][2]==0)
assert(VanityStudioCharacter.secondHat==70001)
assert(V:SyncSecondHat(123,70002) and calls[#calls][1]==123 and calls[#calls][2]==76,'Saved previews own their selection')
VanityStudioCharacter.enabled=true;assert(V:SyncSecondHat() and calls[#calls][2]==75)
assert(V:LoadOutfit('One hat',true) and not VanityStudioCharacter.secondHat and calls[#calls][2]==0)
assert(V:LoadOutfit('Two hats',true) and VanityStudioCharacter.secondHat==70001)
V:ClearAll();assert(not VanityStudioCharacter.secondHat and calls[#calls][2]==0)
SaureksClosetSetSecondHat=nil;assert(not V:SetSecondHat(70002));assert(V:SyncSecondHat())
print('PASS: independent second hat selection, native isolation, per-look save/load/dirty/reset, preview draft/cancel/commit, disabled addon, failures and old DLL gating')
