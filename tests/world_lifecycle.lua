-- Leaving/entering the world must pause the local controllers without sharing.
function CreateFrame()
    return {RegisterEvent=function() end,SetScript=function(self,name,fn) self[name]=fn end}
end
SlashCmdList={}
dofile('addon/SaureksCloset/Core.lua')
for _,name in ipairs({'Physics','CapePhysics','Haircraft','Casting'}) do
    dofile('addon/SaureksCloset/'..name..'.lua')
end
local V=VanityStudio
V.ready=true
VanityStudioCharacter={enabled=true,selected={},weapons={}}
local writes=0
local function native() writes=writes+1;return 1 end
SaureksClosetPhysicsVersion=function() return 4 end
SaureksClosetCastingVersion=function() return 1 end
SaureksClosetHairMaskVersion=function() return 3 end
for _,name in ipairs({'SetWeaponPhysics','SetWeaponSlotPhysics','SetCapeMotion','SetHaircraft','SetHairMask','SetSecondHat','SetCasting'}) do
    _G['SaureksCloset'..name]=native
end
V.RealWeaponItems=function() return {0,0,0} end
V.NativeBody=function() return nil end
local entered=0
for _,name in ipairs({'CheckArmorRepairs','RefreshTrueBody','RefreshBody','RefreshPortraits','InvalidatePreviewModel','QueueRespawnRecovery'}) do
    V[name]=function() entered=entered+1 end
end
local function sync()
    V:SyncPhysics();V:SyncCapePhysics();V:SyncHaircraft();V:SyncHairMask();V:SyncSecondHat();V:SyncCasting()
end
for i=1,3 do
    sync();assert(writes>0)
    event='PLAYER_LEAVING_WORLD';V.events.OnEvent()
    assert(V.worldPaused)
    local before=writes
    sync();assert(writes==before,'No native controller writes during world transfer')
    event='PLAYER_ENTERING_WORLD';V.events.OnEvent()
    assert(not V.worldPaused and entered==i*6)
    sync();assert(writes>before,'Local controllers resume on world entry')
end
print('PASS: world transfer pauses and resumes local physics, cape, hair, second hat and casting without sharing')
