-- Exercise the real weapon page and tab transitions with 1.12-shaped frames.
-- This verifies visibility, interaction and geometry, not game font rendering.
table.getn=table.getn or function(t) return #t end
math.mod=math.mod or math.fmod
VanityStudio={slotButtons={},slotNames={},index={}}
VanityStudioCharacter={enabled=true,weapons={independent=true,carriedEnabled=false}}
SaureksClosetSetWeapons=function() return true end
SaureksClosetRendererVersion=function() return 30704 end
UISpecialFrames={}
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
    if self.kind=="FontString" and self.width==0 then return estimatedTextWidth(self.text,self.fontSize) end
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
function methods:SetTextColor(r,g,b) self.color={r,g,b} end
function methods:SetJustifyH(value) self.align=value end
function methods:SetJustifyV(value) self.verticalAlign=value end
function methods:SetNormalTexture(value) self.normal=value end
function methods:SetChecked(value) self.checked=value end
function methods:GetChecked() return self.checked end
function methods:Enable() self.enabled=true end
function methods:Disable() self.enabled=false end
for _,key in ipairs({"SetVertexColor","SetTexCoord","SetBlendMode","SetPushedTexture","SetDisabledTexture","SetHighlightTexture","SetHitRectInsets","SetBackdropBorderColor","SetBackdropColor","EnableMouse","SetFrameStrata"}) do
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
local refreshes=0
local closed=0
V.CloseOutfitMenu=function() end
V.CloseOutfitDetails=function() end
V.CloseBrowser=function() closed=closed+1;V.draft=nil end
V.ReleaseNeighbor=function() end
V.HidePreviewUntilReady=function() end
V.InvalidatePreviewModel=function() end
V.Refresh=function(self) refreshes=refreshes+1;self:RefreshWeaponCards() end
V.SyncWeapons=function() end
V.TrackUnsaved=function() end
V.Message=function(self,text) self.lastMessage=text end
V:SetTab("weaponry")
assert(V.pagesByName.weaponry:IsVisible(),"The weapon page must open directly")
assert(not V.pagesByName.armor:IsVisible(),"The character page is still beneath the weapon controls")
for _,model in ipairs({V.model,V.previewBuffer}) do assert(not model:IsVisible(),"A player model is visible on the options page") end
for _,background in ipairs(V.wardrobeBackgrounds) do assert(not background:IsVisible(),"The scenic background is visible on the options page") end
assert(not V.rotationControls:IsVisible(),"Model rotation controls must not occupy the options page")
assert(not V.wardrobeViewBorder:IsVisible() and not V.wardrobeViewShadowFrame:IsVisible())
assert(V.wardrobeSelectorBox:IsVisible(),"Players must be able to leave the weapon page")
assert(V.tabButtons.character.selected,"Weapon options still belong to the Wardrobe tab")
assert(not V.weaponPoseButton or not V.weaponPoseButton:IsVisible(),"Stowed/melee/ranged preview mode is still visible")
assert(not V.weaponOptionsButton or not V.weaponOptionsButton:IsVisible(),"The full-page options are still behind another button")
assert(not V.weaponSidebar or not V.weaponSidebar:IsVisible(),"Weapon options still use a narrow sidebar")
for _,tab in pairs(V.weaponSectionTabs or {}) do assert(not tab:IsVisible(),"Item choices must not be hidden behind category tabs") end

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
    if a then
        local rx,ry=pointOffset(a[3],w,h)
        local ax,ay=pointOffset(a[1],nw,nh)
        x=x+rx-ax+(a[4] or 0);y=y+ry-ay-(a[5] or 0)
    end
    return x,y,nw,nh
end
local function inContent(n,description)
    local x,y,w,h=rect(n)
    assert(x>=19 and x+w<=342,description.." extends beyond the window's usable width")
    assert(y>=75 and y+h<=392,description.." overlaps the header or page selector")
    return x,y,w,h
end
local function overlaps(a,b)
    local x,y,w,h=rect(a);local bx,by,bw,bh=rect(b)
    return x<bx+bw and x+w>bx and y<by+bh and y+h>by
end
assert(not V.weaponModeDescription,"Advanced mode must not have inline subtext")
local advancedToggle=assert(V.weaponAdvancedCheckbox,"The mode needs a dedicated Advanced mode checkbox")
local modeOptions=assert(V.weaponModeOptions)
local modeCaption
for _,n in ipairs(frames) do
    if n.parent==modeOptions and n.kind=="FontString" and n.text=="Advanced mode" then modeCaption=n end
end
assert(modeCaption,"The mode switch must be named Advanced mode")
assert(advancedToggle:IsVisible() and modeCaption:IsVisible())
inContent(modeOptions,"Mode controls")
inContent(advancedToggle,"Advanced mode switch")
inContent(modeCaption,"Advanced mode caption")
local _,cy,_,ch=rect(modeCaption);local _,ty,_,th=rect(advancedToggle)
assert(cy+ch/2==ty+th/2 and modeCaption.verticalAlign=="MIDDLE","The mode caption must align with its checkbox")
assert(not overlaps(advancedToggle,modeCaption))
assert(estimatedTextWidth(modeCaption.text,modeCaption.fontSize)<=modeCaption:GetWidth(),"The mode caption is clipped")
for _,field in ipairs({"hideRangedWhenStored","hideMeleeWhenStored"}) do
    assert(not V[field.."Checkbox"],"The old stored-weapon flags must not compete with Advanced mode")
end
local function assertCard(slot)
    local card=assert(V.weaponCards[slot],"Missing item choice "..slot)
    local b=assert(V.slotButtons[slot])
    assert(card:IsVisible() and b:IsVisible(),"Item choice "..slot.." is hidden")
    assert(not card.state,"Redundant equipment descriptions must not return")
    local x,y,w,h=inContent(card,"Item choice "..slot)
    assert(b.width>=24 and b.height>=24,"The item selection target is too small")
    inContent(b,"Item icon "..slot)
    assert(b.clicks[1]=="LeftButtonUp" and b.clicks[2]=="RightButtonUp")
    assert(not card.item and not card.itemMeasure,"Item rows must not duplicate selected names")
    assert(not overlaps(modeOptions,card),"Mode controls overlap item choice "..slot)
    if slot<=107 then
        local gear=assert(card.gear,"Carried appearances need direct placement controls")
        assert(gear:IsVisible() and gear.parent==b and gear.width>=16)
        assert(gear:GetFrameLevel()>b:GetFrameLevel(),"The placement cog must remain above the item icon")
        inContent(gear,"Placement cog "..slot)
        local title
        for _,n in ipairs(frames) do
            if n.parent==card and n.kind=="FontString" and n:IsVisible() then
                assert(not title,"Carried item rows must not contain redundant subtext")
                title=n
            end
        end
        assert(title and title.text==V.weaponNames[slot-100],"Every carried item row needs its role label")
        local _,labelY,_,labelH=rect(title)
        assert(labelY+labelH/2==y+h/2,"Carried role labels must be centered vertically")
    else
        assert(not card.gear,"In-use appearances do not have a carried placement")
    end
    return card,b,x,y,w,h
end
local function assertSimple()
    assert(not advancedToggle:GetChecked(),"Simple mode must appear as an unchecked Advanced mode checkbox")
    assert(not V.weaponCarriedGroup:IsVisible(),"Simple mode must hide the entire carried section")
    for slot=101,107 do
        assert(not V.weaponCards[slot]:IsVisible() and not V.slotButtons[slot]:IsVisible(),"Simple mode exposes carried item controls")
    end
    for slot=108,110 do
        local card,b,_,_,width,height=assertCard(slot)
        assert(width==318 and height==52 and b.width==36 and b.height==36,"Simple equipment choices must use large full-width rows")
        for other=108,slot-1 do assert(not overlaps(card,V.weaponCards[other]),"Simple equipment rows overlap") end
    end
end
local function assertAdvanced()
    assert(advancedToggle:GetChecked(),"Advanced mode must appear as a checked checkbox")
    assert(V.weaponCarriedGroup:IsVisible(),"Advanced mode must expose carried positions")
    for slot=101,110 do
        local card,b=assertCard(slot)
        for other=101,slot-1 do assert(not overlaps(card,V.weaponCards[other]),"Advanced item cards overlap: "..slot.." and "..other) end
    end
    local mx,my,mw=rect(V.weaponCards[108]);local ox,oy,ow=rect(V.weaponCards[109])
    assert(my==oy and mx<ox and mw==158 and ow==158,"Advanced mode must show independent main- and off-hand choices side by side")
    local rx,ry,rw=rect(V.weaponCards[110])
    assert(rx==mx and ry>my and rw==318,"Advanced ranged appearance needs its own full-width row")
end
assertSimple()
assert(V.slotButtons[108].enabled and V.slotButtons[109].enabled,"Two equipped short swords must expose two usable appearance choices")
local _,headingY,_,headingH=rect(V.weaponActiveGroup.heading)
local _,descriptionY,_,descriptionH=rect(V.weaponSlotDescription)
local _,firstCardY=rect(V.weaponCards[108])
assert(descriptionY>headingY+headingH and descriptionY+descriptionH<firstCardY,"Equipped-slot subtext must sit below its header and above the rows")
assert(V.weaponSlotDescription.spacing==4,"Slot explanation must retain line spacing")
assert(headingY==92,"Equipped weapon slots should begin near the top with clear padding")
local _,modeY,_,modeH=rect(modeOptions)
assert(modeY==366 and modeY+modeH<=392 and modeY>firstCardY,"Advanced mode belongs at the bottom of the page")
V:CloseWeaponOptions()
assert(advancedToggle:IsVisible(),"Opening an item browser must not hide the mode switch")
this=advancedToggle;this:SetChecked(1);this.scripts.OnClick()
assert(VanityStudioCharacter.weapons.carriedEnabled,"Checking Advanced mode must enable carried overrides")
assertAdvanced()
print("PASS: Simple mode exposes three equipped slots; Advanced mode adds independent hand choices and all carried positions")

local opened,tuned,menu
local openSlotMenu=V.OpenSlotMenu
V.OpenBrowser=function(_,slot) opened=slot end
V.OpenPlacementTuner=function(_,slot) tuned=slot end
V.OpenSlotMenu=function(_,slot) menu=slot end
for slot=101,110 do
    this=V.slotButtons[slot];arg1="LeftButton";this.scripts.OnClick()
    assert(opened==slot,"Item selection opened the wrong slot")
    arg1="RightButton";this.scripts.OnClick();assert(menu==slot)
    this=V.weaponCards[slot];arg1="LeftButton";this.scripts.OnClick()
    assert(opened==slot,"The full item row must open its selection")
    arg1="RightButton";this.scripts.OnClick();assert(menu==slot)
    if slot<=107 then
        VanityStudioCharacter.weapons[slot]=200
        this=V.weaponCards[slot].gear;this.scripts.OnClick();assert(tuned==slot)
        VanityStudioCharacter.weapons[slot]=nil
        opened=nil;this.scripts.OnClick();assert(opened==slot,"Empty placement cog must open item selection")
    end
end
local sx,sy,sw,sh=rect(V.wardrobeSelectorBox)
assert(sw==176 and V.wardrobeSelectorBox.anchor[4]==-12,"Only Weaponry should use the corrected wide navigation selector")
local spans={}
for _,n in ipairs(frames) do
    if n.parent==V.wardrobeSelectorBox and n.kind=="Texture" and n:IsVisible() and n.texture and n.texture[1]=="Interface\\PaperDoll\\UI-PaperDoll-SlotBackground" then
        local x,_,w=rect(n);table.insert(spans,{x-sx,x-sx+w})
    end
end
table.sort(spans,function(a,b) return a[1]<b[1] end)
local edge=4
for _,span in ipairs(spans) do assert(span[1]<=edge,"The wide navigation texture has a visible gap");edge=math.max(edge,span[2]) end
assert(edge==172,"Native stone texture must fill the widened Weaponry selector")
for _,pageName in ipairs({"armor","body","bags","exposure"}) do
    V:SetTab(pageName)
    assert(V.wardrobeSelectorBox.width==106 and V.wardrobeSelectorBox.anchor[4]==-44,"Weaponry must not change another page's navigation")
    for _,fill in ipairs(V.weaponSelectorFill) do assert(not fill:IsVisible(),"Weaponry-only texture extended another page") end
end
V:SetTab("body")
assert(V.bodyPreviewFade:IsVisible() and #V.bodyPreviewFade.strips==42,"Body preview needs a soft lower fade")
local previousAlpha=-1
for _,strip in ipairs(V.bodyPreviewFade.strips) do
    assert(strip.alpha>previousAlpha and strip.alpha<=1,"Body fade must grow smoothly toward the backdrop")
    previousAlpha=strip.alpha
end
assert(previousAlpha==1,"The lower preview edge must disappear into the backdrop")
assert(V.bodyPreviewFade.tail and V.bodyPreviewFade.tail.anchor[5]==-126,"No model should reappear below the fade")
for _,model in ipairs({V.model,V.previewBuffer}) do
    assert(model.anchor[4]==113 and model.width==232,"Body preview should sit a little farther right")
    assert(model.modelScale==1.55 and model.position[3]==-.55,"Body preview should frame the bust and face")
end
V:SetTab("bags")
assert(not V.bodyPreviewFade:IsVisible(),"Other pages must not inherit the Body fade")
for _,model in ipairs({V.model,V.previewBuffer}) do
    assert(model.anchor[4]==96 and model.width==244 and model.modelScale==1 and model.position[3]==0,
        "The normal character preview must return on other pages")
end
V:SetTab("armor")
assert(V.pagesByName.armor:IsVisible() and V.model:IsVisible() and V.wardrobeBackgrounds[1]:IsVisible())
assert(V.rotationControls:IsVisible(),"Returning to Outfit must restore model controls")
assert(not V.pagesByName.weaponry:IsVisible())
V:SetTab("weaponry");assertAdvanced()
print("PASS: direct item and placement actions retain their targets; navigation texture, other pages and the character view stay correct")

-- Item names belong in tooltips, not duplicated on the page.
VanityStudioCharacter.weapons={independent=true,carriedEnabled=true,[108]=201,[109]=204,[110]=102,[103]=200,[106]=100,[107]=300}
V.index={[200]={200,"Arcanite Reaper"},[201]={201,"Cruel Barb"},[204]={204,"Short Sword"},[202]={202,"High Warlord's Pig Sticker"},[102]={102,"Dwarven Hand Cannon"},[103]={103,"Bonecreeper Stylus"},[100]={100,"Laminated Recurve Bow"},[300]={300,"Ancient Sinew Wrapped Lamina"}}
V:RefreshWeaponCards()
for slot=101,110 do
    local id=VanityStudioCharacter.weapons[slot]
    if id then
        for _,target in ipairs({V.slotButtons[slot],V.weaponCards[slot]}) do
            this=target;this.scripts.OnEnter()
            assert(GameTooltip.lines[1]==V.index[id][2],"Item icons and rows must preserve full selected names in tooltips")
        end
    end
end
this=advancedToggle;this.scripts.OnEnter()
assert(GameTooltip.title=="Advanced mode")
assert(#GameTooltip.lines>=2,"The mode tooltip must explain both simple and advanced choices")
print("PASS: tooltips preserve full item names and explain Advanced mode")

-- Exercise the real setter and preview cancellation: switching modes may not
-- commit a draft or discard saved appearances and placements.
local syncs,dirty=0,0
local snapshots={}
V.SyncWeapons=function(self)
    syncs=syncs+1
    table.insert(snapshots,{enabled=self:WeaponAdvancedMode(VanityStudioCharacter.weapons),draft=self.draft,selected=VanityStudioCharacter.weapons[106]})
end
V.TrackUnsaved=function() dirty=dirty+1 end
local saved=VanityStudioCharacter.weapons
V.draft={slot=106,id=102};V.worldDraftActive=106
assert(advancedToggle:GetChecked(),"An existing carried override must migrate to checked Advanced mode")
this=advancedToggle;this:SetChecked(nil);this.scripts.OnClick()
assert(saved.carriedEnabled==false and V.draft==nil and V.worldDraftActive==nil,"Turning off Advanced mode must cancel the live item draft")
assert(saved[106]==100 and saved[108]==201 and saved[109]==204 and saved[110]==102,"Mode changes must preserve every saved appearance")
assert(syncs==2 and dirty==1,"The draft must restore before the changed mode is synchronized")
assert(snapshots[1].enabled and snapshots[1].draft==nil and snapshots[1].selected==100)
assert(not snapshots[2].enabled and snapshots[2].draft==nil and snapshots[2].selected==100)
assertSimple()
for slot=101,107 do
    local card,b=V.weaponCards[slot],V.slotButtons[slot]
    assert(not card.enabled and not b.enabled and not card.gear.enabled,"Hidden carried controls must also stop accepting edits")
    opened=nil;menu=nil;tuned=nil
    this=card;arg1="LeftButton";this.scripts.OnClick()
    this=b;arg1="RightButton";this.scripts.OnClick()
    this=card.gear;this.scripts.OnClick()
    assert(not opened and not menu and not tuned,"A hidden carried section still accepted an item or placement action")
end
for slot=108,110 do
    opened=nil;this=V.slotButtons[slot];arg1="LeftButton";this.scripts.OnClick()
    assert(opened==slot,"Simple mode must independently edit every equipped slot")
end
-- Occupancy is based on live equipment, not which appearances are selected.
equipped={202,0,103};V:RefreshWeaponCards()
assert(not V.slotButtons[109].enabled and not V.weaponCards[109].enabled,"A two-handed main hand must disable the unavailable off hand")
local function visibleCardText(slot,text)
    for _,n in ipairs(frames) do
        if n.parent==V.weaponCards[slot] and n.kind=="FontString" and n:IsVisible() and n.text==text then return true end
    end
    return false
end
assert(visibleCardText(109,"Two-handed main hand"),"Unavailable offhand needs a clear two-handed reason")
opened=nil;menu=nil
this=V.slotButtons[109];arg1="LeftButton";this.scripts.OnClick()
this=V.weaponCards[109];arg1="RightButton";this.scripts.OnClick()
assert(not opened and not menu,"An unavailable offhand still opens its editor")
equipped={201,0,0};V:RefreshWeaponCards()
for _,slot in ipairs({109,110}) do
    assert(not V.slotButtons[slot].enabled and visibleCardText(slot,"No item equipped"),"Empty equipment slots need a disabled state and reason")
end
equipped={201,204,103};V:RefreshWeaponCards()
for slot=108,110 do
    assert(V.weaponCards[slot].enabled and V.slotButtons[slot].enabled,"Equipping a weapon must restore its appearance editor")
    assert(not visibleCardText(slot,"No item equipped") and not visibleCardText(slot,"Two-handed main hand"),"An occupied slot kept its stale disabled reason")
end
assertSimple()
this=advancedToggle;this:SetChecked(1);this.scripts.OnClick()
assert(saved.carriedEnabled and saved[106]==100 and saved[109]==204 and saved[110]==102)
assert(syncs==3 and dirty==2);assertAdvanced()
for slot=101,107 do assert(V.weaponCards[slot].enabled and V.slotButtons[slot].enabled and V.weaponCards[slot].gear.enabled) end
-- Even in Advanced mode, unavailable real hand slots cannot produce a usable
-- attacking weapon. Carried slots remain independently editable.
equipped={202,0,0};V:RefreshWeaponCards()
for _,slot in ipairs({109,110}) do
    assert(not V.weaponCards[slot].enabled and not V.slotButtons[slot].enabled,"Advanced mode must not imply a nonexistent attacking slot can be used")
    opened=nil;menu=nil
    this=V.weaponCards[slot];arg1="LeftButton";this.scripts.OnClick()
    this=V.slotButtons[slot];arg1="RightButton";this.scripts.OnClick()
    assert(not opened and not menu,"Advanced mode opened an unavailable attacking slot")
    for _,target in ipairs({V.weaponCards[slot],V.slotButtons[slot]}) do
        this=target;this.scripts.OnEnter()
        local found=false
        for _,line in ipairs(GameTooltip.lines) do
            if line==(slot==109 and "Two-handed main hand" or "No item equipped") then found=true end
        end
        assert(found,"Advanced mode must explain unavailable attacking slots in their tooltip")
    end
end
assert(V.slotButtons[101].enabled and V.slotButtons[105].enabled,"Unavailable hand slots must not disable carried decorations")
equipped={99999,99998,99997};V:RefreshWeaponCards()
for slot=108,110 do
    assert(V.weaponCards[slot].enabled and V.slotButtons[slot].enabled,"An equipped server item missing from the catalog must still expose its real slot")
    opened=nil;this=V.slotButtons[slot];arg1="LeftButton";this.scripts.OnClick()
    assert(opened==slot,"An unknown equipped item incorrectly blocked its editor")
end
equipped={201,204,103};V:RefreshWeaponCards()
local priorRefreshes=refreshes
this=advancedToggle;this:SetChecked(1);this.scripts.OnClick()
assert(syncs==3 and dirty==2 and refreshes==priorRefreshes,"An unchanged mode switch must not dirty or resynchronize the look")
-- A still-loaded older DLL cannot partially enable the carried behavior.
SaureksClosetRendererVersion=function() return 30702 end
V:RefreshWeaponCards()
assert(not advancedToggle.enabled and advancedToggle.alpha==.45)
for slot=101,107 do assert(not V.weaponCards[slot].enabled and not V.weaponCards[slot].gear.enabled) end
assert(not V:SetWeaponAdvancedMode(false) and saved.carriedEnabled and V.lastMessage,"An unsupported renderer must not silently alter the saved mode")
SaureksClosetRendererVersion=function() return 30704 end
V:RefreshWeaponCards()
print("PASS: mode changes preserve both hand appearances, restore drafts, gate unavailable equipment, and honor renderer compatibility")

local menuEntries={}
DropDownList1=CreateFrame("Frame","DropDownList1")
UIDropDownMenu_AddButton=function(entry) table.insert(menuEntries,entry) end
ToggleDropDownMenu=function(_,_,owner) menuEntries={};owner.initialize() end
V.RefreshSlotHighlights=function() end
local cleared
V.ClearSlot=function(_,slot) cleared=slot end
openSlotMenu(V,110)
assert(menuEntries[2].text=="Passthrough","The attacking reset choice must be named Passthrough")
menuEntries[2].func();assert(cleared==110,"Passthrough must clear the selected attacking appearance")
local ranged=saved[110];saved[110]=nil
this=V.slotButtons[110];this.scripts.OnEnter()
assert(GameTooltip.lines[1]=="Passthrough","An unset attacking icon needs the same Passthrough label")
saved[110]=ranged
openSlotMenu(V,106)
assert(menuEntries[2].text=="Remove carried item","Carried removal must remain distinct from attacking Passthrough")
print("PASS: Passthrough consistently names the attacking default and preserves carried removal semantics")

if arg[1] then
    -- Export actual geometry for a separate visual review; no screen mockup is
    -- substituted for the production UI controls in these assertions.
    local output=assert(io.open(arg[1],"w"))
    for _,advanced in ipairs({false,true}) do
        saved.carriedEnabled=advanced;V:RefreshWeaponCards()
        for _,n in ipairs(frames) do
            local ancestor=n;local include=false
            while ancestor do
                if ancestor==V.pagesByName.weaponry then include=true end
                ancestor=ancestor.parent
            end
            if include and n:IsVisible() then
                local x,y,w,h=rect(n)
                local text=n.text or (n.texture and type(n.texture[1])=="string" and n.texture[1]) or n.normal or ""
                text=string.gsub(text,"[\t\n]"," ")
                output:write(table.concat({advanced and "advanced" or "simple",n.kind,x,y,w,h,n.fontSize or 10,n.align or "LEFT",text},"\t").."\n")
            end
        end
    end
    output:close()
end

-- Build the actual header with the client's fixed-height panel texture regions.
-- A taller button frame alone must not pass this regression check.
UIParent=CreateFrame("Frame","UIParent");UIPanelWindows={}
for _,key in ipairs({"SetMovable","SetClampedToScreen","RegisterForDrag"}) do methods[key]=function() end end
PanelTemplates_TabResize=function(_,frame,width) frame:SetWidth(width) end
for _,name in ipairs({"CreateArmorPage","CreateBodyPage","CreateWeaponryPage","CreateBagsPage","CreateExposurePage","CreateOutfitPage","CreateSettingsPage","CreateBrowser","CreateOutfitDetails","CreateWardrobeSelector","SetTab"}) do
    V[name]=function() end
end
V:CreateUI()
local toggle=V.enabledButton
local x,y,w,h=rect(toggle)
local _,selectorY,_,selectorH=rect(V.outfitSelector)
assert(x==262 and y==42 and w==76,"The red addon toggle must retain its existing width and position")
assert(h==25 and y==selectorY and y+h==67 and selectorH==27,"The addon toggle must be two pixels shorter without moving the selector or top edge")
for _,part in ipairs({"Left","Middle","Right"}) do
    local region=getglobal(toggle:GetName()..part)
    local _,regionY,_,regionH=rect(region)
    assert(regionY==selectorY and regionH==25 and regionY+regionH==67,"The visible red artwork must move its bottom edge two pixels upward")
    assert(region.texture[1]=="Interface\\Buttons\\UI-Panel-Button-Up","The addon toggle must retain the native red artwork")
end
this=toggle;this.scripts.OnMouseDown()
for _,part in ipairs({"Left","Middle","Right"}) do
    local region=getglobal(toggle:GetName()..part)
    assert(region.texture[1]=="Interface\\Buttons\\UI-Panel-Button-Down" and region.height==25)
end
this.scripts.OnMouseUp()
for _,part in ipairs({"Left","Middle","Right"}) do
    local region=getglobal(toggle:GetName()..part)
    assert(region.texture[1]=="Interface\\Buttons\\UI-Panel-Button-Up" and region.height==25)
end
print("PASS: native red button artwork ends at y=67 in both pressed and released states; header width and top edge are unchanged")
