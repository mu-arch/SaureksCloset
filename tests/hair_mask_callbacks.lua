-- Exercise the remaining Haircraft callbacks on the actual Lua 5.0 runtime.
local function noop() end
local methods={}
local function widget() return setmetatable({scripts={},text=''}, {__index=function(_,key) return methods[key] or noop end}) end
function methods:SetScript(name,fn) self.scripts[name]=fn end
function methods:SetText(text) self.text=text end
function methods:GetText() return self.text end
function methods:GetFrameLevel() return 1 end
function methods:SetChecked(value) self.checked=value end
function methods:GetChecked() return self.checked end
CreateFrame=function() return widget() end
local buttons={}
local function button(_,title,_,_,_,callback) local b=widget();b:SetText(title);b:SetScript('OnClick',callback);table.insert(buttons,b);return b end
VanityStudio={frame=widget()};VanityStudioCharacter={enabled=true}
local V=VanityStudio
GameTooltip={SetOwner=noop,SetText=noop,AddLine=noop,Show=noop,Hide=noop}
SaureksClosetSetHaircraft=function() return 1 end
V.BagTuningAvailable=function() return false end
V.InvalidatePreviewModel=noop
V.OpenPlacementTuner=function(_,slot) assert(slot==111);V.opened=true end
dofile('addon/SaureksCloset/Haircraft.lua')
V:CreateHaircraftPage(widget(),widget,button,noop)
assert(table.getn(buttons)==2 and not V.hairMaskWindow and not V.CreateHairMaskWindow)
for _,b in ipairs(buttons) do this=b;b.scripts.OnEnter();b.scripts.OnLeave();b.scripts.OnClick() end
assert(VanityStudioCharacter.keepHairWithHat and V.opened)
local choice=nil;V.SetHairTrimming=function(_,value) choice=value end
this=V.haircraftTrim;this:SetChecked(false);this.scripts.OnClick();assert(choice==false)
this:SetChecked(true);this.scripts.OnClick();assert(choice==true)
this.scripts.OnEnter();this.scripts.OnLeave()
print('PASS: Haircraft callbacks under Lua 5.0; no obsolete trim window or controls')
