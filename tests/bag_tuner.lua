-- Run with Lua 5.0.3. Tests actual tuner persistence/export and native dispatch.
local function copy(t)
    local out={};for k,v in pairs(t or {}) do out[k]=type(v)=="table" and copy(v) or v end;return out
end
local messages,native,calls={},{},0
local actual={race=2,sex=0};local version=30510;local fail=false
VanityStudio={VERSION="3.5.1",Copy=function(self,t) return copy(t) end,
    Message=function(self,t) table.insert(messages,t) end,NativeBody=function() return copy(actual) end}
VanityStudioRaces={};for race=1,8 do VanityStudioRaces[race]={"Race "..race} end
VanityStudioDB={outfits={untouched={name="My look"}}}
VanityStudioCharacter={enabled=true,weapons={backBag=1}}
function date() return "2026-09-13 12:34:56" end
function SaureksClosetRendererVersion() return version end
function SaureksClosetGetBagFitDefaults(bag,race,sex)
    assert(bag==1 and race>=1 and race<=8 and (sex==0 or sex==1))
    return 1,.126,race==2 and sex==0 and .075 or 0,race==2 and sex==0 and -.0225 or -.06,
        race==2 and (sex==0 and 15 or 10) or 0,race==2 and 15 or 0,0,85
end
function SaureksClosetSetBagFit(bag,race,sex,enabled,left,inset,up,pitch,roll,yaw,scale,motion)
    calls=calls+1;if fail then return -1 end
    local key=bag..":"..race..":"..sex
    if enabled==0 then native[key]=nil
    else native[key]={left=left,inset=inset,up=up,pitch=pitch,roll=roll,yaw=yaw,scale=scale,motion=motion} end
    return 1
end
dofile("addon/SaureksCloset/BagTuner.lua")
local V=VanityStudio
V:InitializeBagTuning();assert(calls==16 and not next(native))
local state=V:GetBagTunerState();assert(state.available and state.key=="1:2:0" and state.values.scale==85)
assert(not state.saved and not state.dirty and native[state.key].pitch==15)
local previousCalls=calls;V:GetBagTunerState();V:SyncBagTuning();assert(calls==previousCalls)
assert(V:SetBagTunerValue("up",.1));assert(native[state.key].up==.1 and V:GetBagTunerState().dirty)
assert(not next(VanityStudioDB.bagTuning.fits)) -- Edits are not silently saved.
for _,invalid in ipairs({-.01,201,1/0,0/0}) do assert(not V:SetBagTunerValue("scale",invalid)) end
assert(not V:SetBagTunerValue("inset",2));assert(not V:SetBagTunerValue("unknown",0));assert(not V:SetBagTunerValue("up",".2"))
assert(native[state.key].up==.1 and native[state.key].scale==85)
assert(V:SaveBagTunerFit());local saved=VanityStudioDB.bagTuning.fits[state.key]
assert(saved.values.up==.1 and saved.renderer==30510 and not V:GetBagTunerState().dirty)
V:SetBagTunerValue("up",.2);assert(saved.values.up==.1) -- No aliasing the saved data.
V:LoadBagTunerFit();assert(native[state.key].up==.1)
V:ResetBagTunerFit();assert(native[state.key].up==-.0225 and saved.values.up==.1 and V:GetBagTunerState().dirty)
-- Each reset restores only its own compiled default; saved work stays intact.
local savedBefore=copy(saved)
for _,field in ipairs(V.bagTunerFields) do
    local defaults=V:BagTunerDefaults(state.bag,state.race,state.sex)
    for _,other in ipairs(V.bagTunerFields) do assert(V:SetBagTunerValue(other.key,defaults[other.key]+other.step*3)) end
    local before=V:GetBagTunerState().values
    assert(V:ResetBagTunerField(field.key))
    local after=V:GetBagTunerState().values
    for _,other in ipairs(V.bagTunerFields) do
        local expected=other.key==field.key and defaults[other.key] or before[other.key]
        assert(after[other.key]==expected and native[state.key][other.key]==expected)
        assert(saved.values[other.key]==savedBefore.values[other.key])
    end
    assert(saved.savedAt==savedBefore.savedAt and saved.renderer==savedBefore.renderer)
end
local beforeInvalid=V:GetBagTunerState().values;previousCalls=calls
assert(not V:ResetBagTunerField("unknown"));assert(not V:ResetBagTunerField(nil));assert(not V:ResetBagTunerField({}))
assert(calls==previousCalls)
for _,field in ipairs(V.bagTunerFields) do assert(V:GetBagTunerState().values[field.key]==beforeInvalid[field.key]) end
V:LoadBagTunerFit();V:SetBagTunerPaused(true);assert(native[state.key].motion==0)
actual.sex=1;local female=V:GetBagTunerState()
assert(female.key=="1:2:1" and female.values.pitch==10 and female.values.inset==0)
assert(native[state.key].motion==1 and native[female.key].motion==0)
V:SetBagTunerValue("left",.15);V:SaveBagTunerFit();assert(saved.values.left==.126)
V:SetBagTunerPaused(false);assert(native[female.key].motion==1)
V:SetBagTunerEnabled(false);assert(not next(native));assert(not V:GetBagTunerState().enabled)
V:SetBagTunerValue("left",.2);assert(not next(native))
V:SetBagTunerValue("pitch",23);assert(V:ResetBagTunerField("pitch"))
local inactive=V:GetBagTunerState()
assert(inactive.values.pitch==10 and inactive.values.left==.2 and not next(native))
assert(VanityStudioDB.bagTuning.fits[female.key].values.left==.15)

V:SetBagTunerEnabled(true);assert(native[female.key].left==.2)
-- Reload writes only saved fits back to the DLL, resuming motion and dropping drafts.
V:InitializeBagTuning();assert(native[female.key].left==.15 and native[state.key].up==.1 and native[female.key].motion==1)
assert(VanityStudioDB.outfits.untouched.name=="My look")
VanityStudioCharacter.body={race=4,sex=0};assert(V:GetBagTunerState().race==4)
VanityStudioCharacter.enabled=false;assert(V:GetBagTunerState().race==2)
VanityStudioCharacter.enabled=true;VanityStudioCharacter.body=nil
assert(V:LoadBagTunerFit()) -- Current female profile is saved.
actual={race=3,sex=0};assert(not V:LoadBagTunerFit())
local oldDefault=SaureksClosetGetBagFitDefaults
SaureksClosetGetBagFitDefaults=function() return 1,0,0,0,0,0,0,0/0 end
actual={race=5,sex=0};assert(not V:GetBagTunerState().available)
SaureksClosetGetBagFitDefaults=oldDefault
actual={race=2,sex=0};V:GetBagTunerState()
fail=true;V:SetBagTunerValue("yaw",17);assert(V.bagTunerError and native[state.key].yaw==0)
fail=false;V:SyncBagTuning();assert(not V.bagTunerError and native[state.key].yaw==17)
-- Bad stored rows never reach the renderer, and future store formats survive.
VanityStudioDB.bagTuning.fits["1:7:1"]={schema=1,bag=1,race=7,sex=1,values={scale=0/0}}
V:InitializeBagTuning();assert(not native["1:7:1"])
local old=VanityStudioDB.bagTuning;VanityStudioDB.bagTuning={schema=99,fits={valuable="future data"}}
assert(not V:GetBagTunerState().available);V:SyncBagTuning();assert(not next(native))
assert(VanityStudioDB.bagTuning.fits.valuable=="future data")
VanityStudioDB.bagTuning=old;V:InitializeBagTuning()
version=30509;assert(not V:GetBagTunerState().available);version=30510
-- Export contains both saved profiles and the current unsaved draft. A fixed
-- optional file target supports collecting data without changing source files.
V:SetBagTunerValue("yaw",23);saved.savedAt='Quoted "date"\n\1';saved.renderer=0/0
local written
function WriteFile(path,mode,text) assert(path=="SaureksCloset-bag-fits.json" and mode=="w");written=text;return true end
local exported=V:ExportBagTunerFits();assert(exported==written and exported==VanityStudioDB.bagTuning.lastExport)
assert(string.find(exported,'"yaw": 23.000000',1,true) and string.find(exported,'"yaw": 0.000000',1,true))
assert(string.find(exported,'\\u0001',1,true) and not string.find(exported,"nan"))
WriteFile=function() error("writer unavailable") end
assert(V:ExportBagTunerFits()==exported)
local output=assert(io.open((os.getenv("TMPDIR") or "/tmp").."/saureks-bag-tuner-test.json","w"));output:write(exported);output:close()
print("PASS: bag tuner live dispatch, fit identity, validation, defaults, pause, persistence, individual field reset/load, retries and JSON export")
