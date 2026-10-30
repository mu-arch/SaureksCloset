-- Differential oracle for the pre-optimization fit dispatch, frozen deliberately.
-- Compare native calls/retries, not just final saved values, through state changes.
local V={}
VanityStudio=V;VanityStudioDB={}
function V:Copy(t) local r={};for k,v in pairs(t or {}) do r[k]=type(v)=="table" and self:Copy(v) or v end;return r end
local supportedHat=true
function V:HatTuningAvailable() return supportedHat end
function V:NativeBody() return {race=1,sex=1} end
SaureksClosetRendererVersion=function() return 40301 end
SaureksClosetGetBagFitDefaults=function() return 1 end
local calls,failed,liveCalls={},false,0
SaureksClosetSetBagFit=function(...) local a=arg;table.insert(calls,table.concat(a,":"));return failed and -1 or 1 end
function V:SyncLiveBagFits() liveCalls=liveCalls+1;return not failed end
-- Capture the original local helpers so the oracle uses the same validations.
local f=assert(io.open('addon/SaureksCloset/BagTuner.lua'));local source=f:read('*a');f:close()
source=source.."\nreturn targets,keyFor"
local targets,keyFor=assert(loadstring(source))()
local function legacySync(self)
    if not self:BagTuningAvailable() then return false end
    self.bagTunerDrafts=self.bagTunerDrafts or {};self.bagTunerSynced=self.bagTunerSynced or {}
    local store=self:BagTunerStore();local enabled=store and store.enabled~=false
    local failure
    for _,target in ipairs(targets()) do for race=1,8 do for sex=0,1 do
        local key=keyFor(target,race,sex);local saved=self:BagTunerSaved(target,race,sex)
        local values=enabled and VanityStudioCharacter.enabled and (self.bagTunerDrafts[key] or (saved and saved.values)) or nil
        local motion=not (self.bagTunerPaused and self.bagTunerTargetKey==key)
        local signature="off"
        if values then
            signature=motion and "on" or "paused"
            for _,field in ipairs(self.bagTunerFields) do signature=signature..":"..string.format("%.6f",values[field.key]) end
        end
        if self.bagTunerSynced[key]~=signature then
            local ok,status
            if values then
                ok,status=pcall(SaureksClosetSetBagFit,target,race,sex,1,values.left,values.inset,values.up,values.pitch,values.roll,values.yaw,values.scale,motion and 1 or 0)
            else ok,status=pcall(SaureksClosetSetBagFit,target,race,sex,0) end
            if ok and status==1 then self.bagTunerSynced[key]=signature
            else failure="The placement renderer is not ready. Your values are kept; retrying." end
        end
    end end end
    if self.SyncLiveBagFits and not self:SyncLiveBagFits() then failure="The bag renderer is not ready. Your values are kept; retrying." end
    self.bagTunerError=failure;return not failure
end
local function character()
 return {enabled=true,placements={schema=1,enabled=true,fits={}}}
end
local old,new={},{ }
setmetatable(old,{__index=V});setmetatable(new,{__index=V})
old.character=character();new.character=character()
local steps=0
local function check()
 VanityStudioCharacter=old.character;calls={};liveCalls=0;legacySync(old)
 local expected=calls;local expectedLive=liveCalls
 VanityStudioCharacter=new.character;calls={};liveCalls=0;V.SyncBagTuning(new)
 assert(table.concat(calls,"|")==table.concat(expected,"|"),'Native dispatch changed at step '..steps)
 assert(liveCalls==expectedLive and old.bagTunerError==new.bagTunerError,'Retry/live bag behavior changed')
 steps=steps+1
end
local function both(fn) fn(old);fn(new);check();check() end
local function values(n) return {left=.01*n,inset=.02,up=.03,pitch=n,roll=0,yaw=0,scale=85} end
check();check()
both(function(v) v.character.placements.fits['108:1:1']={schema=1,bag=108,race=1,sex=1,values=values(1)} end)
both(function(v) v.bagTunerDrafts['108:1:1']=values(2) end)
both(function(v) v.bagTunerDrafts['108:1:1'].up=.2 end)
both(function(v) v.bagTunerPaused=true;v.bagTunerTargetKey='108:1:1' end)
both(function(v) v.bagTunerPaused=false end)
both(function(v) v.character.enabled=false end)
both(function(v) v.character.enabled=true end)
both(function(v) v.character.placements.enabled=false end)
both(function(v) v.character.placements.enabled=true end)
failed=true
both(function(v) v.bagTunerDrafts['108:1:1']=values(3) end)
failed=false;check();check()
failed=true
both(function(v) v.bagTunerDrafts={};v.character.placements.fits={} end)
failed=false;check();check()
-- Retry failed initial clears as well as failed edited fits.
failed=true;both(function(v) v.bagTunerSynced={} end)
failed=false;check();check()
-- Look swaps, world entry, addon reload and capability changes.
both(function(v) v.character=character();v.bagTunerSynced={};v.bagTunerDrafts={} end)
supportedHat=false;check();supportedHat=true;check()
for i=1,40 do
 local target=101+i%11;local race=1+i%8;local sex=i%2;local key=target..":"..race..":"..sex
 both(function(v) v.character.placements.fits[key]={schema=1,bag=target,race=race,sex=sex,values=values(i%5)} end)
end
both(function(v) v.character.placements.fits['bad key']={};v.character.placements.fits['201:1:0']={} end)
both(function(v) v.character.placements={schema=99,fits={}} end)
both(function(v) v.character=character() end)
-- Settled empty settings should perform zero saved-fit validations.
local visits=0;local saved=V.BagTunerSaved
new.BagTunerSaved=function(self,...) visits=visits+1;return saved(self,unpack(arg)) end
VanityStudioCharacter=new.character;V.SyncBagTuning(new);assert(visits==0)
print('PASS: sparse placement sync matches original native call order, values, reset, load, failure/retry and live bag behavior ('..steps..' updates)')
