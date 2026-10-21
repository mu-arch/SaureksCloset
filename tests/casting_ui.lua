-- Real UI builders with 1.12-shaped frames; no newer widget APIs allowed.
dofile('tests/bags_list_ui.lua')
local V=VanityStudio
dofile('addon/SaureksCloset/Casting.lua')
local helpers={}
for i=1,80 do local name,value=debug.getupvalue(V.CreateUI,i);if not name then break end;helpers[name]=value end
local calls={}
SaureksClosetCastingVersion=function() return 1 end
SaureksClosetSetCasting=function(...) table.insert(calls,{...});return 1 end
V.LookSettingsChanged=function(self) self.castingDirty=true end
V.pagesByName.casting=CreateFrame('Frame',nil,V.frame)
V:CreateCastingPage(V.pagesByName.casting,helpers.label,helpers.button,helpers.enabled)
assert(#calls==0 and not V:CastingConfig().enabled,'Opening a page must not enable the feature')
assert(V.castingToggle.enabled and not V.castingChecks.effects.enabled and not V.castingChecks.swing.enabled)
local function click(control)
    assert(control.enabled);this=control;control.scripts.OnClick();this=nil
end
click(V.castingToggle)
assert(V:CastingConfig().enabled and V.castingDirty and calls[#calls][1]==1)
assert(V.castingChecks.effects.enabled and V.castingChecks.swing.enabled)
V.castingChecks.effects:SetChecked(nil);click(V.castingChecks.effects)
assert(not V:CastingConfig().effects and calls[#calls][2]==0)
V:SetTab('casting');assert(V.wardrobePage=='casting' and V.wardrobeSelectorLabel.text=='Casting')
for name,p in pairs(V.pagesByName) do assert(p:IsShown()==(name=='casting'),'Casting must not overlay another page: '..name) end
assert(V.castingStatus.anchor[5]+(-V.castingStatus.height)>-392,'Status must end above the wardrobe navigation')
local choices={};UIDropDownMenu_AddButton=function(info) choices[info.text]=info.func end
V.wardrobeMenu.initialize();assert(choices.Casting)
choices.Outfit();assert(V.tab=='armor' and not V.pagesByName.casting:IsShown())
choices.Casting();assert(V.tab=='casting' and V.pagesByName.casting:IsShown())
this=V.castingChecks.swing;this.scripts.OnEnter();assert(GameTooltip.title=='Swing weapon on release' and #GameTooltip.lines>0)
SaureksClosetCastingVersion=nil;V:RefreshCastingPage()
assert(not V.castingToggle.enabled and not V.castingChecks.effects.enabled and string.find(V.castingStatus.text,'restart',1,true))
print('PASS: Casting page navigation, layout bounds, opt-in controls, tooltips and missing-DLL handling')
