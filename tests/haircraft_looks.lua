-- Real persistence, migration, toggle controller, and native bridge dispatch.
dofile('tests/look_settings.lua')
dofile('addon/SaureksCloset/Haircraft.lua')
local V=VanityStudio
local calls,result={},1
SaureksClosetSetHaircraft=function(on) table.insert(calls,on);return result end
SaureksClosetHairMaskVersion=function() return 3 end
SaureksClosetSetHairMask=function() return 1 end
VanityStudioDB.outfits={
    Old={version=4,slots={},weapons={},settings={placements={schema=1,enabled=true,fits={}}}},
    Other={version=4,slots={},weapons={},settings={}},
    Off={version=4,slots={},weapons={},settings={keepHairWithHat=false}},
}
VanityStudioCharacter={enabled=true,selected={},weapons={},keepHairWithHat=true,trimHair=false,lookSettingsVersion=1,activeOutfit='Old'}
V:InitializeLookSettings()
assert(VanityStudioDB.outfits.Old.settings.keepHairWithHat==true)
assert(VanityStudioDB.outfits.Other.settings.keepHairWithHat==true)
assert(VanityStudioDB.outfits.Off.settings.keepHairWithHat==false,'Preserve an explicit saved Off value')
assert(V:LoadOutfit('Old',true) and calls[#calls]==1)
assert(not V:HasUnsavedLookChanges())
assert(V:SetHaircraft(false) and V:HasUnsavedLookChanges() and V:ActiveSavedLookName()=='Old')
assert(VanityStudioCharacter.unsaved.settings.keepHairWithHat==false)
assert(VanityStudioDB.outfits.Old.settings.keepHairWithHat==true,'Editing must not overwrite the named look')
assert(V:SaveOutfit('Old',true) and VanityStudioDB.outfits.Old.settings.keepHairWithHat==false)
assert(not V:HasUnsavedLookChanges())
assert(V:LoadOutfit('Other',true) and VanityStudioCharacter.keepHairWithHat and calls[#calls]==1)
assert(V:LoadOutfit('Old',true) and VanityStudioCharacter.keepHairWithHat==false and calls[#calls]==0)
assert(V:SaveOutfit('Copy',false,'Other') and VanityStudioDB.outfits.Copy.settings.keepHairWithHat)
-- Unsaved Off survives reload while the saved look stays On.
assert(V:LoadOutfit('Other',true) and V:SetHaircraft(false))
VanityStudioCharacter=V:Copy(VanityStudioCharacter);V:InitializeLookSettings();V:SyncLookSettings()
assert(VanityStudioCharacter.keepHairWithHat==false and calls[#calls]==0)
assert(VanityStudioDB.outfits.Other.settings.keepHairWithHat==true)
assert(V:NeedsLookChangeConfirmation('Copy'),'Hair-only edits must trigger the discard warning')
assert(V:LoadOutfit('Copy',true) and calls[#calls]==1)
assert(not V:HasUnsavedLookChanges())
-- Failed native changes roll back and do not mark the saved look edited.
result=-1;assert(not V:SetHaircraft(false))
assert(VanityStudioCharacter.keepHairWithHat and not V:HasUnsavedLookChanges())
result=1;assert(V:SetHaircraft(true) and not V:HasUnsavedLookChanges(),'An unchanged toggle stays clean')
-- Fresh/default looks use normal hat visibility; older snapshots retain their copy.
V:ClearAll();assert(not VanityStudioCharacter.keepHairWithHat and calls[#calls]==0)
assert(V:SaveOutfit('Fresh',false) and VanityStudioDB.outfits.Fresh.settings.keepHairWithHat==false)
assert(VanityStudioCharacter.trimHair==false,'The separate trimming preference is unchanged')
VanityStudioCharacter.keepHairWithHat=false;V:InitializeLookSettings()
assert(VanityStudioDB.outfits.Other.settings.keepHairWithHat==true,'Migration must not overwrite stored hair choices')
print('PASS: per-look Toggle Hair migration, on/off persistence, dirty/save/discard, copies, reload, native restoration, failures and defaults')
