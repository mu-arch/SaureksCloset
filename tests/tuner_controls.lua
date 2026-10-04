-- Real tuner callbacks and value validation, including native focus ordering.
local function noop() end
local methods={}
local function widget(name)
    return setmetatable({name=name or "test",scripts={},shown=true},
        {__index=function(_,key) return methods[key] or (string.find(key,"^%u") and noop or nil) end})
end
local function run(control,event)
    local previous=this;this=control;assert(control.scripts[event])();this=previous
end
function methods:SetScript(event,fn) self.scripts[event]=fn end
function methods:GetScript(event) return self.scripts[event] end
function methods:GetName() return self.name end
function methods:GetFrameLevel() return 1 end
function methods:CreateTexture() return widget() end
methods.CreateFontString=methods.CreateTexture
function methods:SetText(text) self.text=text end
function methods:GetText() return self.text or "" end
function methods:Hide() self.shown=false end
function methods:Show() self.shown=true end
function methods:IsShown() return self.shown end
function methods:ClearFocus()
    if self.scripts.OnEditFocusLost then run(self,"OnEditFocusLost") end
end
CreateFrame=function(_,name) return widget(name) end
UIParent=widget();UISpecialFrames={}
GameTooltip={SetOwner=noop,ClearLines=noop,AddLine=noop,Show=noop,Hide=noop}
VanityStudioCharacter={enabled=true}
VanityStudio={frame=widget(),slotNames={},bagModelChoices={},placementTunerSlot=101}
local V=VanityStudio
dofile("addon/SaureksCloset/BagTuner.lua")
dofile("addon/SaureksCloset/BagTunerUI.lua")
local target="101:1:0";local available=true;local writes=0;local onSync
local function values()
    return {left=0,inset=0,up=0,pitch=0,roll=0,yaw=0,scale=100}
end
V.bagTunerDrafts={["101:1:0"]=values(),["101:1:1"]=values()}
function V:GetBagTunerState()
    return {key=target,bag=101,available=available,enabled=true,
        values=self.bagTunerDrafts[target],status=available and "" or "Waiting for the model."}
end
function V:SyncBagTuning() writes=writes+1;if onSync then onSync() end end
local function sheet() local f=widget();f.close=widget();return f end
local function make() return widget() end
local function button(_,_,_,_,_,callback)
    local b=widget();b:SetScript("OnClick",callback);return b
end
local function enable(w,yes) w.closetEnabled=yes end
V:CreateBagTunerUI(sheet,make,make,make,button,enable,button)
local f=V.bagTunerWindow
local editor=f.rows[3].editor
local plus,minus=f.rows[1].plus,f.rows[1].minus
local function typeValue(text)
    run(editor,"OnEditFocusGained");editor:SetText(text)
end
local function unchanged(before,message)
    assert(writes==before and V.bagTunerDrafts[target].left==0,message)
end

typeValue("not a number")
local before=writes
run(plus,"OnClick")
unchanged(before,"Rejected pending input must block the nudge")
assert(not editor.editing and not f.invalidInput)

-- WoW may deliver focus loss before the mouse button click.
typeValue("500")
run(plus,"OnMouseDown");editor:ClearFocus()
assert(f.invalidInput)
before=writes;run(plus,"OnClick")
unchanged(before,"Focus loss cannot let a rejected number fall through into a nudge")

typeValue("0.25")
run(plus,"OnMouseDown");run(plus,"OnClick")
assert(V.bagTunerDrafts[target].up==.25 and V.bagTunerDrafts[target].left==.005,
    "Valid pending text commits before the requested nudge")
IsShiftKeyDown=function() return true end
run(minus,"OnClick")
assert(math.abs(V.bagTunerDrafts[target].left+.045)<.000001,"Shift retains the larger step")
IsShiftKeyDown=nil
V.bagTunerDrafts[target].left=1;run(plus,"OnClick")
assert(V.bagTunerDrafts[target].left==1,"Nudging at the maximum remains bounded")
V.bagTunerDrafts[target].left=-1;run(minus,"OnClick")
assert(V.bagTunerDrafts[target].left==-1,"Nudging at the minimum remains bounded")

-- Body replacement between pressing and releasing must not edit the new fit.
run(plus,"OnMouseDown")
target="101:1:1";V:RefreshBagTunerUI();before=writes
run(plus,"OnClick")
unchanged(before,"A nudge started on another body must be rejected")
run(plus,"OnClick")
assert(V.bagTunerDrafts[target].left==.005,"A fresh click can edit the current body")

-- A stale editor can also be encountered before the periodic UI refresh.
target="101:1:0";V:RefreshBagTunerUI();typeValue("0.5")
target="101:1:1";f.targetKey=target
before=writes;run(plus,"OnClick")
assert(writes==before and V.bagTunerDrafts[target].up==0,
    "A rejected stale editor cannot be followed by a nudge on the new body")

-- Applying valid text can itself dispatch a body change from the renderer.
target="101:1:0";V:RefreshBagTunerUI();typeValue("0.5")
before=writes;local nextLeft=V.bagTunerDrafts["101:1:1"].left
onSync=function() target="101:1:1" end
run(plus,"OnClick");onSync=nil
assert(writes==before+1 and V.bagTunerDrafts["101:1:0"].up==.5 and V.bagTunerDrafts[target].left==nextLeft,
    "A body change during commit must block the subsequent nudge")

available=false;before=writes;run(plus,"OnClick")
assert(writes==before,"Unavailable placement cannot be nudged")
print("PASS: tuner nudges respect invalid text, focus ordering, body changes and valid numeric edits")

-- Absolute/relative is a view of the same fit, never a different saved format.
available=true;target="101:1:0";V.bagTunerDrafts[target]=values()
V.bagTunerDrafts[target].left=.35;V.bagTunerDrafts[target].up=-.2;V.bagTunerDrafts[target].pitch=25
V:RefreshBagTunerUI();before=writes
run(f.positionMode,"OnMouseDown");run(f.positionMode,"OnClick")
assert(f.relativeMode and f.positionMode.text=="Relative" and writes==before)
assert(f.rows[1].editor:GetText()=="0.0000" and f.rows[3].editor:GetText()=="0.0000" and f.rows[4].editor:GetText()=="0.0")
assert(f.rows[7].editor:GetText()=="100.0","Relative positioning cannot turn size into a zero percentage")
local function enter(row,text)
    local e=f.rows[row].editor;run(e,"OnEditFocusGained");e:SetText(text);run(e,"OnEnterPressed")
end
enter(1,"0.15");assert(math.abs(V.bagTunerDrafts[target].left-.5)<.000001)
enter(1,"0.15");assert(math.abs(V.bagTunerDrafts[target].left-.5)<.000001,"Re-entering a relative offset cannot accumulate it")
enter(3,"-0.3");assert(V.bagTunerDrafts[target].up==-.5)
enter(4,"-10");assert(V.bagTunerDrafts[target].pitch==15)
enter(7,"80");assert(V.bagTunerDrafts[target].scale==80)
run(plus,"OnClick");assert(math.abs(V.bagTunerDrafts[target].left-.505)<.000001 and f.rows[1].editor:GetText()=="0.1550")
V:RefreshBagTunerUI();assert(f.rows[1].editor:GetText()=="0.1550","Refresh must not drift the relative origin")
before=writes;enter(1,"0.66");assert(writes==before and f.invalidInput,"Relative values retain absolute safety limits")
run(f.positionMode,"OnClick");assert(f.relativeMode,"Rejected input blocks changing coordinate mode")
run(f.positionMode,"OnClick");assert(not f.relativeMode and f.rows[1].editor:GetText()=="0.5050" and writes==before)
run(f.positionMode,"OnClick");assert(f.rows[1].editor:GetText()=="0.0000","Re-enabling relative mode starts from the current fit")
-- A valid unfinished edit is interpreted in its original mode before switching.
run(editor,"OnEditFocusGained");editor:SetText("0.1")
run(f.positionMode,"OnClick")
assert(not f.relativeMode and math.abs(V.bagTunerDrafts[target].up+.4)<.000001)
-- Reset keeps its existing built-in-fit meaning and displays the resulting delta.
function V:BagTunerDefaults() return values() end
run(f.positionMode,"OnClick");run(f.rows[1].reset,"OnClick")
assert(V.bagTunerDrafts[target].left==0 and f.rows[1].editor:GetText()=="-0.5050")
-- Changing body/slot/model discards the old origin without moving the new item.
before=writes;run(f.positionMode,"OnMouseDown");target="101:1:1";V.bagTunerDrafts[target]=values();V.bagTunerDrafts[target].left=-.6
V:RefreshBagTunerUI();run(f.positionMode,"OnClick")
assert(f.relativeMode and writes==before and f.rows[1].editor:GetText()=="0.0000")
enter(1,"0.1");assert(V.bagTunerDrafts[target].left==-.5)
assert(V.bagTunerDrafts["101:1:0"].left==0,"New target edits cannot affect the previous fit")
-- Valid pending commits that change the target block the mode change.
run(editor,"OnEditFocusGained");editor:SetText("0.2")
onSync=function() target="101:1:0" end;run(f.positionMode,"OnClick");onSync=nil
assert(f.relativeMode and V.bagTunerDrafts["101:1:1"].up==.2)
print("PASS: relative fit offsets, unchanged mode switches, stable origins, absolute storage/size, reset semantics, bounds and target isolation")
