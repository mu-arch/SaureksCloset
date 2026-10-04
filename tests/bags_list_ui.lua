-- Exercise the real eight-bag list and tuner entry with 1.12-shaped frames.
-- This verifies visibility, interaction and geometry, not game font rendering.
table.getn=table.getn or function(t) return #t end
math.mod=math.mod or math.fmod
VanityStudio={slotButtons={},slotNames={},index={}}
VanityStudioCharacter={enabled=true,weapons={independent=true,carriedEnabled=false}}
SaureksClosetSetWeapons=function() return true end
SaureksClosetRendererVersion=function() return 30704 end
UISpecialFrames={}
UIMenus={}
local frames={}
local methods={}
local function node(kind,name,parent)
    local n=setmetatable({kind=kind,name=name,parent=parent,scripts={},shown=true}, {__index=methods})
    table.insert(frames,n)
    if name then _G[name]=n end
    return n
end
function CreateFrame(kind,name,parent,template)
    local frame=node(kind,name,parent)
    if template=="UIPanelButtonTemplate2" or template=="CharacterFrameTabButtonTemplate" then
        node("FontString",name.."Text",frame)
        for _,part in ipairs({"Left","Middle","Right"}) do
            local region=node("Texture",name..part,frame)
            region.width=12;region.height=22
            region.anchor={part=="Right" and "RIGHT" or "LEFT",frame,part=="Right" and "RIGHT" or "LEFT",0,0}
            region.texture={"Interface\\Buttons\\UI-Panel-Button-Up"}
        end
    end
    return frame
end
function getglobal(name) return _G[name] end
function setglobal(name,value) _G[name]=value end
function PanelTemplates_SelectTab(b) b.selected=true end
function PanelTemplates_DeselectTab(b) b.selected=false end
function methods:CreateTexture(name) return node("Texture",name,self) end
function methods:CreateFontString(name) return node("FontString",name,self) end
function methods:SetPoint(point,owner,relative,x,y) self.anchor={point,owner,relative,x,y} end
function methods:ClearAllPoints() self.anchor=nil;self.allPoints=nil end
function methods:SetWidth(value) self.width=value end
function methods:SetHeight(value) self.height=value end
local function estimatedTextWidth(text,fontSize)
    -- Approximate glyph widths for frame geometry, not a font-rendering test.
    return string.len(text or "")*(fontSize or 10)*.5
end
function methods:GetWidth()
    if self.kind=="FontString" and (not self.width or self.width==0) then return estimatedTextWidth(self.text,self.fontSize) end
    return self.width
end
function methods:GetHeight() return self.height end
function methods:GetTextWidth() return estimatedTextWidth(self.text,self.fontSize) end
function methods:GetName() return self.name end
function methods:GetFrameLevel() return self.level or (self.parent and self.parent:GetFrameLevel()+1) or 1 end
function methods:SetFrameLevel(value) self.level=value end
function methods:SetScript(event,fn) self.scripts[event]=fn end
function methods:GetScript(event) return self.scripts[event] end
function methods:RegisterForClicks(...) self.clicks={...} end
function methods:SetText(value) self.text=value end
function methods:SetBackdrop(value) self.backdrop=value end
function methods:SetTexture(...) self.texture={...} end
function methods:SetAllPoints(parent) self.allPoints=parent or self.parent end
function methods:Hide() self.shown=false end
function methods:Show() self.shown=true end
function methods:IsShown() return self.shown end
function methods:IsVisible() return self.shown and (not self.parent or self.parent:IsVisible()) end
function methods:SetFont(path,size) self.fontSize=size end
function methods:SetSpacing(value) self.spacing=value end
function methods:SetAlpha(value) self.alpha=value end
function methods:SetModelScale(value) self.modelScale=value end
function methods:SetPosition(x,y,z) self.position={x,y,z} end
function methods:GetModelScale() return self.modelScale or 1.37 end
function methods:GetPosition()
    local p=self.position or {0,0,.2}
    return p[1],p[2],p[3]
end
function methods:SetTextColor(r,g,b) self.color={r,g,b} end
function methods:SetJustifyH(value) self.align=value end
function methods:SetJustifyV(value) self.verticalAlign=value end
function methods:SetNormalTexture(value) self.normal=value end
function methods:SetChecked(value) self.checked=value end
function methods:GetChecked() return self.checked end
function methods:Enable() self.enabled=true end
function methods:Disable() self.enabled=false end
for _,key in ipairs({"SetVertexColor","SetTexCoord","SetBlendMode","SetPushedTexture","SetDisabledTexture","SetHighlightTexture","SetHitRectInsets","SetBackdropBorderColor","SetBackdropColor","EnableMouse","EnableMouseWheel","SetFrameStrata","RegisterForDrag"}) do
    methods[key]=function() end
end
GameTooltip={Hide=function() end,Show=function() end,SetOwner=function() end,
    SetText=function(self,text) self.title=text;self.lines={} end,
    AddLine=function(self,text) table.insert(self.lines,text) end}
SaureksClosetWeaponAssets={[200]={2,8},[201]={1,7},[202]={2,6},[204]={1,7},[102]={4,3},[103]={4,19},[100]={4,2},[300]={5,0}}
local equipped={201,204,103}
GetInventoryItemLink=function(_,slot)
    local id=equipped[slot-15]
    return id and id>0 and "|Hitem:"..id..":0:0:0|h[Test weapon]|h" or nil
end
dofile("addon/SaureksCloset/Weaponry.lua")
dofile("addon/SaureksCloset/Preview.lua")
dofile("addon/SaureksCloset/UI.lua")
local V=VanityStudio
V.NativeBody=function() return {race=1,sex=1} end
V.frame=CreateFrame("Frame","FullPageTestMain")
V.frame:SetWidth(384);V.frame:SetHeight(512)
V.pagesByName={}
for _,name in ipairs({"armor","weaponry","body","bags","exposure","outfits","settings"}) do
    local p=CreateFrame("Frame",nil,V.frame);p:SetAllPoints(V.frame);V.pagesByName[name]=p
end
V.tabButtons={}
for _,name in ipairs({"character","outfits","settings"}) do V.tabButtons[name]=CreateFrame("Button",nil,V.frame) end
V:CreateArmorPage(V.pagesByName.armor)
V:CreateWeaponryPage(V.pagesByName.weaponry)
V:CreateWardrobeSelector()
for _,key in ipairs({"SetClampedToScreen","SetMovable","RegisterForDrag","SetAutoFocus","SetMaxLetters","SetFontObject","SetMultiLine","SetScrollChild","SetVerticalScroll","SetFocus","HighlightText","StartMoving","StopMovingOrSizing"}) do methods[key]=function() end end
function methods:GetText() return self.text or "" end
function methods:ClearFocus() end
function methods:GetParent() return self.parent end
function methods:Hide()
    local shown=self.shown;self.shown=false
    if shown and self.scripts.OnHide then local old=this;this=self;self.scripts.OnHide();this=old end
end
function methods:Show() self.shown=true end
UIParent=node("Frame","UIParent");UIParent.width=1920;UIParent.height=1080
VanityStudioDB={}
VanityStudioCharacter.weapons={bags={}}
SaureksClosetRendererVersion=function() return 30800 end
SaureksClosetSetBags=function() return 1 end
SaureksClosetSetBagInstanceFit=function() return 1 end
function V:Copy(value)
    if type(value)~="table" then return value end
    local result={};for key,item in pairs(value) do result[key]=self:Copy(item) end;return result
end
V.CloseOutfitMenu=function() end;V.CloseOutfitDetails=function() end;V.CloseBrowser=function() end
V.CancelDraft=function() end;V.TrackUnsaved=function() end
local syncs=0;V.SyncWeapons=function() syncs=syncs+1 end
V.Message=function(self,message) self.lastMessage=message end
V.HidePreviewUntilReady=function() end;V.InvalidatePreviewModel=function() end
V.SetPanelArea=function(self,area) self.panelArea=area end
local entries={}
local menuOpens=0
CloseDropDownMenus=function() end
UIDropDownMenu_AddButton=function(info) table.insert(entries,info) end
ToggleDropDownMenu=function(level,value,menu,anchor)
    menuOpens=menuOpens+1;entries={};menu.anchor=anchor;menu.initialize()
end
IsShiftKeyDown=function() return false end
GameTooltip.ClearLines=function() end
dofile("addon/SaureksCloset/BagCatalog.lua")
dofile("addon/SaureksCloset/Bags.lua")
dofile("addon/SaureksCloset/BagTuner.lua")
dofile("addon/SaureksCloset/BagPlacementEditor.lua")
dofile("addon/SaureksCloset/BagTunerUI.lua")
local realGetBagTunerState=V.GetBagTunerState
V.GetBagTunerState=function(self)
    if self.bagTunerWindow and self.bagTunerWindow.emptySlot then return realGetBagTunerState(self) end
    local target=self.placementTunerBag and 200+self.placementTunerBag or self.placementTunerSlot or 1
    local bag=self.placementTunerBag and self:BagInstance(self.placementTunerBag)
    return {available=true,enabled=true,bag=target,key=tostring(target),title=bag and self.bagCatalogByID[bag.model].name or "Placement",values={scale=100}}
end
V.SetBagTunerPaused=function(self,value) self.bagTunerPaused=value end
V.Refresh=function(self) self:RefreshBagsPage() end
V:CreateBagsPage(V.pagesByName.bags)
V:SetTab("bags")
local checks=0
local function check(condition,message) assert(condition,message);checks=checks+1 end
local function click(widget)
    if not widget.enabled or not widget:IsVisible() then return end
    local old=this;this=widget;widget.scripts.OnClick();this=old
end
local function hover(widget)
    local old=this;this=widget;widget.scripts.OnEnter();this=old
end
local function pointOffset(point,w,h)
    local x=string.find(point,"LEFT",1,true) and 0 or (string.find(point,"RIGHT",1,true) and w or w/2)
    local y=string.find(point,"TOP",1,true) and 0 or (string.find(point,"BOTTOM",1,true) and h or h/2)
    return x,y
end
local function rect(n)
    if not n or n==V.frame then return 0,0,384,512 end
    if n.allPoints then return rect(n.allPoints) end
    local a=n.anchor
    local x,y,w,h=rect(a and a[2] or n.parent)
    local nw,nh=n.width or w,n.height or h
    if n.kind=="FontString" and (not n.width or n.width==0) then nw=n:GetWidth() end
    if a then
        local rx,ry=pointOffset(a[3],w,h);local ax,ay=pointOffset(a[1],nw,nh)
        x=x+rx-ax+(a[4] or 0);y=y+ry-ay-(a[5] or 0)
    end
    return x,y,nw,nh
end
local function inContent(n,description)
    local x,y,w,h=rect(n)
    check(x>=23 and x+w<=341,description.." exceeds usable width")
    check(y>=79 and y+h<=392,description.." overlaps navigation")
end
local function overlaps(a,b)
    local x,y,w,h=rect(a);local bx,by,bw,bh=rect(b)
    return x<bx+bw and x+w>bx and y<by+bh and y+h>by
end
check(V.pagesByName.bags:IsVisible(),"Bags page is visible")
check(not V.pagesByName.armor:IsVisible(),"List has the full width like weapon controls")
check(not V.model:IsVisible() and not V.rotationControls:IsVisible(),"List controls do not obscure the character preview")
check(table.getn(V.bagRows)==5,"All five fixed slots are visible")
check(not V.bagCount and not V.addBagButton and not V.bagEmpty,"Bag heading/count/Add row is removed")
check(not V.bagPrevious and not V.bagNext and not V.bagListHint and not V.pagesByName.bags.scripts.OnMouseWheel,"Fixed slots have no pagination or footer text")
for i,row in ipairs(V.bagRows) do
    check(row:IsVisible() and not row.bagID and row.choose.enabled and not row.remove:IsShown(),"Each empty slot remains visible and customizable")
    inContent(row,"Empty bag slot")
end
local tuner=V.bagTunerWindow
check(tuner.positionMode.closetPanel,"Coordinate mode uses the shared native red button style")
check(tuner.positionMode.anchor[4]+tuner.positionMode.width==331 and tuner.positionMode.anchor[5]==-151 and tuner.positionMode.height==22,"Mode button sits at the top right of the value rows")
check(not overlaps(tuner.positionMode,tuner.live) and not overlaps(tuner.positionMode,tuner.rows[1].editor),"Mode button cannot overlap live tuning or numeric fields")
click(V.bagRows[4].choose)
check(tuner.emptySlot==4 and tuner:IsShown() and table.getn(V:GetBags())==0,"Opening an empty slot does not create or equip a default bag")
check(tuner.modelPicker:IsShown() and not tuner.save.enabled and not tuner.reset.enabled and not tuner.live.enabled,"Empty slot opens the visual picker with fitting actions disabled")
check(not tuner.positionMode.enabled,"Empty slots cannot switch placement modes")
for _,row in ipairs(tuner.rows) do check(row.editor:GetText()=="","Empty slot has no legacy fit values") end
local staleEmpty=tuner.modelPicker.rows[1]
V:CloseBagTunerModelPicker()
check(table.getn(V:GetBags())==0 and tuner:IsShown(),"Dismissing the model picker leaves the slot empty")
local staleThis=this;this=staleEmpty;staleEmpty.scripts.OnClick();this=staleThis
check(table.getn(V:GetBags())==0,"A dismissed empty-slot picker cannot create a bag through a stale callback")
tuner:Hide()
check(not tuner.emptySlot and not V.bagRows[4].selected:IsShown(),"Closing empty tuner clears its target and highlight")
click(V.bagRows[1].choose)
local savedThis=this;this=staleEmpty;staleEmpty.scripts.OnClick();this=savedThis
-- Reopened cards now target slot1; creation must use that slot, never old slot4.
local first=V:BagInSlot(1)
check(first and not V:BagInSlot(4) and first.mount=="back","Model selection creates the bag only in the currently chosen slot")
check(V.tab=="bags" and V.pagesByName.bags:IsVisible() and not V.model:IsVisible(),"Choosing a bag stays on the Bags page")
check(tuner:IsShown() and V.placementTunerBag==first.id and not tuner.emptySlot,"Selecting a model turns the empty slot into its placement tuner")
check(not tuner.savedState,"Bottom saved/default-fit label is removed")
check(tuner.modelSelector:IsVisible() and tuner.mountSelector:IsVisible(),"Model and starting position are integrated into the tuner")
click(tuner.modelSelector)
local picker=tuner.modelPicker
check(picker:IsShown() and table.getn(picker.rows)==6,"Visual picker shows all six model choices")
local seen={}
for _,row in ipairs(picker.rows) do
    seen[row.choice.name]=true
    check(row:IsVisible() and row.icon.width==40 and row.icon.height==40,"Every model has a visible forty-pixel icon")
    check(row.icon.texture[1]==row.choice.icon,"Picker uses each model family custom icon")
    check(row.bodyType.text==(row.choice.name=="Mageweave Bag" and "Soft body" or "Rigid body"),"Picker identifies soft versus rigid bags")
    check(not overlaps(row.caption,row.bodyType),"Body type stays below the model name")
end
for _,model in ipairs(V.bagModelChoices) do check(seen[model.name],"Model family is reachable: "..model.name) end
for _,name in ipairs({"Burgundy Rucksack","Milloo Dark Leather","Milloo Scratch","Milloo Tan Leather","Olive Cloth Pouch"}) do
    check(not seen[name],"Removed model is absent: "..name)
end
local function choose(menuButton,name)
    if menuButton==tuner.modelSelector then
        if not picker:IsShown() then click(menuButton) end
        for _,row in ipairs(picker.rows) do if row.choice.name==name then click(row);return end end
    else
        click(menuButton)
        for _,entry in ipairs(entries) do if entry.text==name then entry.func(entry.arg1);return end end
    end
    error("Missing selection: "..name)
end
choose(tuner.modelSelector,"Mageweave Bag")
check(first.model==16 and tuner:IsShown() and V.placementTunerBag==first.id,"Choosing a model updates the bag without closing the tuner")
choose(tuner.mountSelector,"Left hip")
check(first.mount=="leftHip" and tuner:IsShown(),"Starting position changes in the same tuner")
check(tuner.modelCaption.text=="Mageweave Bag" and tuner.mountCaption.text=="Left hip","Both selectors display the active choices")
check(tuner.colorChoices:IsVisible() and table.getn(tuner.colorSwatches)==4,"Mageweave Bag offers four color swatches")
for _,swatch in ipairs(tuner.colorSwatches) do
    click(swatch)
    check(first.model==swatch.modelID and swatch.selectedBorder:IsShown() and swatch.selectedCheck:IsShown(),"Color changes the model and marks its swatch")
    check(V.tab=="bags" and V.bagRows[1].title.text=="Mageweave Bag","Color selection stays on Bags with the grouped name")
    check(string.find(V.bagRows[1].subtitle.text,swatch.color.name,1,true) and V.bagRows[1].iconButton.icon.texture[1]==V.bagCatalogByID[first.model].icon,"Color is reflected in the card subtitle and custom icon")
end
check(first.model==16,"Olive is the last displayed color")
choose(tuner.modelSelector,"Mageweave Bag");check(first.model==16,"Reselecting the family preserves its color")
local editor=tuner.rows[1].editor
editor.editing=true;editor.targetKey=V:GetBagTunerState().key;editor:SetText("invalid")
local beforeOpen=menuOpens
click(tuner.modelSelector)
check(not picker:IsShown() and first.model==16,"Invalid numeric edits block opening the model selector")
local originalSetValue=V.SetBagTunerValue;local committed
V.SetBagTunerValue=function(self,key,value) committed={key,value};return true end
editor.editing=true;editor.targetKey=V:GetBagTunerState().key;editor:SetText("0.25")
click(tuner.modelSelector)
check(committed and committed[1]==editor.field.key and committed[2]==.25 and picker:IsShown(),"Valid numeric edits commit before selecting a model")
V.SetBagTunerValue=originalSetValue
local staleEntry=picker.rows[1]
local ok,otherBag=V:AddBag(2,"back")
check(ok,"Second bag is available for stale-menu protection")
V:OpenBagTuner(otherBag.id)
local oldThis=this;this=staleEntry;staleEntry.scripts.OnClick();this=oldThis
check(first.model==16 and otherBag.model==2,"A stale menu callback cannot change a different bag")
V:DeleteBag(otherBag.id);V:OpenBagTuner(first.id)
local function inTuner(widget,description)
    local tx,ty=rect(tuner);local x,y,w,h=rect(widget)
    check(x-tx>=23 and x-tx+w<=341,description.." exceeds tuner width")
    check(y-ty>=70 and y-ty+h<=436,description.." exceeds tuner content height")
end
local controls={tuner.modelSelector,tuner.mountSelector,tuner.colorChoices,tuner.status,tuner.live,tuner.save,tuner.load,tuner.reset,tuner.export}
for _,row in ipairs(tuner.rows) do table.insert(controls,row.editor.parent) end
for i,control in ipairs(controls) do
    inTuner(control,"Tuner control "..i)
    for j=1,i-1 do check(not overlaps(control,controls[j]),"Tuner controls must remain separate") end
end
for i,mount in ipairs({"Back","Right hip","Left hip","Back"}) do
    V:SetTab("bags");click(V.bagRows[i+1].choose)
    choose(tuner.modelSelector,"Runecloth Bag");choose(tuner.mountSelector,mount)
end
V:SetTab("bags")
check(table.getn(V:GetBags())==5,"All five slots can be customized")
check(not V:AddBag(1,"back"),"Direct add also respects the five-bag limit")
for i,row in ipairs(V.bagRows) do
    local bag=V:BagInSlot(i)
    check(row:IsVisible() and bag and row.bagID==bag.id,"Every fixed slot binds its stable bag identity")
    inContent(row,"Bag card");inContent(row.remove,"Remove bag")
    local x,y,w,h=rect(row)
    check(x==23 and 341-(x+w)==6 and h==54,"Bag cards leave only a six-pixel right inset, without a scrollbar gutter")
    local rx,ry,rw,rh=rect(row.remove)
    check(rx+rw==x+w-4 and ry+rh/2==y+h/2,"Remove remains inset and centered inside the narrower row")
    local tx,ty,tw,th=rect(row.title)
    local sx,sy,sw,sh=rect(row.subtitle)
    local bx,by,bw,bh=rect(row.bodyType)
    check(row.bodyType.text==V:BagBodyType(bag.model),"Equipped bag shows its actual body type")
    check(by>=sy+sh and by+bh<=y+h-4 and bx+bw<rx,"Body type fits on its own line inside the card")
    check(tx+tw<rx and sx+sw<rx,"Both text lines stay clear of Remove")
    check(not overlaps(row.choose,row.remove),"Edit and remove targets are separate")
    check(row.iconButton.width==40 and row.iconButton.height==40,"Custom bag artwork has a readable forty-pixel icon")
    check(not row.gear,"Bag cards have no redundant cogwheel")
    check(row.backdrop.bgFile=="Interface\\DialogFrame\\UI-DialogBox-Background","Cards use the clean dialog background")
    if i>1 then
        local px,py,pw,ph=rect(V.bagRows[i-1])
        check(y-(py+ph)==2,"Bag rows retain a compact two-pixel gap")
        check(not overlaps(V.bagRows[i-1],row),"Bag cards remain separate")
    end
end
local row=V.bagRows[4];local targetID=row.bagID;local original=V:BagInstance(targetID)
hover(row.choose);check(GameTooltip.title==V.bagCatalogByID[original.model].name,"Tooltip retains the full bag name")
click(row.choose)
check(tuner:IsShown() and V.placementTunerBag==targetID and V.tab=="bags","Clicking a card opens its tuner while keeping Bags visible")
check(row.selected:IsShown(),"The tuned bag is highlighted in the list")
choose(tuner.mountSelector,"Right hip")
check(V:BagInstance(targetID).mount=="rightHip","Mount callback edits the chosen instance")
choose(tuner.modelSelector,"Slim Leather Bag")
check(V:BagInstance(targetID).model==5 and V:BagInstance(targetID).mount=="rightHip","Replacing a model preserves its mount")
check(tuner.colorChoices:IsVisible() and not tuner.colorSwatches[4]:IsShown(),"Leather offers three colors without a stale fourth cloth swatch")
for i,id in ipairs({5,7,8}) do
    click(tuner.colorSwatches[i])
    check(V:BagInstance(targetID).model==id and V:BagInstance(targetID).mount=="rightHip","Leather color keeps the mount and stable bag identity")
    check(V.tab=="bags" and V.bagRows[4].title.text=="Slim Leather Bag","Leather variant shares its family name without changing pages")
    check(V.bagRows[4].iconButton.icon.texture[1]==V.bagCatalogByID[id].icon,"Leather icon follows the selected color")
end
choose(tuner.modelSelector,"Slim Leather Bag");check(V:BagInstance(targetID).model==8,"Reselecting the leather family preserves Tan")
choose(tuner.modelSelector,"Runecloth Bag");check(not tuner.colorChoices:IsVisible(),"Ungrouped models hide color controls")
choose(tuner.modelSelector,"Slim Leather Bag");click(tuner.colorSwatches[3])
local preservedID=V:BagInSlot(5).id
click(V.bagRows[2].remove)
check(table.getn(V:GetBags())==4 and V.bagRows[2]:IsVisible() and not V.bagRows[2].bagID,"Delete leaves its slot empty and visible")
check(V.bagRows[4].bagID==targetID and V.bagRows[5].bagID==preservedID,"Deletion preserves the positions of all other slots")
click(V.bagRows[4].iconButton)
check(V.tab=="bags" and V.pagesByName.bags:IsVisible(),"Icon never switches to Outfit")
check(tuner:IsShown() and V.placementTunerBag==targetID and not V.placementTunerSlot,"Icon opens only the chosen stable bag ID")
check(not tuner.pause,"Bag physics controls belong on the Animations page")
local layoutGetState=V.GetBagTunerState
V.GetBagTunerState=function(self)
    local state=layoutGetState(self)
    self.bagTunerDrafts=self.bagTunerDrafts or {}
    self.bagTunerDrafts[state.key]=self.bagTunerDrafts[state.key] or {up=0,scale=100}
    state.values=self:Copy(self.bagTunerDrafts[state.key]);return state
end
local upRow=tuner.rows[3];local upEditor=upRow.editor
upEditor.editing=true;upEditor.targetKey=V:GetBagTunerState().key;upEditor:SetText("-2.25")
check(V:CommitBagTunerEditor(upEditor),"Typed foot placement is accepted")
click(upRow.minus)
check(math.abs(V:GetBagTunerState().values.up+2.255)<.000001,"Nudging a foot fit stays at its height without snapping to the former limit")
tuner:Hide();check(not V.bagRows[4].selected:IsShown(),"Closing the tuner clears the selected card")
V:OpenPlacementTuner(101)
check(V.placementTunerSlot==101 and not V.placementTunerBag,"Weapon tuner never inherits a bag target")
check(not tuner.pause,"Weapon tuner has no bag physics toggle")
check(not tuner.modelSelector:IsVisible() and not tuner.mountSelector:IsVisible() and not tuner.colorChoices:IsVisible(),"Bag selectors stay hidden when tuning weapons")
check(V:SetBagTunerValue("up",-1),"Weapon lower bound remains accepted")
click(upRow.minus)
check(V:GetBagTunerState().values.up==-1,"Weapon nudge retains its original lower bound")
V.GetBagTunerState=layoutGetState
V:OpenBagTuner(targetID)
local targetRow
for _,candidate in ipairs(V.bagRows) do if candidate.bagID==targetID then targetRow=candidate end end
click(targetRow.remove)
check(not V:BagInstance(targetID) and not tuner:IsShown(),"Removing the tuned bag closes its tuner")
V:OpenBagTuner(99);check(not tuner:IsShown(),"An invalid bag ID cannot open a stale tuner")
click(V.bagRows[4].choose)
choose(tuner.modelSelector,"Runecloth Bag")
check(V:BagInSlot(4) and V.bagRows[5].bagID==preservedID and not V:BagInSlot(2),"Refilling a later empty slot does not move bags or fill an earlier empty slot")
SaureksClosetRendererVersion=function() return 30713 end;V:RefreshBagsPage()
check(string.find(V.bagNotice.text,"restart",1,true),"Old renderer shows restart guidance")
for _,candidate in ipairs(V.bagRows) do
    check(candidate:IsShown() and not candidate.choose.enabled and not candidate.remove.enabled,"Old renderer shows all five slots but cannot mutate bags")
end
check(syncs>0,"Bag interactions reached the real state change path")
local visibility=V.bagVisibilityCheckbox
check(visibility.checked==1,"Bags are shown by default")
for _,index in ipairs({1,3,4,5}) do
    check(V.bagVisibilityOptions.anchor[index]==V.weaponModeOptions.anchor[index],"Visibility aligns with Advanced mode")
end
check(V.bagVisibilityOptions.height==V.weaponModeOptions.height,"Matching footer height")
check(not visibility.enabled,"Old renderer disables visibility toggle")
SaureksClosetRendererVersion=function() return 40004 end;V:RefreshBagsPage()
visibility:SetChecked(nil);click(visibility)
check(not V:BagsShown() and not visibility.checked,"Checkbox hides bags")
visibility:SetChecked(1);click(visibility)
check(V:BagsShown() and visibility.checked==1,"Checkbox restores bags")
print("PASS: "..checks.." bag list, integrated tuner, identity and geometry checks. Mocked frames, not an in-game visual test.")
