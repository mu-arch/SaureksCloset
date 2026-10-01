-- Exercise character preferences, the native bridge and the fixed Physics page.
table.getn=table.getn or function(t) return #t end
math.mod=math.mod or math.fmod
local checks=0
local function check(value,message) assert(value,message);checks=checks+1 end
local methods={}
local function node(kind,name,parent)
    local n=setmetatable({kind=kind,name=name,parent=parent,children={},scripts={},shown=true},{__index=methods})
    if parent then table.insert(parent.children,n) end
    if name then _G[name]=n end
    return n
end
function CreateFrame(kind,name,parent,template)
    local n=node(kind,name,parent)
    if template=="UIPanelButtonTemplate2" then
        for _,part in ipairs({"Text","Left","Middle","Right"}) do node("Texture",name..part,n) end
    end
    return n
end
function methods:CreateTexture(name) return node("Texture",name,self) end
function methods:CreateFontString(name) return node("FontString",name,self) end
function methods:SetPoint(...) self.anchor={...} end
function methods:SetAllPoints(parent) self.allPoints=parent or self.parent end
function methods:SetWidth(value) self.width=value end
function methods:SetHeight(value) self.height=value end
function methods:GetName() return self.name end
function methods:GetFrameLevel() return self.level or 1 end
function methods:SetFrameLevel(value) self.level=value end
function methods:SetScript(event,callback) self.scripts[event]=callback end
function methods:SetText(value) self.text=value end
function methods:GetText() return self.text or "" end
function methods:SetChecked(value) self.checked=value end
function methods:GetChecked() return self.checked end
function methods:Enable() self.enabled=true end
function methods:Disable() self.enabled=false end
function methods:Show() self.shown=true end
function methods:Hide() self.shown=false end
function methods:IsShown() return self.shown end
function methods:IsVisible() return self.shown and (not self.parent or self.parent:IsVisible()) end
local function noop() end
for _,name in ipairs({"RegisterEvent","SetFont","ClearAllPoints","SetJustifyH","SetJustifyV","SetBackdrop","SetBackdropBorderColor","SetBackdropColor","SetVertexColor","SetTexCoord","SetAlpha","SetTextColor","SetHighlightTexture","EnableMouse","SetTexture","SetBlendMode","SetHitRectInsets","SetFrameStrata","SetClampedToScreen","SetMovable","RegisterForDrag"}) do methods[name]=noop end
function getglobal(name) return _G[name] end
function GetTime() return 100 end
UISpecialFrames={};SlashCmdList={};UIParent=node("Frame","UIParent")
VanityStudioCatalog={}
dofile("addon/SaureksCloset/Core.lua")
dofile("addon/SaureksCloset/Physics.lua")
dofile("addon/SaureksCloset/UI.lua")
local V=VanityStudio
for _,name in ipairs({"InitializeWeapons","RefreshTrueBody","InitializeBagTuning","ArmorLifeState","CheckArmorRepairs","CreateLauncher","InitializeUpdates","CancelDraft","InvalidatePreviewModel","UpdateArmorEquipment","SyncBody","SyncWeapons","UpdateUpdates","UpdatePreviewLoading","UpdateRespawnRecovery","UpdateArmorCheck","RefreshBody","RefreshPortraits","QueueRespawnRecovery","Refresh"}) do V[name]=noop end

local optionWrites={}
SaureksClosetConfigureCapePhysics=function(bags,weapons,weight,stiffness,air)
    table.insert(optionWrites,{bags,weapons,weight,stiffness,air});return 1
end
local writes,status,resets={},0,0
local function setter(value)
    check(value==0 or value==1,"Native enable accepts only numeric 0 or 1")
    table.insert(writes,value);status=value==1 and 1 or 0;return 1
end
SaureksClosetSetCapePhysics=setter
SaureksClosetCapePhysicsStatus=function() return status end
SaureksClosetResetCapePhysics=function() resets=resets+1;return 1 end
V:Initialize()
check(VanityStudioCharacter.physics.cape==false and writes[1]==0,"New characters start off and explicitly disable the native simulation")
V.slotOrder={}
check(V:SetCapePhysics(true) and writes[2]==1 and VanityStudioCharacter.physics.cape,"Enabling persists the per-character preference and starts physics")
local count=table.getn(writes)
V:Sync();V:SyncCapePhysics()
check(table.getn(writes)==count,"Appearance refreshes do not restart an unchanged simulation")
V:SetEnabled(false)
check(writes[table.getn(writes)]==0 and VanityStudioCharacter.physics.cape,"Disabling the addon stops cloth but preserves its preference")
V:SetEnabled(true)
check(writes[table.getn(writes)]==1,"Re-enabling the addon restores the saved cloth preference")
check(not V:CurrentLook().physics and not VanityStudioCharacter.outfitDirty and not VanityStudioCharacter.activeUnsaved,"Physics changes neither saved looks nor their dirty state")
VanityStudioDB.outfits.Plain={version=3,slots={}}
V.NormalizeWeapons=function(self,weapons) return self:Copy(weapons) end
V.MigrateEquippedWeaponOverrides=noop
check(V:LoadOutfit("Plain",true) and V:CapePhysicsEnabled(),"Loading a saved look preserves the independent physics preference")
V:Initialize()
check(VanityStudioCharacter.physics.cape and writes[table.getn(writes)]==1,"Initialization restores an explicitly enabled saved preference")
count=table.getn(writes);event="PLAYER_ENTERING_WORLD";V.events.scripts.OnEvent()
check(table.getn(writes)==count+1 and writes[table.getn(writes)]==1,"World entry reapplies the saved preference")

V.frame=CreateFrame("Frame","PhysicsTest",UIParent);V.frame.width=384;V.frame.height=512
local p=CreateFrame("Frame",nil,V.frame);p:SetAllPoints(V.frame)
V.pagesByName={physics=p};V:CreatePhysicsPage(p)
check(V.physicsCategoryButtons[1].enabled and not V.physicsCategoryButtons[2].enabled and not V.physicsCategoryButtons[3].enabled,"Only Cape configuration is active")
check(not V.capePhysicsWindow:IsShown(),"Cape settings open separately, not on the landing page")
V.physicsCategoryButtons[1].scripts.OnClick()
check(V.capePhysicsWindow:IsShown() and V.capePhysicsCheckbox.parent==V.physicsCapePanel,"Cape opens its own window")
check(not V.capeBagCollision:GetChecked() and V.capeWeaponCollision:GetChecked(),"Bags default off; weapon collisions default on")
local optionCount=table.getn(optionWrites);V:SyncCapePhysics()
check(table.getn(optionWrites)==optionCount,"Unchanged collision/tuning preferences do not restart cloth")
this=V.capeBagCollision;this:SetChecked(1);this.scripts.OnClick();this=nil
check(VanityStudioCharacter.physics.capeBags and optionWrites[table.getn(optionWrites)][1]==1,"Bag checkbox enables native cape-to-bag collisions")
check(V:SetCapeOption("capeWeapons",false) and optionWrites[table.getn(optionWrites)][2]==0,"Weapon collisions toggle independently")
V.capeTuningControls.capeWeight.plus.scripts.OnClick()
check(VanityStudioCharacter.physics.capeWeight==1.25 and optionWrites[table.getn(optionWrites)][3]==1.25,"Weight control updates actual native settings")
check(V:SetCapeOption("capeWeight",99) and VanityStudioCharacter.physics.capeWeight==3 and not V.capeTuningControls.capeWeight.plus.enabled,"Weight is bounded and its upper button disables")
check(not V:SetCapeOption("unrelated",1) and not V:SetCapeOption("capeWeight",0/0),"Invalid option names and nonfinite values are rejected")
local configure=SaureksClosetConfigureCapePhysics
SaureksClosetConfigureCapePhysics=function() return 0 end
check(not V:SetCapeOption("capeWeight",1) and VanityStudioCharacter.physics.capeWeight==3,"Rejected native changes restore the displayed value")
SaureksClosetConfigureCapePhysics=configure
check(V:RestoreCapeDefaults() and VanityStudioCharacter.physics.capeWeight==1 and VanityStudioCharacter.physics.capeBags and not VanityStudioCharacter.physics.capeWeapons,"Defaults restore tuning without overriding collision choices")
V:SetCapeOption("capeBags",false);V:SetCapeOption("capeWeapons",true)
SaureksClosetConfigureCapePhysics=nil;V:RefreshPhysicsPage()
check(not V.capeBagCollision.enabled and not V.capeTuningControls.capeWeight.plus.enabled and V.capePhysicsCheckbox.enabled,"Older DLL disables new settings but preserves enable control")
SaureksClosetConfigureCapePhysics=configure;V:RefreshPhysicsPage()
V.capePhysicsWindow.close.scripts.OnClick();check(not V.capePhysicsWindow:IsShown(),"Close dismisses the separate menu")
V:OpenCapePhysics();p.scripts.OnHide();check(not V.capePhysicsWindow:IsShown(),"Leaving Physics dismisses its menu")
check(V.capePhysicsCheckbox:GetChecked() and V.capePhysicsCheckbox.enabled,"Enabled preference appears checked")
check(V.capePhysicsStatusLabel.text=="Waiting for a visible cape." and not V.resetCapePhysicsButton.enabled,"Waiting status disables reset until there is a cape")
check(not V:ResetCapePhysics() and resets==0,"Reset does nothing without an active cape")
status=2;arg1=.5;V.events.scripts.OnUpdate()
check(V.capePhysicsStatusLabel.text=="Active: NvCloth cape physics." and V.resetCapePhysicsButton.enabled,"The visible page refreshes live runtime status")
check(V:ResetCapePhysics() and resets==1 and VanityStudioCharacter.physics.cape,"Reset restarts motion without changing the saved preference")
status=5;arg1=.5;V.events.scripts.OnUpdate()
check(V.capePhysicsStatusLabel.text=="Preparing NvCloth cape physics..." and V.resetCapePhysicsButton.enabled,"Worker preparation is distinct from invisible cape and can be reset")
status=6;arg1=.5;V.events.scripts.OnUpdate()
check(V.capePhysicsStatusLabel.text=="Recovering cape physics. Holding the last cloth pose." and V.resetCapePhysicsButton.enabled,"A recovery holds physics rather than claiming the normal cape animation is active")
status=4;arg1=.5;V.events.scripts.OnUpdate()
check(V.capePhysicsStatusLabel.text=="Temporarily using normal cape motion. Physics will retry automatically." and V.resetCapePhysicsButton.enabled,"Fit conflicts show the native fallback honestly and permit retry")
local resetsBefore=resets;check(V:ResetCapePhysics() and resets==resetsBefore+1,"Fit fallback can be reset explicitly")
status=3;arg1=.5;V.events.scripts.OnUpdate()
check(V.capePhysicsStatusLabel.text=="Cape physics is unavailable for the current model or renderer." and not V.resetCapePhysicsButton.enabled,"Unsupported models or renderers are explained without a fake active state")
check(V:SetCapePhysics(false) and writes[table.getn(writes)]==0 and not V.capePhysicsCheckbox:GetChecked(),"Turning the page option off disables and persists")
check(V.capePhysicsStatusLabel.text=="Off" and not V:ResetCapePhysics(),"Disabled physics cannot be reset")
this=V.capePhysicsCheckbox;this:SetChecked(1);this.scripts.OnClick();this=nil
check(VanityStudioCharacter.physics.cape and writes[table.getn(writes)]==1,"Checkbox invokes the native enable action")
V:SetEnabled(false);V:RefreshPhysicsPage()
check(V.capePhysicsStatusLabel.text=="Addon is off. Your cape preference is saved." and not V.resetCapePhysicsButton.enabled,"Global-off state is clear and cannot reset")
V:SetCapePhysics(false);V:SetCapePhysics(true)
check(writes[table.getn(writes)]==0,"Changing the preference cannot bypass the addon master toggle")
V:SetEnabled(true)
local savedCharacter=VanityStudioCharacter
VanityStudioCharacter={enabled=true};V:InitializePhysics()
check(not V:CapePhysicsEnabled() and writes[table.getn(writes)]==0,"Another character starts off independently")
VanityStudioCharacter=savedCharacter;V:InitializePhysics()
check(V:CapePhysicsEnabled() and writes[table.getn(writes)]==1,"The original character retains its preference")

SaureksClosetSetCapePhysics=nil;count=table.getn(writes);V:SyncCapePhysics();V:RefreshPhysicsPage()
check(not V.capePhysicsCheckbox.enabled and not V.resetCapePhysicsButton.enabled,"An older renderer disables unsupported controls")
check(string.find(V.capePhysicsStatusLabel.text,"Update SaureksCloset.dll",1,true) and V:CapePhysicsEnabled(),"Missing bridge is explained and never erases the saved preference")
check(not V:SetCapePhysics(false) and not V:ResetCapePhysics() and table.getn(writes)==count,"Unavailable bridge actions do not call native code or mutate preferences")
SaureksClosetSetCapePhysics=setter;V:SyncCapePhysics()
check(table.getn(writes)==count+1 and writes[table.getn(writes)]==1,"Bridge availability restores the saved preference")
SaureksClosetSetCapePhysics=function() error("native failure") end
check(not V:SetCapePhysics(true) and V.capePhysicsError,"Native failures are caught and reported")
SaureksClosetSetCapePhysics=setter;V:SyncCapePhysics();status=2
SaureksClosetResetCapePhysics=function() return 0 end
check(not V:ResetCapePhysics() and V.capePhysicsStatusLabel.text=="Could not reset cape motion. Try again.","Rejected reset gets an actionable status")
SaureksClosetCapePhysicsStatus=function() error("unavailable") end;V:RefreshPhysicsPage()
check(not V.resetCapePhysicsButton.enabled and V:CapePhysicsStatus()==nil,"Failed status calls cannot enable reset")
SaureksClosetSetCapePhysics=function() return 0 end
SaureksClosetCapePhysicsStatus=function() return 3 end
check(not V:SetCapePhysics(true) and V.capePhysicsError,"Optional native hook unavailability rejects enabling without throwing")
check(V.capePhysicsStatusLabel.text=="Cape physics is unavailable for the current model or renderer." and not V.resetCapePhysicsButton.enabled,"Optional hook unavailability takes priority over the generic update error")
V:SetCapePhysics(false)
check(V.capePhysicsStatusLabel.text=="Off","An unsupported renderer does not replace the unchecked preference's off status")
V:SetEnabled(false);V:SetCapePhysics(true)
check(V.capePhysicsStatusLabel.text=="Addon is off. Your cape preference is saved.","An unsupported renderer preserves the master-off status")

local function rect(n)
    if n==V.frame then return 0,0,384,512 end
    if n.allPoints then return rect(n.allPoints) end
    local a=n.anchor;local x,y=rect(a and a[2] or n.parent)
    return x+(a and a[4] or 0),y-(a and a[5] or 0),n.width,n.height
end
local controls={V.physicsTitle,V.physicsScopeLabel}
for _,widget in ipairs(controls) do
    local x,y,w,h=rect(widget)
    check(x>=30 and x+w<=334 and y>=80 and y+h<=392,"Physics content fits between wardrobe navigation without scrolling")
end
local textRegions={V.capePhysicsLabel,V.capePhysicsStatusLabel,V.physicsScopeLabel}
for _,region in ipairs(textRegions) do
    local columns=math.floor(region.width/6)
    local lines=math.max(1,math.ceil(string.len(region.text)/columns))
    check(lines*12<=region.height,"Reserved text height accommodates wrapped small-font copy")
end
local function fixed(frame)
    check(frame.kind~="ScrollFrame" and not frame.scripts.OnMouseWheel,"Physics has no scrolling controls")
    for _,child in ipairs(frame.children) do fixed(child) end
end
fixed(p)

-- Use the real selector and page switcher; only unrelated wardrobe editors are stubbed.
for _,name in ipairs({"CloseWeaponOptions","CloseOutfitMenu","CloseOutfitDetails","CloseBrowser","FrameBodyPreview","HidePreviewUntilReady"}) do V[name]=noop end
for _,name in ipairs({"armor","body","weaponry","bags","exposure","outfits","settings"}) do
    V.pagesByName[name]=CreateFrame("Frame",nil,V.frame)
end
V.tabButtons={}
for _,name in ipairs({"character","outfits","settings"}) do V.tabButtons[name]=CreateFrame("Button",nil,V.frame) end
function PanelTemplates_SelectTab(frame) frame.selected=true end
function PanelTemplates_DeselectTab(frame) frame.selected=false end
V.armorDecorationFrame=CreateFrame("Frame",nil,V.pagesByName.armor)
V.armorShadowFrame=CreateFrame("Frame",nil,V.pagesByName.armor)
V.bodyPreviewFade=CreateFrame("Frame",nil,V.pagesByName.armor)
V.rotationControls=CreateFrame("Frame",nil,V.pagesByName.armor)
V.model=CreateFrame("Model",nil,V.pagesByName.armor)
V.previewBuffer=CreateFrame("Model",nil,V.pagesByName.armor)
V:CreateWardrobeSelector()
local choices={}
function UIDropDownMenu_AddButton(info) table.insert(choices,info) end
V.wardrobeMenu.initialize()
check(table.getn(choices)==6 and choices[6].text=="Physics","Physics is a real entry in the existing wardrobe navigation")
choices[6].func()
check(V.tab=="physics" and p:IsVisible() and not V.model:IsVisible(),"Selecting Physics opens the full page and hides the character preview")
check(V.tabButtons.character.selected and V.wardrobeSelectorLabel.text=="Physics" and V.wardrobeSelectorBox.width==176,"Physics keeps Wardrobe selected and uses its wide page selector")
check(not V.rotationControls:IsVisible() and not V.armorDecorationFrame:IsVisible(),"Physics has no unrelated outfit controls")
V:SetTab("settings");V:SetTab("character")
check(V.tab=="physics" and p:IsVisible(),"Returning to Wardrobe restores the Physics page")
V:SetTab("armor")
check(not p:IsVisible() and V.pagesByName.armor:IsVisible(),"Leaving Physics hides all its controls")
print("PASS: "..checks.." cape physics preference, bridge, lifecycle and fixed page checks")
