-- Drive the real wardrobe Save/Saved Looks controls and saved-look state machine.
-- Game rendering is stubbed; saving, copying, edit tracking and button callbacks are not.
table.getn=table.getn or function(t) return #t end
math.mod=math.mod or math.fmod
local checks=0
local function check(value,message) assert(value,message);checks=checks+1 end
local function noop() end
local frames,methods={},{}
local function script(frame,event)
    if frame.scripts[event] then local old=this;this=frame;frame.scripts[event]();this=old end
end
function CreateFrame(kind,name,parent,template)
    local frame=setmetatable({kind=kind,name=name,parent=parent,scripts={},shown=true}, {__index=methods})
    table.insert(frames,frame);if name then _G[name]=frame end
    if template=="UIPanelButtonTemplate2" or template=="CharacterFrameTabButtonTemplate" then
        for _,part in ipairs({"Text","Left","Middle","Right"}) do CreateFrame("Texture",name..part,frame) end
    elseif template=="UIPanelScrollBarTemplate" then
        for _,part in ipairs({"ThumbTexture","ScrollUpButton","ScrollDownButton"}) do CreateFrame("Button",name..part,frame) end
    end
    return frame
end
function methods:CreateTexture(name) return CreateFrame("Texture",name,self) end
function methods:CreateFontString(name) return CreateFrame("FontString",name,self) end
function methods:SetScript(event,fn) self.scripts[event]=fn end
function methods:GetScript(event) return self.scripts[event] end
function methods:GetName() return self.name end
function methods:GetParent() return self.parent end
function methods:SetText(text) self.text=text;script(self,"OnTextChanged") end
function methods:GetText() return self.text or "" end
function methods:GetTextWidth() return string.len(self:GetText())*6 end
function methods:SetWidth(value) self.width=value end
function methods:SetHeight(value) self.height=value end
function methods:GetWidth() return self.width or 12 end
function methods:GetHeight() return self.height or 22 end
function methods:SetTexture(value) self.texture=value end
function methods:Show() self.shown=true end
function methods:Hide() self.shown=false end
function methods:IsShown() return self.shown end
function methods:IsVisible() return self.shown and (not self.parent or self.parent:IsVisible()) end
function methods:Enable() self.disabled=false end
function methods:Disable() self.disabled=true end
function methods:EnableMouse(value) self.mouseEnabled=value end
function methods:SetFocus() self.focused=true;script(self,"OnEditFocusGained") end
function methods:ClearFocus() self.focused=false;script(self,"OnEditFocusLost") end
function methods:HighlightText() self.highlighted=true end
function methods:GetFrameLevel() return 1 end
function methods:GetEffectiveScale() return 1 end
function methods:SetRotation(value) self.rotation=value end
for _,name in ipairs({"RegisterEvent","SetPoint","ClearAllPoints","SetAllPoints","SetFont","SetJustifyH","SetJustifyV","SetTextColor","SetFrameStrata","SetFrameLevel","SetMovable","SetClampedToScreen","RegisterForDrag","SetHighlightTexture","SetHitRectInsets","SetBackdrop","SetBackdropBorderColor","SetBackdropColor","SetTexCoord","SetBlendMode","SetAlpha","SetAutoFocus","SetMaxLetters","SetVertexColor","SetValueStep","EnableMouseWheel"}) do methods[name]=noop end
function getglobal(name) return _G[name] end
function PanelTemplates_TabResize() end
function time() return 123456 end
UIParent=CreateFrame("Frame","UIParent")
UISpecialFrames={};UIPanelWindows={};SlashCmdList={}
GameTooltip={SetOwner=noop,Hide=noop,Show=noop,AddLine=noop,SetText=function(self,value) self.text=value end}
SaureksClosetWeaponAssets={};SaureksClosetQuivers={}
dofile("addon/SaureksCloset/Core.lua")
dofile("addon/SaureksCloset/BagCatalog.lua")
dofile("addon/SaureksCloset/Weaponry.lua")
dofile("addon/SaureksCloset/Bags.lua")
dofile("addon/SaureksCloset/Preview.lua")
dofile("addon/SaureksCloset/UI.lua")
local V=VanityStudio
local function copy(value) return V:Copy(value) end
local function equal(a,b)
    if type(a)~=type(b) then return false end
    if type(a)~="table" then return a==b end
    for key,value in pairs(a) do if not equal(value,b[key]) then return false end end
    for key in pairs(b) do if a[key]==nil then return false end end
    return true
end
local function click(frame)
    check(frame and not frame.disabled,"Control is available")
    script(frame,"OnClick")
end
local syncs=0
for _,name in ipairs({"CreateArmorPage","CreateBodyPage","CreateWeaponryPage","CreateBagsPage","CreateExposurePage","CreateSettingsPage","CreateBrowser","CreateOutfitDetails","CreateWardrobeSelector","InvalidatePreviewModel","RefreshOutfitDetails","CloseOutfitMenu","CloseOutfitDetails","CloseBrowser","CloseBagModelPicker","CloseWeaponOptions","DiscardBagDrafts"}) do V[name]=noop end
function V:NativeBody() return {race=1,sex=0} end
function V:NormalizeBody(body) return copy(body) end
function V:BodyAvailable() return true end
function V:SyncBody() return true end
function V:WeaponRendererAvailable() return true end
function V:MultiBagRendererAvailable() return true end
function V:BagRendererAvailable() return true end
function V:Sync() syncs=syncs+1 end
function V:CancelDraft() self.draft=nil end
function V:Message(message) self.lastMessage=message end
function V:Refresh()
    if self.activeOutfitLabel then self.activeOutfitLabel:SetText(self:ActiveOutfitText()) end
    self:RefreshWardrobeSaveButton()
    self:RefreshUnsavedLookPanel()
    if self.unsavedLookPanel then
        if VanityStudioCharacter.activeUnsaved then self.unsavedLookPanel:Show() else self.unsavedLookPanel:Hide() end
    end
end
V.RefreshOutfits=V.Refresh
function V:SetTab(tab)
    self.tab=tab
    for name,page in pairs(self.pagesByName or {}) do if name==tab then page:Show() else page:Hide() end end
    self:Refresh()
end
local function reset(saved,current)
    VanityStudioDB={outfits=copy(saved or {})}
    VanityStudioCharacter=copy(current or {enabled=true,selected={},weapons={}})
    V.pendingLook=nil;V.pendingLookChange=nil;V.lastMessage=nil;V.unsavedLookMessage=nil;V.bodyError=nil;V.draft=nil;V.bagTunerDrafts=nil
    if V.unsavedOutfitName then V.unsavedOutfitName:SetText("");V.unsavedOutfitName:ClearFocus() end
    V:SetTab("armor")
end
reset()
V:CreateUI()
V.outfitMessage=CreateFrame("FontString")
V.outfitDetails=CreateFrame("Frame");V.outfitDetails:Hide()
local save
for _,frame in ipairs(frames) do if frame.kind=="Button" and frame.parent==V.frame and frame:GetText()=="Save" then save=frame end end
check(save,"The wardrobe header contains a Save button")
local function saveEnabled(expected,message)
    check(save.disabled==not expected and save.closetEnabled==expected,message)
    for _,part in ipairs({"Left","Middle","Right"}) do
        check(getglobal(save:GetName()..part).texture=="Interface\\Buttons\\UI-Panel-Button-"..(expected and "Up" or "Disabled"),message..": native "..part.." artwork matches")
    end
end
saveEnabled(false,"Save starts disabled before any edits")
local initialState=copy(VanityStudioCharacter)
script(save,"OnClick")
check(not V:SaveWardrobeLook() and equal(VanityStudioCharacter,initialState) and V.tab=="armor","A disabled Save callback cannot create an empty draft or navigate")
script(save,"OnEnter")
check(GameTooltip.text=="Save your latest changes to your active saved look","Save has the requested tooltip")

local first={version=3,updatedAt=10,slots={[1]=10500,[3]=0},weapons={independent=true,[101]=200,bags={{id=1,model=2,mount="back",fits={["1:0"]={left=.2,inset=0,up=0,pitch=0,roll=0,yaw=0,scale=85}}}}},body={race=1,sex=0,hair=3}}
local second={version=3,updatedAt=9,slots={[1]=10501},weapons={independent=true,bags={}},body={race=2,sex=1,hair=2}}
V.index[10500]={10500,"Goggles",1};V.index[10501]={10501,"Helmet",1}
local function startEdited(enabled)
    reset({Traveler=first,Warrior=second},{enabled=enabled~=false,selected=copy(first.slots),weapons=copy(first.weapons),body=copy(first.body),activeOutfit="Traveler"})
    local c=VanityStudioCharacter
    c.selected[1]=10501;V:TrackUnsaved();V:Refresh()
    return c
end
local c=startEdited()
saveEnabled(true,"Armor edits enable Save")
check(V:ActiveSavedLookName()=="Traveler","An edit remembers its active saved look")
check(V:ActiveOutfitText()=="Traveler (Edited)","Edited appearance retains the original name")
c.body.hair=7;c.weapons.bags[1].fits["1:0"].left=.45;c.weapons.bags[1].mount="rightHip";V:TrackUnsaved()
check(c.unsaved.baseName=="Traveler" and V:ActiveOutfitText()=="Traveler (Edited)","Repeated edits retain one original-name suffix")
check(equal(VanityStudioDB.outfits.Traveler,first),"Editing leaves the saved look unchanged")
local beforeOther=copy(VanityStudioDB.outfits.Warrior)
-- Save must read the latest applied configuration, even if the cached draft is older.
c.body.hair=8;c.weapons.bags[1].fits["1:0"].left=.55
local latest=V:CurrentLook();local beforeSyncs=syncs
click(save)
check(c.activeOutfit=="Traveler" and not c.activeUnsaved and not c.outfitDirty and not c.unsaved,"Save clears the edited state")
check(V:ActiveOutfitText()=="Traveler" and V.tab=="armor","Save preserves the active name and wardrobe page")
check(equal(VanityStudioDB.outfits.Traveler.slots,latest.slots) and equal(VanityStudioDB.outfits.Traveler.body,latest.body) and equal(VanityStudioDB.outfits.Traveler.weapons,latest.weapons),"Save writes current armor, body, weapons, bag mounts and independent fits")
check(equal(VanityStudioDB.outfits.Warrior,beforeOther),"Save updates only the active look")
check(syncs==beforeSyncs,"Saving does not reapply the appearance")
saveEnabled(false,"Saving returns the header to its disabled state")
c.weapons.bags[1].fits["1:0"].left=.65;c.body.hair=9
check(VanityStudioDB.outfits.Traveler.weapons.bags[1].fits["1:0"].left==.55 and VanityStudioDB.outfits.Traveler.body.hair==8,"Saved nested data is detached from live edits")

c=startEdited(false);click(save)
check(c.enabled==false and V:ActiveOutfitText()=="Traveler (off)","Save keeps a disabled addon disabled")
check(save:GetText()=="Save","The header action never becomes Enable or Disable")
c=startEdited();check(V:RenameOutfit("Traveler","Explorer"))
check(V:ActiveSavedLookName()=="Explorer" and V:ActiveOutfitText()=="Explorer (Edited)","Renaming the base updates the edited look name")
click(save)
check(not VanityStudioDB.outfits.Traveler and c.activeOutfit=="Explorer","Save follows a renamed base without restoring the old name")

local function expectNewLook(message)
    local old=copy(VanityStudioDB.outfits);local wasEnabled=VanityStudioCharacter.enabled
    click(save)
    check(V.tab=="outfits" and V.pagesByName.outfits:IsShown(),message..": opens Saved Looks")
    check(V.unsavedLookPanel:IsShown() and V.unsavedOutfitName.focused and V.unsavedOutfitName.mouseEnabled,message..": offers and focuses the new name")
    check(equal(VanityStudioDB.outfits,old) and VanityStudioCharacter.enabled==wasEnabled,message..": does not overwrite a saved look or enable the addon")
    local current=V:CurrentLook()
    V.unsavedOutfitName:SetText("New adventure")
    check(V.saveUnsavedAsNewButton.closetEnabled,message..": Save New becomes available")
    click(V.saveUnsavedAsNewButton)
    local look=VanityStudioDB.outfits["New adventure"]
    check(look and equal(look.slots,current.slots) and equal(look.weapons,current.weapons) and equal(look.body,current.body),message..": Save New stores the current complete look")
    check(VanityStudioCharacter.activeOutfit=="New adventure" and not VanityStudioCharacter.activeUnsaved,message..": new saved look becomes active")
    saveEnabled(false,message..": Save becomes disabled after Save New")
end
reset();V:Select(1,10500);expectNewLook("First use with no saved look")
reset({Warrior=second},{enabled=false,selected=copy(first.slots),weapons=copy(first.weapons),body=copy(first.body)})
V:Select(1,10501)
expectNewLook("No active saved look even when other saved looks exist")
c=startEdited();check(V:DeleteOutfit("Traveler"));check(not V:ActiveSavedLookName(),"A deleted base is no longer an active saved look")
expectNewLook("Edited look whose saved base was deleted")
reset({Warrior=second},{enabled=true,selected=copy(first.slots),weapons=copy(first.weapons),activeOutfit="Missing"})
V:Select(1,10501)
expectNewLook("Stale selected name does not recreate a deleted look")

-- Body/bag edits are changes, while rotating only the preview is not.
reset({Traveler=first},{enabled=true,selected=copy(first.slots),weapons=copy(first.weapons),body=copy(first.body),activeOutfit="Traveler"})
saveEnabled(false,"An unchanged active look keeps Save disabled")
V.model=CreateFrame("PlayerModel");V.previewBuffer=CreateFrame("PlayerModel")
local cursorX=100
function GetCursorPosition() return cursorX,0 end
local drag=V:CreatePreviewDragSurface(V.frame,"model","previewBuffer")
arg1="LeftButton";script(drag,"OnMouseDown");cursorX=135;script(drag,"OnUpdate");script(drag,"OnMouseUp");V:Refresh()
check(V.model.rotation==V.previewBuffer.rotation and V.model.rotation~=.61,"The real drag handler rotates the preview models")
saveEnabled(false,"Preview rotation alone does not enable Save")
c=VanityStudioCharacter;c.body.hair=5;V:TrackUnsaved();V:Refresh()
saveEnabled(true,"Body edits enable Save")
click(save);saveEnabled(false,"Saving body edits disables Save")
c.weapons.bags[1].model=5;V:TrackUnsaved();V:Refresh()
saveEnabled(true,"Changing a bag model enables Save")
click(save);saveEnabled(false,"Saving bag model changes disables Save")

-- Header Save also captures a currently previewed item and tuner changes.
reset({Traveler=first},{enabled=true,selected=copy(first.slots),weapons=copy(first.weapons),body=copy(first.body),activeOutfit="Traveler"})
c=VanityStudioCharacter;V.draft={slot=1,id=10501};V:Refresh()
check(V:ActiveOutfitText()=="Traveler (Edited)","A live item preview marks its active saved look as edited")
saveEnabled(true,"An item preview enables Save before it is committed")
V:CancelDraft();V:Refresh();saveEnabled(false,"Canceling the only preview edit disables Save")
V.draft={slot=1,id=10501};V:Refresh()
click(save)
check(not V.draft and c.selected[1]==10501 and VanityStudioDB.outfits.Traveler.slots[1]==10501,"Header Save commits the previewed item before writing the active look")
check(not c.activeUnsaved and c.activeOutfit=="Traveler","Saving a preview finishes with a clean active look")
local bag=c.weapons.bags[1];local fit=copy(bag.fits["1:0"]);fit.up=.31;fit.scale=110
V.bagTunerDrafts={[V:BagDraftKey(bag,1,0)]=fit}
V:Refresh()
check(V:HasUnsavedBagFits(),"Changed live tuner values count as unsaved")
check(V:ActiveOutfitText()=="Traveler (Edited)","A live bag tuner edit marks its active saved look as edited")
saveEnabled(true,"A live bag fit enables Save before Save Fit is pressed")
local fitKey=V:BagDraftKey(bag,1,0)
V.bagTunerDrafts[fitKey]=copy(bag.fits["1:0"]);V:Refresh()
saveEnabled(false,"Returning the only bag draft to its saved fit disables Save")
V.bagTunerDrafts[fitKey]=fit;V:Refresh()
click(save)
check(bag.fits["1:0"].up==.31 and VanityStudioDB.outfits.Traveler.weapons.bags[1].fits["1:0"].scale==110,"Header Save stores the active bag's live placement draft")
check(not V:HasUnsavedBagFits() and not c.activeUnsaved,"Saving fit drafts clears their unsaved status")
check(V:ActiveOutfitText()=="Traveler","Saving fit drafts removes the edited suffix")
fit.up=.42
check(bag.fits["1:0"].up==.31 and VanityStudioDB.outfits.Traveler.weapons.bags[1].fits["1:0"].up==.31,"Tuner draft, live saved fit and saved look are independent copies")

c=startEdited();bag=c.weapons.bags[1];fit=copy(bag.fits["1:0"]);fit.up=.37
V.bagTunerDrafts={[V:BagDraftKey(bag,1,0)]=fit}
V:SetTab("outfits");V.unsavedOutfitName:SetText("Tuned copy");click(V.saveUnsavedAsNewButton)
check(VanityStudioDB.outfits["Tuned copy"].weapons.bags[1].fits["1:0"].up==.37,"Save New also includes live bag fitting when reached without the header Save")
check(VanityStudioDB.outfits.Traveler.weapons.bags[1].fits["1:0"].up==0,"Save New preserves the original saved look's fit")
c=startEdited();bag=c.weapons.bags[1];fit=copy(bag.fits["1:0"]);fit.up=.48
V.bagTunerDrafts={[V:BagDraftKey(bag,1,0)]=fit}
V:ReplaceWithUnsaved("Traveler")
check(VanityStudioDB.outfits.Traveler.weapons.bags[1].fits["1:0"].up==.48 and c.activeOutfit=="Traveler" and not c.activeUnsaved,"Update Existing includes live fitting newer than the cached unsaved look")
check(not V:HasUnsavedBagFits(),"Update Existing saves and clears the live tuner draft state")

-- Copying an inactive saved look must not absorb edits to the active look.
c=startEdited();local originalSecond=copy(VanityStudioDB.outfits.Warrior)
V.draft={slot=1,id=10500};local preview=copy(V.draft)
check(V:SaveOutfit("Warrior copy",false,"Warrior"),"An inactive saved look can be copied")
check(equal(VanityStudioDB.outfits["Warrior copy"].slots,originalSecond.slots) and equal(VanityStudioDB.outfits["Warrior copy"].body,originalSecond.body),"Inactive look copies keep their own saved data")
check(c.activeUnsaved and c.unsaved.baseName=="Traveler" and equal(V.draft,preview),"Saving an inactive look does not commit or retarget active edits")

-- The same loading gateway protects direct selections, quick selection and cycling.
local prompts={}
local actualConfirmLookChange=V.ConfirmLookChange
function V:ConfirmLookChange(name)
    self.pendingLook=name;table.insert(prompts,name);return false
end
c=startEdited(false)
local before=copy(c);local target=copy(VanityStudioDB.outfits.Warrior);local promptCount=table.getn(prompts)
check(not V:LoadOutfit("Warrior"),"Loading another look defers while there are unsaved edits")
check(V.pendingLook=="Warrior" and table.getn(prompts)==promptCount+1 and equal(c,before),"A deferred change preserves every current field")
V.pendingLook=nil -- Dismissing the dialog must not call the commit continuation.
check(equal(c,before) and equal(VanityStudioDB.outfits.Warrior,target),"Cancel preserves both current and saved looks")
check(not V:LoadOutfit("Missing") and not V.pendingLook,"Missing targets cannot prompt or discard changes")
V:LoadOutfit("Warrior")
local nextLook=V.pendingLook;V.pendingLook=nil
check(V:LoadOutfit(nextLook,true),"Confirming explicitly permits the pending change")
check(c.activeOutfit=="Warrior" and not c.activeUnsaved and c.enabled==false,"Direct confirmed loading replaces the look without changing addon enablement")
check(c.selected[1]==10501 and c.body.race==2,"Confirmed loading applies the selected saved appearance")

c=startEdited(false);before=copy(c);V:ActivateSavedOutfit("Warrior")
check(V.pendingLook=="Warrior" and equal(c,before),"Quick selection also waits without enabling the addon")
check(not V.outfitMessage:GetText() or V.outfitMessage:GetText()=="","Ordinary confirmation is not reported as a renderer failure")
c=startEdited();V:CycleOutfit(1)
check(V.pendingLook~=nil and c.activeUnsaved and c.unsaved.baseName=="Traveler","Minimap cycling cannot silently discard unsaved edits")
reset({Traveler=first,Warrior=second},{enabled=true,selected={},weapons={}})
check(V:LoadOutfit("Warrior") and not V.pendingLook,"Loading with no unsaved changes needs no confirmation")
saveEnabled(false,"Loading an unchanged saved look disables Save")

-- Exercise the production confirmation popup, including its continuation intent.
V.ConfirmLookChange=actualConfirmLookChange
StaticPopupDialogs={}
local popup,showFailure
function StaticPopup_Show(key,name)
    if showFailure then return nil end
    popup={key=key,name=name,info=StaticPopupDialogs[key]};return popup
end
function StaticPopup_Hide(key)
    if popup and popup.key==key then local old=popup;popup=nil;if old.info.OnCancel then old.info.OnCancel() end end
end
local function acceptPopup()
    local old=popup;popup=nil;check(old~=nil,"Discard confirmation is visible");old.info.OnAccept()
end
local function cancelPopup()
    local old=popup;popup=nil;check(old~=nil,"Cancel confirmation is visible");old.info.OnCancel()
end
c=startEdited(false);before=copy(c)
V:ActivateSavedOutfit("Warrior")
check(popup and popup.name=="Warrior" and popup.info.button1=="Discard Changes" and popup.info.button2=="Cancel","The switch warning names the destination and offers explicit discard/cancel")
check(popup.info.hideOnEscape==1 and V.pendingLookChange.activate==true,"Escape can cancel and activation intent is retained")
cancelPopup()
check(not V.pendingLookChange and equal(c,before),"Actual Cancel callback leaves appearance, saved draft and enablement untouched")
V:ActivateSavedOutfit("Warrior");acceptPopup()
check(c.activeOutfit=="Warrior" and c.enabled and not c.unsaved and not c.activeUnsaved,"Actual Discard callback applies and activates the requested look once")
check(not V.pendingLookChange,"Accept clears the pending continuation")
c=startEdited(false);V:LoadOutfit("Warrior");acceptPopup()
check(c.activeOutfit=="Warrior" and c.enabled==false and not c.unsaved,"A direct/cycled switch preserves disabled state through the popup")
c=startEdited();V:LoadOutfit("Warrior");click(save)
check(not popup and not V.pendingLookChange and c.activeOutfit=="Traveler","Saving the draft dismisses an outdated switch warning")
c=startEdited();before=copy(c);V:LoadOutfit("Warrior");VanityStudioDB.outfits.Warrior=nil;acceptPopup()
check(equal(c,before),"A destination removed while confirmation is open cannot discard current work")
c=startEdited();before=copy(c);showFailure=true
V:LoadOutfit("Warrior");showFailure=false
check(not V.pendingLookChange and equal(c,before),"Unavailable popup capacity cannot silently switch looks")
c=startEdited(false);V:LoadOutfit("Warrior");V.BodyAvailable=function() return false end
before=copy(c);acceptPopup()
check(equal(c,before),"A missing body renderer retains the current look and its saved draft")
V.BodyAvailable=function() return true end

-- A temporary appearance is still work worth preserving if a new look fails to load.
reset({Traveler=first,Warrior=second},{enabled=true,selected=copy(first.slots),weapons=copy(first.weapons),body=copy(first.body),activeOutfit="Traveler"})
c=VanityStudioCharacter;V.draft={slot=1,id=10501};local itemDraft=copy(V.draft)
V:LoadOutfit("Warrior");check(popup~=nil,"Uncommitted item previews also require confirmation")
V.BodyAvailable=function() return false end;before=copy(c);acceptPopup()
check(equal(c,before) and equal(V.draft,itemDraft),"A failed confirmed switch preserves the temporary item appearance")
V.BodyAvailable=function() return true end

print("PASS wardrobe save: "..checks.." checks")
