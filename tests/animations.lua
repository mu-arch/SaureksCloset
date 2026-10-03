-- Use real bag state, tuner and UI helpers; only engine frames/DLL are mocked.
dofile('tests/bags_list_ui.lua')
local V=VanityStudio
local capeCalls={}
SaureksClosetGetBagFitDefaults=function(target,race,sex,mount) return 1,0,0,-.15,0,0,mount==1 and -90 or mount==2 and 90 or 0,70 end
SaureksClosetAnimationVersion=function() return 1 end
SaureksClosetSetCapeAnimation=function(mode) table.insert(capeCalls,mode);return 1 end
V.InvalidatePreviewModel=function() end
local originalCreateFrame=CreateFrame
function CreateFrame(kind,name,parent,template)
    local f=originalCreateFrame(kind,name,parent,template)
    if kind=='Slider' then
        -- Vanilla sliders are Frames, not Buttons: no Enable/Disable methods.
        local base=getmetatable(f).__index
        setmetatable(f,{__index=function(_,key)
            if key=='Enable' or key=='Disable' then return nil end
            return base[key]
        end})
        function f:EnableMouse(on) self.mouseEnabled=on and true or false end
        function f:SetMinMaxValues(low,high) self.min=low;self.max=high end
        function f:SetValueStep(step) self.step=step end
        function f:GetValue() return self.value end
        function f:SetValue(value)
            self.value=value
            if self.scripts.OnValueChanged then local old=this;this=self;self.scripts.OnValueChanged();this=old end
        end
    end
    return f
end
local helpers={}
for i=1,80 do local name,value=debug.getupvalue(V.CreateUI,i);if not name then break end;helpers[name]=value end
local page=CreateFrame('Frame',nil,V.frame);page:SetAllPoints(V.frame)
dofile('addon/SaureksCloset/Animations.lua')
V:CreateAnimationsPage(page,helpers.sheet,helpers.section,helpers.label,helpers.settingsButton,helpers.enabled)
assert(#capeCalls==0,'Opening controls cannot enable cape animation')
assert(#V.animationBagRows==5 and not V.bagTunerWindow.pause)
local bag=V:BagInSlot(1);assert(bag)
assert(V:BagPhysicsAmount(bag)==100 and bag.physics~=false)
assert(V:SetBagPhysics(bag.id,false,65));V:RefreshAnimationsPage()
local normalized=V:NormalizeBags(VanityStudioCharacter.weapons);local saved
for _,b in ipairs(normalized) do if b.id==bag.id then saved=b end end
assert(saved.physics==false and saved.amplitude==65,'Per-bag settings survive save normalization')
assert(not V.animationBagRows[1].slider.mouseEnabled and not V.animationBagRows[1].toggle.checked)
V.animationBagRows[1].slider:SetValue(20)
assert(bag.physics==false and bag.amplitude==65,'Disabled slider callbacks cannot enable physics or change amplitude')
assert(V.animationBagRows[1].slider.alpha==.4,'Unavailable sliders are visibly dimmed')
assert(V:SetBagPhysics(bag.id,true,65));V:RefreshAnimationsPage()
assert(V.animationBagRows[1].slider.mouseEnabled and V.animationBagRows[1].slider.alpha==1)
local other=V:BagInSlot(3);local previous=other and V:BagPhysicsAmount(other)
V.animationBagRows[1].slider:SetValue(40)
assert(bag.amplitude==40 and bag.physics)
assert(not other or V:BagPhysicsAmount(other)==previous,'Slider cannot change another bag')
assert(not V:SetBagPhysics(bag.id,true,0/0) and not V:SetBagPhysics(bag.id,true,201))
local before=V:BagSignature(VanityStudioCharacter.weapons);V:SetBagPhysics(bag.id,false,40)
assert(before~=V:BagSignature(VanityStudioCharacter.weapons),'Physics edits invalidate saved-look comparisons and previews')
local calls={}
SaureksClosetSetBagInstanceFit=function(...) table.insert(calls,{...});return 1 end
V.bagFitCaches={};V:ApplyBagRenderer(0,VanityStudioCharacter.weapons,false)
local found=false
for _,args in ipairs(calls) do if args[2]==bag.id then assert(args[5]==1 and args[13]==0 and args[14]==40);found=true end end
assert(found,'Disabled dynamics reach native code even with built-in fits')
V:BagTunerStore().enabled=false;V.bagFitCaches={};calls={};V:ApplyBagRenderer(0,VanityStudioCharacter.weapons,false)
assert(#calls>0 and calls[1][13]==0,'Disabling live placement tuning cannot re-enable physics')
local fitReference=bag.fits;local model,mount=bag.model,bag.mount
local oldThis=this;this=V.animationBagRows[1].reset;this.scripts.OnClick();this=oldThis
assert(bag.physics and bag.amplitude==100 and bag.fits==fitReference and bag.model==model and bag.mount==mount,'Per-bag Default preserves placement and model')
assert(not other or V:BagPhysicsAmount(other)==previous,'Per-bag Default does not reset another bag')
V:SetBagPhysics(bag.id,false,55)
this=V.bagAnimationDefault;this.scripts.OnClick();this=oldThis
for _,b in ipairs(V:GetBags()) do assert(b.physics and b.amplitude==100,'Default restores original dynamics without changing placement') end
assert(V:SetCalmCape(true) and capeCalls[#capeCalls]==1)
VanityStudioCharacter.enabled=false;V:SyncAnimations();assert(capeCalls[#capeCalls]==0)
VanityStudioCharacter.enabled=true;this=V.capeAnimationDefault;this.scripts.OnClick();this=oldThis;assert(capeCalls[#capeCalls]==0)
SaureksClosetAnimationVersion=nil;V:RefreshAnimationsPage()
assert(not V.animationBagRows[1].toggle.enabled and not V.calmCapeCheckbox.enabled)
assert(not V.animationBagRows[1].slider.mouseEnabled)
local oldAmount=bag.amplitude;V.animationBagRows[1].slider:SetValue(175)
assert(bag.amplitude==oldAmount,'Old DLL cannot receive slider edits')
assert(not V:SetBagPhysics(bag.id,true,100))
for _,window in ipairs({V.bagAnimationWindow,V.capeAnimationWindow}) do
    assert(window.close.anchor[4]==-46 and window.close.anchor[5]==-24,'Animation windows share corrected close alignment')
end
print('PASS: per-bag amplitude/disable, saved settings, built-in fits, tuning independence, cape opt-in, old-DLL gating and shared window alignment')

local styles={}
SaureksClosetAnimationVersion=function() return 2 end
SaureksClosetSetAnimationStyles=function(...) table.insert(styles,{...});return 1 end
V:RefreshAnimationsPage()
assert(V:SetAnimationStyle('body',11,true));assert(styles[#styles][1]==11 and styles[#styles][2]==0)
assert(V:SetAnimationStyle('cape',4,true));assert(styles[#styles][1]==11 and styles[#styles][2]==4)
assert(V.animationStyleRows.body.race.caption.text=='Tauren' and V.animationStyleRows.cape.sex.caption.text=='Female')
assert(V:SetAnimationStyle('body',11,false));assert(styles[#styles][1]==0 and styles[#styles][2]==4)
assert(V:SetAnimationStyle('body',7,true));assert(V:SetCalmCape(true));assert(styles[#styles][1]==7 and styles[#styles][2]==0 and styles[#styles][3]==1)
V:ResetAnimationStyle('cape');assert(styles[#styles][1]==7 and styles[#styles][2]==0 and styles[#styles][3]==0)
V:ResetAnimationStyle('body');assert(styles[#styles][1]==0 and styles[#styles][2]==0)
assert(not V:SetAnimationStyle('body',17,true) and not V:SetAnimationStyle('cape',0/0,true) and not V:SetAnimationStyle('unknown',1,true))
-- Real menu callbacks must be anchored to a named frame; custom buttons have no native label.
local menuEntries={};UIDropDownMenu_AddButton=function(info) table.insert(menuEntries,info) end
this=V.animationStyleRows.body.race;this.scripts.OnClick();assert(#menuEntries==8 and V.animationStyleMenu.anchor)
menuEntries[2].func();assert(VanityStudioCharacter.bodyAnimationEnabled)
menuEntries={};this=V.animationStyleRows.cape.sex;this.scripts.OnClick();assert(#menuEntries==2);menuEntries[1].func()
assert(VanityStudioCharacter.capeAnimationEnabled and VanityStudioCharacter.bodyAnimationEnabled)
VanityStudioCharacter.enabled=false;V:SyncAnimations();assert(styles[#styles][1]==0 and styles[#styles][2]==0)
VanityStudioCharacter.enabled=true
for _,window in ipairs({V.bodyAnimationWindow,V.capeAnimationWindow}) do assert(window.close.anchor[4]==-46 and window.close.anchor[5]==-24) end
print('PASS: independent cape/body toggles, race/gender menus, named anchors, original defaults, calmer option and addon disable')
