-- Offline cost of settled half-second controllers; native calls are counted stubs.
function CreateFrame() return {RegisterEvent=function() end,SetScript=function() end} end
SlashCmdList={}
dofile('addon/SaureksCloset/Core.lua')
for _,name in ipairs({'BagTuner','Physics','CapePhysics','Haircraft','Casting'}) do
    dofile('addon/SaureksCloset/'..name..'.lua')
end
local V=VanityStudio
VanityStudioDB={}
VanityStudioCharacter={enabled=true,selected={},weapons={},placements={schema=1,enabled=true,fits={}}}
V.NativeBody=function() return nil end
V.RealWeaponItems=function() return {0,0,0} end
local calls,fitVisits={},0
local function native(name,result)
    return function() calls[name]=(calls[name] or 0)+1;return result or 1 end
end
for _,entry in ipairs({{'RendererVersion',40301},{'PhysicsVersion',4},{'CastingVersion',1},{'HairMaskVersion',3},
    {'GetBagFitDefaults'},{'SetBagFit'},{'SetWeaponPhysics'},{'SetWeaponSlotPhysics'},{'SetCapeMotion'},
    {'SetHaircraft'},{'SetHairMask'},{'SetSecondHat'},{'SetCasting'}}) do
    _G['SaureksCloset'..entry[1]]=native(entry[1],entry[2])
end
local saved=V.BagTunerSaved
function V:BagTunerSaved(...) fitVisits=fitVisits+1;return saved(self,unpack(arg)) end
local function sync()
    V:SyncPhysics();V:SyncHaircraft();V:SyncSecondHat();V:SyncCasting();V:SyncBagTuning()
end
sync();calls={};fitVisits=0
collectgarbage('collect')
local iterations=1000
local start=os.clock()
for i=1,iterations do sync() end
local duration=os.clock()-start
local count=0;for _,n in pairs(calls) do count=count+n end
print(string.format('Settled controller sample: %.3f ms/cycle; %.0f fit visits; %.0f native calls; %.0f placement writes',duration*1000/iterations,fitVisits/iterations,count/iterations,(calls.SetBagFit or 0)/iterations))
local keys={};for key in pairs(calls) do table.insert(keys,key) end;table.sort(keys)
for _,key in ipairs(keys) do print(string.format('  %s: %.0f/cycle',key,calls[key]/iterations)) end
print('Runs twice per second; excludes SyncWeapons/SyncLiveBagFits, client memory access, model work and UI refreshes.')
