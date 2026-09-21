-- Saurek's Closet: original 1.12 character-sheet art and Blizzard widget templates.
local V=VanityStudio
local icons={[1]="Head",[3]="Shoulder",[4]="Shirt",[5]="Chest",[6]="Waist",[7]="Legs",[8]="Feet",[9]="Wrists",[10]="Hands",[15]="Chest",[16]="MainHand",[17]="SecondaryHand",[18]="Ranged",[19]="Tabard"}
local serial=0
local art="Interface\\AddOns\\SaureksCloset\\Textures\\"
local function label(parent,text,x,y,w,h,white)
    local f=parent:CreateFontString(nil,"OVERLAY",white and "GameFontHighlightSmall" or "GameFontNormal")
    f:SetPoint("TOPLEFT",parent,"TOPLEFT",x,-y);f:SetWidth(w);f:SetHeight(h or 16)
    f:SetJustifyH("LEFT");f:SetJustifyV("TOP");f:SetText(text)
    return f
end
local function paintButton(b,state)
    for _,part in ipairs({"Left","Middle","Right"}) do
        getglobal(b:GetName()..part):SetTexture("Interface\\Buttons\\UI-Panel-Button-"..state)
    end
end
local function enabled(b,on)
    if on then b:Enable() else b:Disable() end
    b.closetEnabled=on and true or false
    if b.closetPanel then paintButton(b,on and "Up" or "Disabled") end
end
local function button(parent,text,x,y,w,callback,name)
    serial=serial+1
    local b=CreateFrame("Button",name or ("SaureksClosetButton"..serial),parent,"UIPanelButtonTemplate2")
    b.closetPanel=true;b.closetEnabled=true
    b:SetWidth(w);b:SetHeight(22);b:SetPoint("TOPLEFT",parent,"TOPLEFT",x,-y)
    b:SetText(text);b:SetScript("OnClick",callback)
    local t=getglobal(b:GetName().."Text")
    t:ClearAllPoints();t:SetPoint("CENTER",b,"CENTER",0,0);t:SetWidth(w-12);t:SetHeight(16)
    t:SetJustifyH("CENTER");t:SetJustifyV("MIDDLE")
    b:SetScript("OnMouseDown",function() if this.closetEnabled then paintButton(this,"Down") end end)
    b:SetScript("OnMouseUp",function() paintButton(this,this.closetEnabled and "Up" or "Disabled") end)
    return b
end
-- Full native page buttons retain the original frame, arrow size and alignment.
local function arrow(parent,direction,x,y,callback,name)
    serial=serial+1
    local b=CreateFrame("Button",name or ("SaureksClosetArrow"..serial),parent)
    b:SetWidth(32);b:SetHeight(32);b:SetPoint("TOPLEFT",parent,"TOPLEFT",x,-y)
    local path="Interface\\Buttons\\UI-SpellbookIcon-"..direction.."Page-"
    b:SetNormalTexture(path.."Up");b:SetPushedTexture(path.."Down");b:SetDisabledTexture(path.."Disabled")
    b:SetHighlightTexture("Interface\\Buttons\\UI-Common-MouseHilight","ADD")
    b:SetScript("OnClick",callback)
    return b
end
local function edit(parent,name,x,y,w,limit)
    local e=CreateFrame("EditBox",name,parent,"InputBoxTemplate")
    e:SetPoint("TOPLEFT",parent,"TOPLEFT",x,-y);e:SetWidth(w);e:SetHeight(22)
    e:SetAutoFocus(false);e:SetMaxLetters(limit or 80)
    e:SetScript("OnEscapePressed",function() this:ClearFocus() end)
    e:SetScript("OnEnterPressed",function() this:ClearFocus() end)
    return e
end
local function texture(parent,path,x,y,w,h,layer)
    local t=parent:CreateTexture(nil,layer or "BACKGROUND")
    t:SetTexture(path);t:SetPoint("TOPLEFT",parent,"TOPLEFT",x,-y);t:SetWidth(w);t:SetHeight(h)
    return t
end
local function sheet(name,parent,title)
    local f=CreateFrame("Frame",name,parent)
    f:SetWidth(384);f:SetHeight(512);f:EnableMouse(true)
    f:SetHitRectInsets(0,30,0,75)
    local path="Interface\\PaperDollInfoFrame\\UI-Character-General-"
    texture(f,path.."TopLeft",0,0,256,256)
    texture(f,path.."TopRight",256,0,128,256)
    texture(f,path.."BottomLeft",0,256,256,256)
    texture(f,path.."BottomRight",256,256,128,256)
    f.portrait=texture(f,art.."Logo.tga",7,6,60,60,"ARTWORK")
    f.portrait:SetTexture(art.."Logo.tga")
    -- CharacterNameText uses GameFontNormal, white, centered at y=24.
    f.title=f:CreateFontString(nil,"OVERLAY","GameFontNormal")
    f.title:SetWidth(230);f.title:SetHeight(16)
    f.title:SetPoint("CENTER",f,"TOPLEFT",198,-24)
    f.title:SetTextColor(1,1,1);f.title:SetJustifyH("CENTER");f.title:SetJustifyV("MIDDLE");f.title:SetText(title)
    local close=CreateFrame("Button",name.."CloseButton",f,"UIPanelCloseButton")
    close:SetPoint("CENTER",f,"TOPRIGHT",-44,-25)
    f.close=close
    return f
end
local function page(parent)
    local p=CreateFrame("Frame",nil,parent)
    p:SetAllPoints(parent)
    return p
end
local function section(parent,x,y,w,h,stone,kind,opacity)
    local f=CreateFrame(kind or "Frame",nil,parent)
    f:SetPoint("TOPLEFT",parent,"TOPLEFT",x,-y);f:SetWidth(w);f:SetHeight(h)
    f:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",
        edgeFile="Interface\\Tooltips\\UI-Tooltip-Border",tile=true,tileSize=16,edgeSize=12,
        insets={left=3,right=3,top=3,bottom=3}})
    f:SetBackdropBorderColor(.6,.55,.45)
    if stone and opacity then f:SetBackdropColor(1,1,1,0) end
    if stone then
        -- Tile the native finish at its actual scale; mirror adjacent tiles to avoid seams.
        local i=0
        for x=4,w-5,124 do
            local width=math.min(124,w-4-x);local left=68;local right=68+width
            if math.mod(i,2)==1 then left=192;right=192-width end
            local t=texture(f,"Interface\\PaperDoll\\UI-PaperDoll-SlotBackground",x,4,width,h-8)
            t:SetTexCoord(left/256,right/256,32/256,(32+h-8)/256)
            if opacity then t:SetAlpha(opacity) end
            i=i+1
        end
    end
    return f
end
local function textWidth(parent,text,size)
    local measure=label(parent,text,0,0,0,18,true)
    measure:SetFont("Fonts\\FRIZQT__.TTF",size)
    local width=math.ceil(measure:GetWidth())
    measure:Hide()
    return width
end
local function ensureLeftFrameOverlay()
    if V.leftPaneFrameOverlay then return end
    local f=CreateFrame("Frame",nil,V.frame);V.leftPaneFrameOverlay=f
    f:SetPoint("TOPLEFT",V.frame,"TOPLEFT",0,0);f:SetWidth(26);f:SetHeight(512)
    f:SetFrameLevel(V.frame:GetFrameLevel()+30);f:EnableMouse(false)
    local path="Interface\\PaperDollInfoFrame\\UI-Character-General-"
    local top=texture(f,path.."TopLeft",0,0,26,256,"OVERLAY")
    top:SetTexCoord(0,26/256,0,1)
    local bottom=texture(f,path.."BottomLeft",0,256,26,256,"OVERLAY")
    bottom:SetTexCoord(0,26/256,0,1)
    f.parts={top,bottom};f:Hide()
end
-- Every character-options page uses the same pane inside the left side of the
-- character frame. The real window-edge art is redrawn above it, making the
-- pane sit beneath the existing frame rather than simulating that join.
local function sidebar(parent,height)
    ensureLeftFrameOverlay()
    local f=CreateFrame("Frame",nil,parent)
    -- Keep this tab wholly between the header controls and the bottom wardrobe
    -- selector. It must never become part of either navigation strip.
    height=height or 220
    f:SetPoint("TOPLEFT",parent,"TOPLEFT",19,-80);f:SetWidth(145);f:SetHeight(height)
    f:SetBackdrop({edgeFile="Interface\\Tooltips\\UI-Tooltip-Border",edgeSize=14,
        insets={left=0,right=4,top=4,bottom=4}})
    -- Use the exact dark CharacterFrame crop behind the Saved Looks list.
    -- It is fully opaque; the ornamental wardrobe border covers the first 8px.
    f.background=texture(f,"Interface\\PaperDollInfoFrame\\UI-Character-General-TopLeft",4,4,137,height-8,"BACKGROUND")
    f.background:SetTexCoord(64/256,244/256,80/256,250/256)
    f.background:SetAlpha(.8)
    f:SetBackdropBorderColor(.72,.64,.48,.95)
    f.sidebarHeight=height
    return f
end
local function sidebarGroup(parent,title,y,height)
    local box=CreateFrame("Frame",nil,parent)
    box:SetPoint("TOPLEFT",parent,"TOPLEFT",6,-y);box:SetWidth(136);box:SetHeight(height)
    local heading=label(box,title,15,5,106,14)
    heading:SetFont("Fonts\\FRIZQT__.TTF",11)
    heading:SetJustifyH("CENTER");heading:SetJustifyV("MIDDLE")
    if y>0 then
        local rule=box:CreateTexture(nil,"BORDER");rule:SetTexture(.55,.49,.35,.25)
        rule:SetPoint("TOPLEFT",box,"TOPLEFT",16,0);rule:SetWidth(104);rule:SetHeight(1)
    end
    return box
end
local function settingsButton(parent,text,x,y,w,callback,opacity,height)
    local fitText=not w
    if fitText then w=textWidth(parent,text,11)+66 end
    local b=section(parent,x,y,w,height or 27,true,"Button",opacity)
    b.caption=label(b,text,8,5,w-36,17,true)
    b.caption:SetFont("Fonts\\FRIZQT__.TTF",fitText and 11 or (w<150 and 10 or 12))
    b.caption:SetJustifyV("MIDDLE")
    b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
    local open=texture(b,"Interface\\Buttons\\UI-ScrollBar-ScrollDownButton-Up",w-25,2,22,22,"ARTWORK")
    -- Rotate the native dropdown arrow counterclockwise: down becomes right.
    open:SetTexCoord(1,0,0,0,1,1,0,1)
    b:SetScript("OnClick",callback)
    return b
end
local function itemName(id)
    if id==nil then return "Passing through real armor" end
    if id==0 then return "Hidden" end
    return V.index[id] and V.index[id][2] or "Item "..id
end
function V:MoveLauncher()
    local angle=(VanityStudioDB.minimapAngle or 225)*math.pi/180
    self.launcher:ClearAllPoints()
    self.launcher:SetPoint("CENTER",Minimap,"CENTER",math.cos(angle)*80,math.sin(angle)*80)
end
function V:CreateLauncher()
    if self.launcher then return end
    local b=CreateFrame("Button","SaureksClosetMinimapButton",Minimap)
    self.launcher=b;b:SetWidth(31);b:SetHeight(31);b:SetFrameStrata("MEDIUM")
    b:RegisterForClicks("LeftButtonUp");b:RegisterForDrag("LeftButton")
    b:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")
    local icon=b:CreateTexture(nil,"BACKGROUND");icon:SetWidth(20);icon:SetHeight(20)
    icon:SetPoint("TOPLEFT",b,"TOPLEFT",7,-5)
    icon:SetTexture(art.."MiniLogo.tga")
    texture(b,"Interface\\Minimap\\MiniMap-TrackingBorder",0,0,54,54,"OVERLAY")
    b:SetScript("OnClick",function() if arg1=="LeftButton" then V:Toggle() end end)
    b:SetScript("OnDragStart",function() this.dragging=true end)
    b:SetScript("OnDragStop",function() this.dragging=nil end)
    b:SetScript("OnUpdate",function()
        if not this.dragging then return end
        local x,y=GetCursorPosition();local cx,cy=Minimap:GetCenter();local scale=Minimap:GetEffectiveScale()
        VanityStudioDB.minimapAngle=math.atan2(y/scale-cy,x/scale-cx)*180/math.pi;V:MoveLauncher()
    end)
    b:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_LEFT");GameTooltip:SetText("Saurek's Closet",1,.82,0)
        GameTooltip:AddLine("Left-click: open wardrobe",1,1,1)
        GameTooltip:AddLine("Drag around the minimap.",1,1,1);GameTooltip:Show()
    end)
    b:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self:MoveLauncher()
end
function V:Toggle(show)
    if not self.ready then return end
    if not self.frame then self:CreateUI() end
    if self.frame:IsShown() and not show then HideUIPanel(self.frame)
    else
        -- Main-window dragging is deliberately session-only. Every fresh open
        -- returns to the original CharacterFrame-aligned position.
        if not self.frame:IsShown() then
            self.frame:ClearAllPoints();self.frame:SetPoint("TOPLEFT",UIParent,"TOPLEFT",0,-104)
        end
        ShowUIPanel(self.frame);self:Refresh()
    end
end
function V:SetTab(tab)
    if tab=="character" then tab=self.wardrobePage or "armor" end
    if not self.pagesByName or not self.pagesByName[tab] then return end
    local wasCharacter=self.pagesByName.armor:IsVisible()
    if tab~="weaponry" then self:CloseWeaponOptions() end
    self:CloseOutfitMenu();self:CloseOutfitDetails();self:CloseBrowser();self.tab=tab
    self.selectedOutfit=nil;self.confirmDelete=nil
    local character=tab=="armor" or tab=="body" or tab=="weaponry" or tab=="bags" or tab=="exposure"
    local sideControls=tab=="body" or tab=="weaponry" or tab=="bags" or tab=="exposure"
    local leftPane=tab=="body" or tab=="weaponry" or tab=="bags"
    if character then self.wardrobePage=tab end
    for name,p in pairs(self.pagesByName) do
        if name==tab or (name=="armor" and character) then p:Show() else p:Hide() end
    end
    local primary=character and "character" or tab
    for name,b in pairs(self.tabButtons) do
        if name==primary then PanelTemplates_SelectTab(b) else PanelTemplates_DeselectTab(b) end
    end
    if character then
        self.wardrobeSelectorBox:Show()
        self.wardrobeSelectorLabel:SetText(({armor="Outfit",body="Body",weaponry="Weaponry",bags="Bags",exposure="Exposure"})[tab])
    else self.wardrobeSelectorBox:Hide() end
    if self.leftPaneFrameOverlay then
        if leftPane then self.leftPaneFrameOverlay:Show() else self.leftPaneFrameOverlay:Hide() end
    end
    if tab=="armor" then
        self.armorDecorationFrame:Show();self.armorShadowFrame:Show()
    else
        self.armorDecorationFrame:Hide();self.armorShadowFrame:Hide()
    end
    -- The non-Outfit wardrobe pages share the ornamental character-area
    -- frame. Outfit keeps its purpose-built equipment-slot decoration.
    if self.wardrobeViewBorder then
        if sideControls then self.wardrobeViewBorder:Show() else self.wardrobeViewBorder:Hide() end
    end
    if self.wardrobeViewShadowFrame then
        if sideControls then self.wardrobeViewShadowFrame:Show() else self.wardrobeViewShadowFrame:Hide() end
    end
    local modelX=sideControls and 96 or 61
    for _,model in ipairs({self.model,self.previewBuffer}) do
        model:ClearAllPoints();model:SetPoint("TOPLEFT",self.pagesByName.armor,"TOPLEFT",modelX,-86)
        model:SetWidth(244);model:SetHeight(340)
    end
    if character and not wasCharacter then self:HidePreviewUntilReady();self:InvalidatePreviewModel(0,true) end
    self:Refresh()
end
function V:CreateUI()
    self.uiReady=false
    self.slot=1;self.query="";self.offset=0;self.previewRequests={}
    local f=sheet("VanityStudioFrame",UIParent,"Saurek's Closet")
    self.frame=f;f:Hide();f:SetFrameStrata("MEDIUM")
    f.close:ClearAllPoints();f.close:SetPoint("CENTER",f,"TOPRIGHT",-46,-24)
    f:SetPoint("TOPLEFT",UIParent,"TOPLEFT",0,-104)
    f:SetMovable(true);f:SetClampedToScreen(true);f:RegisterForDrag("LeftButton")
    f:SetScript("OnDragStart",function() this:StartMoving() end)
    f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
    -- Reserve both native panel columns while the item browser is open.
    UIPanelWindows[f:GetName()]={area="left",pushable=0,whileDead=1}
    f.close:SetScript("OnClick",function() HideUIPanel(V.frame) end)
    f:SetScript("OnShow",function()
        V:HidePreviewUntilReady();V:RefreshPortraits();V:InvalidatePreviewModel(0,true);V:Refresh()
    end)
    f:SetScript("OnHide",function()
        if UIParent.doublewide==V.frame then UIParent.doublewide=nil end
        V:CloseOutfitMenu();V:CloseOutfitDetails();V:CloseBrowser();V:CloseWeaponOptions();V:CancelDraft()
        if V.outfitName then V.outfitName:ClearFocus() end
    end)
    table.insert(UISpecialFrames,f:GetName())
    local outfitBox=section(f,74,42,178,27,true)
    self.activeOutfitLabel=label(outfitBox,"",8,5,140,17,true)
    self.activeOutfitLabel:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.activeOutfitLabel:SetJustifyV("MIDDLE")
    local selector=CreateFrame("Button","SaureksClosetOutfitSelector",outfitBox)
    self.outfitSelector=selector;selector:SetAllPoints(outfitBox)
    selector:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
    texture(selector,"Interface\\Buttons\\UI-ScrollBar-ScrollDownButton-Up",149,1,26,26,"ARTWORK")
    selector:SetScript("OnClick",function() V:ToggleOutfitMenu() end)
    selector:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT")
        GameTooltip:SetText(V:ActiveOutfitText(),1,1,1)
        GameTooltip:AddLine("Click to switch saved outfits.",1,.82,0);GameTooltip:Show()
    end)
    selector:SetScript("OnLeave",function() GameTooltip:Hide() end)
    -- Restore the original master-toggle position with the same dark inset
    -- treatment previously used by the outfit count.
    local toggle=section(f,262,42,76,27,true,"Button")
    self.enabledButton=toggle
    toggle.caption=label(toggle,"",6,5,64,17)
    toggle.caption:ClearAllPoints();toggle.caption:SetPoint("CENTER",toggle,"CENTER",0,0)
    toggle.caption:SetFont("Fonts\\FRIZQT__.TTF",11)
    toggle.caption:SetJustifyH("CENTER");toggle.caption:SetJustifyV("MIDDLE")
    local toggleHover=texture(toggle,"Interface\\QuestFrame\\UI-QuestTitleHighlight",3,3,70,21,"ARTWORK")
    toggleHover:SetBlendMode("ADD");toggleHover:SetAlpha(.45);toggleHover:Hide()
    toggle:SetScript("OnClick",function() V:SetEnabled(not VanityStudioCharacter.enabled) end)
    toggle:SetScript("OnEnter",function()
        toggleHover:Show()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT")
        GameTooltip:SetText("Toggle the addon completely on/off.",1,1,1);GameTooltip:Show()
    end)
    toggle:SetScript("OnLeave",function() toggleHover:Hide();GameTooltip:Hide() end)
    toggle:SetScript("OnHide",function() toggleHover:Hide() end)
    -- ToggleDropDownMenu accepts a named owner with an initializer and explicit anchor.
    self.outfitMenu=CreateFrame("Frame","SaureksClosetOutfitMenu",f)
    self.outfitMenu.displayMode="MENU";self.outfitMenu:Hide()
    self.outfitMenu.initialize=function(level) V:BuildOutfitMenu(level) end
    self.pagesByName={armor=page(f),body=page(f),weaponry=page(f),bags=page(f),exposure=page(f),outfits=page(f),settings=page(f)}
    self.tabButtons={}
    local function fitTab(b)
        local text=getglobal(b:GetName().."Text")
        -- Measure without the prior width constraint so a wrapped label cannot shrink the tab.
        text:SetWidth(0)
        local width=math.ceil(b:GetTextWidth())+12
        local ends=2*getglobal(b:GetName().."Left"):GetWidth()
        PanelTemplates_TabResize(0,b,width+ends)
    end
    local previous
    for i,name in ipairs({"character","outfits","settings"}) do
        local b=CreateFrame("Button",f:GetName().."Tab"..i,f,"CharacterFrameTabButtonTemplate")
        b:SetText(({character="Wardrobe",outfits="Saved Looks",settings="Settings"})[name])
        if previous then b:SetPoint("TOPLEFT",previous,"TOPRIGHT",-12,0)
        else b:SetPoint("TOPLEFT",f,"TOPLEFT",13,-434) end
        fitTab(b)
        b.tab=name
        -- Replace the inherited callback that switches Blizzard's CharacterFrame.
        b:SetScript("OnClick",function() V:SetTab(this.tab) end)
        b:SetScript("OnShow",function() fitTab(this) end)
        if name=="outfits" then self.savedLooksTab=b end
        self.tabButtons[name]=b;previous=b
    end
    self:CreateArmorPage(self.pagesByName.armor)
    self:CreateBodyPage(self.pagesByName.body)
    self:CreateWeaponryPage(self.pagesByName.weaponry)
    self:CreateBagsPage(self.pagesByName.bags)
    self:CreateExposurePage(self.pagesByName.exposure)
    self:CreateOutfitPage(self.pagesByName.outfits)
    self:CreateSettingsPage(self.pagesByName.settings)
    self:CreateBrowser()
    self:CreateOutfitDetails()
    self:CreateWardrobeSelector()
    self.uiReady=true
    self:SetTab("armor")
end
function V:CreateWardrobeSelector()
    local box=section(self.frame,0,392,106,27,true,nil,.75)
    -- Center the selector and both rotation buttons as one control group.
    -- The selector sits to the left; the two rotation buttons stay together
    -- on its right on every wardrobe page.
    box:ClearAllPoints();box:SetPoint("TOP",self.frame,"TOP",-44,-392)
    box:SetFrameLevel(self.frame:GetFrameLevel()+10)
    self.wardrobeSelectorBox=box
    -- Keep the rotation controls vertically aligned with the selector.
    self.rotationControls:ClearAllPoints()
    self.rotationControls:SetWidth(66)
    self.rotationControls:SetPoint("LEFT",box,"RIGHT",4,-1)
    self.wardrobeSelectorLabel=label(box,"Outfit",8,5,72,17,true)
    self.wardrobeSelectorLabel:SetFont("Fonts\\FRIZQT__.TTF",11)
    self.wardrobeSelectorLabel:SetJustifyV("MIDDLE")
    local b=CreateFrame("Button","SaureksClosetWardrobeSelector",box)
    self.wardrobeSelector=b;b:SetAllPoints(box)
    local hover=texture(b,"Interface\\QuestFrame\\UI-QuestTitleHighlight",4,4,98,19,"OVERLAY")
    hover:SetBlendMode("ADD");hover:SetAlpha(.45);hover:Hide()
    b:SetScript("OnEnter",function() hover:Show() end)
    b:SetScript("OnLeave",function() hover:Hide() end)
    b:SetScript("OnHide",function() hover:Hide() end)
    texture(b,"Interface\\Buttons\\UI-ScrollBar-ScrollDownButton-Up",78,1,26,26,"ARTWORK")
    local menu=CreateFrame("Frame","SaureksClosetWardrobeMenu",self.frame)
    self.wardrobeMenu=menu;menu.displayMode="MENU";menu:Hide()
    menu.initialize=function()
        local function choice(text,page)
            UIDropDownMenu_AddButton({text=text,checked=V.tab==page and 1 or nil,
                func=function() V:SetTab(page) end})
        end
        choice("Outfit","armor");choice("Weaponry","weaponry");choice("Body","body");choice("Bags","bags");choice("Exposure","exposure")
    end
    b:SetScript("OnClick",function() ToggleDropDownMenu(1,nil,menu,b:GetName(),0,0) end)
end
function V:CreateArmorPage(p)
    -- Reuse the original baked background; inset only its right edge by 5px.
    self.wardrobeBackgrounds={texture(p,art.."Main.blp",19,75,323,355)}
    local m=CreateFrame("DressUpModel","SaureksClosetModel",p)
    self.model=m;m:SetPoint("TOPLEFT",p,"TOPLEFT",61,-86);m:SetWidth(244);m:SetHeight(340)
    m.rotation=.61
    local buffer=CreateFrame("DressUpModel","SaureksClosetModelBuffer",p)
    buffer:SetPoint("TOPLEFT",p,"TOPLEFT",61,-86);buffer:SetWidth(244);buffer:SetHeight(340);buffer:SetAlpha(0)
    self.previewBuffer=buffer
    m:SetFrameLevel(p:GetFrameLevel()+2);buffer:SetFrameLevel(p:GetFrameLevel()+2)
    -- This artwork is split across four power-of-two TGA files for the 1.12
    -- client. Keep it above every embedded menu and window-edge overlay so no
    -- sidebar can obscure the ornamental frame.
    local viewBorder=CreateFrame("Frame","SaureksClosetWardrobeViewBorder",p)
    self.wardrobeViewBorder=viewBorder
    viewBorder:SetPoint("TOPLEFT",p,"TOPLEFT",19,-75)
    viewBorder:SetWidth(327);viewBorder:SetHeight(357)
    viewBorder:SetFrameLevel(self.frame:GetFrameLevel()+50)
    viewBorder:EnableMouse(false)
    viewBorder.parts={}
    -- Use Outfit's exact four-tile geometry, stretch, edge crop and tint.
    for _,part in ipairs({{"TL",0,0,164},{"TR",164,0,163},
        {"BL",0,357/2,164},{"BR",164,357/2,163}}) do
        local border=texture(viewBorder,art.."WardrobeFrameCrop"..part[1]..".tga",part[2],part[3],part[4],357/2,"OVERLAY")
        if part[2]>0 then
            border:SetWidth(part[4]-1);border:SetTexCoord(0,(part[4]-1)/part[4],0,1)
        end
        border:SetVertexColor(.82,.88,1)
        table.insert(viewBorder.parts,border)
    end
    viewBorder:Hide()
    -- Match Outfit's separate silhouette shadow exactly: 3px right, 4px down,
    -- 45% opacity, the same right-edge clip, and beneath both model buffers.
    local viewShadow=CreateFrame("Frame",nil,p)
    self.wardrobeViewShadowFrame=viewShadow
    viewShadow:SetAllPoints(p);viewShadow:SetFrameLevel(p:GetFrameLevel()+1)
    for _,part in ipairs({{"TL",0,0,164},{"TR",164,0,163},
        {"BL",0,357/2,164},{"BR",164,357/2,163}}) do
        local shadow=texture(viewShadow,art.."WardrobeFrameShadowCrop"..part[1]..".tga",22+part[2],79+part[3],part[4],357/2,"BACKGROUND")
        if part[2]>0 then
            shadow:SetWidth(part[4]-4);shadow:SetTexCoord(0,(part[4]-4)/part[4],0,1)
        end
        shadow:SetAlpha(.45)
    end
    viewShadow:Hide()
    local controls=CreateFrame("Frame",nil,p);self.rotationControls=controls;controls:SetFrameLevel(self.frame:GetFrameLevel()+14);controls:SetWidth(66);controls:SetHeight(35)
    -- No keyboard handlers or character/equipment mouse callbacks.
    for i,direction in ipairs({"Left","Right"}) do
        local b=CreateFrame("Button",m:GetName().."Rotate"..direction.."Button",controls)
        local edge=direction=="Left" and "LEFT" or "RIGHT"
        b:SetPoint(edge,controls,edge,0,0);b:SetWidth(35);b:SetHeight(35)
        b:SetHitRectInsets(2,2,0,0)
        b:SetNormalTexture("Interface\\Buttons\\UI-Rotation"..direction.."-Button-Up")
        b:SetPushedTexture("Interface\\Buttons\\UI-Rotation"..direction.."-Button-Down")
        b:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Round","ADD")
        b:RegisterForClicks("LeftButtonDown","LeftButtonUp")
        b.direction=direction
        setglobal(buffer:GetName().."Rotate"..direction.."Button",b)
        b:SetScript("OnClick",function()
            if this.direction=="Left" then Model_RotateLeft(V.model) else Model_RotateRight(V.model) end
        end)
    end
    m:SetScript("OnUpdate",function() Model_OnUpdate(arg1,V.model) end)
    self.slotButtons={};self.wardrobeSlots={1,3,15,5,4,19,9,10,6,7,8}
    local leftWidth,rightWidth,decorationHeight=164,163,357
    local slotLayer=CreateFrame("Frame","SaureksClosetArmorDecorations",p)
    self.armorDecorationFrame=slotLayer
    slotLayer:SetPoint("TOPLEFT",p,"TOPLEFT",19,-75)
    slotLayer:SetWidth(leftWidth+rightWidth);slotLayer:SetHeight(decorationHeight);slotLayer:SetAlpha(1)
    slotLayer:SetFrameLevel(math.max(m:GetFrameLevel(),buffer:GetFrameLevel())+1)
    -- Shadow follows the alpha silhouette of the complete decoration, behind the models.
    local shadowLayer=CreateFrame("Frame",nil,p);self.armorShadowFrame=shadowLayer
    shadowLayer:SetAllPoints(p)
    for _,part in ipairs({{"TL",0,0,leftWidth},{"TR",leftWidth,0,rightWidth},{"BL",0,decorationHeight/2,leftWidth},{"BR",leftWidth,decorationHeight/2,rightWidth}}) do
        local shadow=texture(shadowLayer,art.."ArmorShadow"..part[1]..".tga",22+part[2],79+part[3],part[4],decorationHeight/2,"BACKGROUND")
        -- Clip to the same right edge as the decoration, accounting for its 3px shadow offset.
        if part[2]>0 then
            shadow:SetWidth(part[4]-4);shadow:SetTexCoord(0,(part[4]-4)/part[4],0,1)
        end
        shadow:SetAlpha(.45)
    end
    -- The shadow must be below both preview buffers, not above the character.
    shadowLayer:SetFrameLevel(p:GetFrameLevel()+1)
    self.armorSlotPanels={}
    for _,part in ipairs({{"TL",0,0,leftWidth},{"TR",leftWidth,0,rightWidth},{"BL",0,decorationHeight/2,leftWidth},{"BR",leftWidth,decorationHeight/2,rightWidth}}) do
        local t=texture(slotLayer,art.."ArmorSlots"..part[1]..".tga",part[2],part[3],part[4],decorationHeight/2,"ARTWORK")
        -- Crop the last UI pixel; retain texture scale, anchors and slot positions.
        if part[2]>0 then
            t:SetWidth(part[4]-1);t:SetTexCoord(0,(part[4]-1)/part[4],0,1)
        end
        -- Cool and mute the warm gold toward the native window's metal finish.
        t:SetVertexColor(.82,.88,1)
        t:SetAlpha(1);table.insert(self.armorSlotPanels,t)
    end
    -- Centers of the slot openings in armor_slot_decorations.png.
    local positions={};local iconSize=32.0045
    local function slotAt(slot,x,y)
        local offset=x<=600 and x*leftWidth/600 or leftWidth+(x-600)*rightWidth/600
        positions[slot]={19+offset-iconSize/2,75+y*decorationHeight/1310-iconSize/2}
    end
    local leftY={86,236.5,387,537,688,840.5,991.5}
    local rightY={86,236.5,387,538.5}
    for i,slot in ipairs({1,3,15,5,4,19,9}) do slotAt(slot,80.5,leftY[i]) end
    for i,slot in ipairs({10,6,7,8}) do slotAt(slot,1115.5,rightY[i]) end
    for _,slot in ipairs(self.wardrobeSlots) do
        local pos=positions[slot]
        local b=self:CreateSlotButton(slotLayer,p,slot,pos[1],pos[2],iconSize)
        b:SetFrameLevel(slotLayer:GetFrameLevel()+1)
        -- A subtle additive pass brightens only the actual item rim.
        b.borderLight=b:CreateTexture(nil,"ARTWORK")
        b.borderLight:SetTexture("Interface\\Buttons\\UI-Quickslot2")
        b.borderLight:SetAllPoints(b.border)
        b.borderLight:SetBlendMode("ADD");b.borderLight:SetAlpha(.20)
    end
    -- A sibling frame above BOTH model buffers, anchored just above the weapons.
    local note=CreateFrame("Frame",nil,p);self.previewNoteOverlay=note
    note:SetWidth(240);note:SetHeight(38)
    note:SetPoint("BOTTOM",p,"TOPLEFT",183,-390)
    note:SetFrameLevel(math.max(m:GetFrameLevel(),buffer:GetFrameLevel())+1)
    note:EnableMouse(false)
    self.previewNote=label(note,"",0,0,240,38,true)
    self.previewNote:SetJustifyH("CENTER");self.previewNote:SetJustifyV("BOTTOM")


end
function V:CreateSlotButton(parent,anchor,slot,x,y,iconSize)
    local b=CreateFrame("Button","SaureksClosetSlot"..slot,parent)
    b:SetPoint("TOPLEFT",anchor,"TOPLEFT",x,-y);b:SetWidth(iconSize);b:SetHeight(iconSize);b.slot=slot
    b.icon=texture(b,"Interface\\Icons\\INV_Misc_QuestionMark",0,0,iconSize,iconSize,"BORDER")
    -- Remove the icon's baked bevel and use the native character-sheet item rim.
    b.icon:SetTexCoord(.08,.92,.08,.92)
    local borderSize=iconSize*64/37
    b.border=texture(b,"Interface\\Buttons\\UI-Quickslot2",0,0,borderSize,borderSize,"ARTWORK")
    b.border:ClearAllPoints();b.border:SetPoint("CENTER",b,"CENTER",0,-iconSize/37)
    b.hiddenOverlay=texture(b,art.."HiddenSlot.tga",-1,0,iconSize,iconSize,"OVERLAY")
    b.hiddenOverlay:Hide()
    b.selected=texture(b,"Interface\\Buttons\\CheckButtonHilight",-3,-3,iconSize+6,iconSize+6,"OVERLAY")
    b.selected:SetBlendMode("ADD")
    b:RegisterForClicks("LeftButtonUp","RightButtonUp")
    b:SetScript("OnClick",function() V:OpenSlotMenu(this.slot) end)
    b:SetScript("OnEnter",function()
        local id=V:SlotSelection(this.slot)
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText(V.slotNames[this.slot],1,.82,0)
        GameTooltip:AddLine(V:IsWeaponPosition(this.slot) and not id and "Game Default" or itemName(id),1,1,1)
        if this.slot==103 then GameTooltip:AddLine("Normal back weapon placement.",1,1,1)
        elseif this.slot==104 then GameTooltip:AddLine("Alternate back placement for a second weapon.",1,1,1) end
        GameTooltip:Show()
    end)
    b:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self.slotButtons[slot]=b
    return b
end
function V:CreateWeaponryPage(p)
    p:SetFrameLevel(self.frame:GetFrameLevel()+12)
    local rail=sidebar(p,253);self.weaponSidebar=rail
    local function group(title,y,height)
        return sidebarGroup(rail,title,y,height)
    end
    local function choice(box,slot,title,column,y)
        local x=column==1 and 25 or 83
        self:CreateSlotButton(box,box,slot,x,y,28)
        local caption=label(box,title,x-8,y+30,44,14,true)
        caption:SetFont("Fonts\\FRIZQT__.TTF",10)
        caption:SetTextColor(.82,.82,.82)
        caption:SetJustifyH("CENTER");caption:SetJustifyV("MIDDLE")
    end
    -- Paired positions read left-to-right; all choices stay in a compact rail
    -- beside the model and above the wardrobe selector.
    local waist=group("Waist",8,72)
    choice(waist,101,"Left",1,25);choice(waist,102,"Right",2,25)
    local back=group("Back",84,116)
    choice(back,103,"Primary",1,25);choice(back,104,"Alt",2,25)
    choice(back,105,"Shield",1,71);choice(back,106,"Ranged",2,71)
    local quiver=CreateFrame("Frame",nil,rail)
    quiver:SetPoint("TOPLEFT",rail,"TOPLEFT",6,-206);quiver:SetWidth(136);quiver:SetHeight(45)
    self:CreateSlotButton(quiver,quiver,107,25,9,28)
    local quiverTitle=label(quiver,"Quiver",66,15,55,16)
    quiverTitle:SetFont("Fonts\\FRIZQT__.TTF",11);quiverTitle:SetJustifyV("MIDDLE")
    self.weaponOptionsButton=settingsButton(p,"Settings",0,90,nil,function() V:OpenWeaponOptions() end,.75,26)
    self.weaponOptionsButton:ClearAllPoints()
    self.weaponOptionsButton:SetPoint("TOPLEFT",p,"TOPLEFT",336-self.weaponOptionsButton:GetWidth(),-90)
    self:CreateWeaponOptions()
end
function V:CreateWeaponOptions()
    local f=sheet("SaureksClosetWeaponOptions",self.frame,"Weapon Settings")
    self.weaponOptions=f;f:Hide();f:SetPoint("TOPLEFT",self.frame,"TOPLEFT",384,0)
    f.close:SetScript("OnClick",function() V:CloseWeaponOptions() end)
    f:SetScript("OnHide",function() GameTooltip:Hide();V:ReleaseNeighbor() end)
    table.insert(UISpecialFrames,f:GetName())
    local stored=section(f,23,90,318,94,true)
    self.weaponStoredOptions=stored
    label(stored,"Stored weapons",12,8,290,18)
    local function storedChoice(kind,field,text,y,tooltip)
        local name=kind=="ranged" and "SaureksClosetHideRangedWhenStored" or "SaureksClosetHideMeleeWhenStored"
        local checkbox=CreateFrame("CheckButton",name,stored,"UICheckButtonTemplate")
        self[field.."Checkbox"]=checkbox;checkbox.kind=kind
        checkbox:SetPoint("TOPLEFT",stored,"TOPLEFT",8,-y);checkbox:SetWidth(24);checkbox:SetHeight(24)
        checkbox:SetHitRectInsets(0,-278,0,0)
        local caption=label(stored,text,38,y+1,270,22,true)
        self[field.."Label"]=caption
        caption:SetFont("Fonts\\FRIZQT__.TTF",11);caption:SetJustifyV("MIDDLE")
        checkbox:SetScript("OnClick",function() V:SetWeaponStoredHidden(this.kind,this:GetChecked()) end)
        checkbox:SetScript("OnEnter",function()
            GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:ClearLines()
            GameTooltip:AddLine(tooltip,1,1,1,true);GameTooltip:Show()
        end)
        checkbox:SetScript("OnLeave",function() GameTooltip:Hide() end)
    end
    storedChoice("ranged","hideRangedWhenStored","Hide ranged weapon when not in use",30,
        "Hide your ranged weapon while it is stored. It remains visible while drawn.")
    storedChoice("melee","hideMeleeWhenStored","Hide melee weapons when not in use",60,
        "Hide melee weapons while they are stored. Drawn weapons and shields remain visible.")
    local placement=section(f,23,198,318,74,true)
    label(placement,"Placement",12,8,290,18)
    local help=label(placement,"Right-click a weapon or quiver slot and choose Placement Tuner to adjust its position.",12,30,290,36,true)
    help:SetFont("Fonts\\FRIZQT__.TTF",10);help:SetTextColor(.8,.8,.8)
    self.weaponNotice=label(f,"",29,286,306,40,true)
    self.weaponNotice:SetFont("Fonts\\FRIZQT__.TTF",10)
end
function V:OpenWeaponOptions()
    if not self.weaponOptions or not self.frame:IsVisible() or self.tab~="weaponry" then return end
    self.weaponOptions:Show()
    if self.browser and self.browser:IsShown() then self:CloseBrowser() end
    self:SetPanelArea("doublewide");self:Refresh()
end
function V:CloseWeaponOptions()
    if self.weaponOptions then self.weaponOptions:Hide() end
    self:ReleaseNeighbor()
end
function V:CreateBagsPage(p)
    p:SetFrameLevel(self.frame:GetFrameLevel()+12)
    local rail=sidebar(p,246);self.bagSidebar=rail
    local back=sidebarGroup(rail,"Back",8,152)
    local hips=sidebarGroup(rail,"Hips",168,76)
    self.bagDescription=nil
    self.bagSlots={}
    local positions={{"Top Left",back,25,31},{"Center",back,56,70},{"Top Right",back,87,31},
        {"Bottom Left",back,25,108},{"Bottom Right",back,87,108},{"Left Hip",hips,25,32},{"Right Hip",hips,87,32}}
    for i,entry in ipairs(positions) do
        local b=self:CreateSlotButton(entry[2],entry[2],200+i,entry[3],entry[4],24)
        self.slotButtons[200+i]=nil;self.bagSlots[i]=b;b.selected:Hide()
        b.icon:SetTexture("Interface\\PaperDoll\\UI-PaperDoll-Slot-Bag")
        local caption=label(entry[2],entry[1],entry[3]-13,entry[4]+25,50,13,true)
        caption:SetFont("Fonts\\FRIZQT__.TTF",9);caption:SetJustifyH("CENTER")
        b.bagTitle=entry[1];b.bagIndex=i
        b:SetScript("OnEnter",function()
            GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText(this.bagTitle,1,.82,0)
            local text=this.bagIndex==1 and ((VanityStudioCharacter.weapons or {}).backBag==1 and "Runecloth Bag" or "Choose a visible bag.") or "Not implemented yet."
            GameTooltip:AddLine(text,1,1,1);GameTooltip:Show()
        end)
        b:SetScript("OnClick",function() if this.bagIndex==1 then V:OpenBackBagMenu() end end)
        if i~=1 then b:Disable();b:SetAlpha(.4);caption:SetAlpha(.4) end
    end
    local menu=CreateFrame("Frame","SaureksClosetBackBagMenu",self.frame)
    self.backBagMenu=menu;menu.displayMode="MENU";menu:Hide()
    menu.initialize=function()
        local id=(VanityStudioCharacter.weapons or {}).backBag
        UIDropDownMenu_AddButton({text="None",checked=not id and 1 or nil,func=function() V:SelectBackBag(nil) end})
        UIDropDownMenu_AddButton({text="Runecloth Bag",checked=id==1 and 1 or nil,func=function() V:SelectBackBag(1) end})
        UIDropDownMenu_AddButton({text="Placement Tuner",notCheckable=1,func=function() CloseDropDownMenus();V:OpenBagTuner() end})
    end
    self.bagSlots[1]:SetScript("OnHide",function() V.bagSlots[1].selected:Hide();CloseDropDownMenus() end)
    self.bagTunerButton=settingsButton(p,"Bag Tuner",0,90,nil,function() V:OpenBagTuner() end,.75,26)
    self.bagTunerButton:ClearAllPoints()
    self.bagTunerButton:SetPoint("TOPLEFT",p,"TOPLEFT",336-self.bagTunerButton:GetWidth(),-90)
    self:CreateBagTunerUI(sheet,section,label,edit,settingsButton,enabled)
end
function V:OpenBackBagMenu()
    self:WatchSlotMenuDismissal()
    GameTooltip:Hide()
    self:CloseBrowser()
    ToggleDropDownMenu(1,nil,self.backBagMenu,self.bagSlots[1]:GetName(),0,0)
    self:RefreshSlotHighlights()
end
function V:CreateExposurePage(p)
    p:SetFrameLevel(self.frame:GetFrameLevel()+12)
    label(p,"Exposure",33,87,124,18)
    self.exposureNote=label(p,"Exposure is not implemented yet.\n\nThis planned feature will add visual effects such as blood, dust, water, and filth to your character's model and weapons.",33,113,128,0,true)
    self.exposureNote:SetJustifyV("TOP")
end
function V:RefreshSlotHighlights()
    if not self.slotButtons then return end
    if self.bagSlots then
        local active=self.tab=="bags" and UIDROPDOWNMENU_OPEN_MENU==self.backBagMenu:GetName() and DropDownList1:IsShown()
        if active then self.bagSlots[1].selected:Show() else self.bagSlots[1].selected:Hide() end
    end
    local slot
    if self.frame and self.frame:IsVisible() then
        if self.slotMenu and UIDROPDOWNMENU_OPEN_MENU==self.slotMenu:GetName() and DropDownList1:IsShown() then
            slot=self.menuSlot
        elseif self.browser and self.browser:IsShown() then slot=self.slot end
    end
    for id,b in pairs(self.slotButtons) do
        if id==slot then b.selected:Show() else b.selected:Hide() end
    end
end
function V:WatchSlotMenuDismissal()
    if self.watchingSlotMenuDismissal then return end
    self.watchingSlotMenuDismissal=true
    local onHide=DropDownList1:GetScript("OnHide")
    DropDownList1:SetScript("OnHide",function()
        if onHide then onHide() end
        V:RefreshSlotHighlights()
    end)
end
function V:OpenSlotMenu(slot)
    if not self.slotButtons[slot] or (self:IsWeaponPosition(slot) and self.tab~="weaponry") then return end
    GameTooltip:Hide()
    if not self.slotMenu then
        self.slotMenu=CreateFrame("Frame","SaureksClosetSlotMenu",self.frame)
        self.slotMenu.displayMode="MENU";self.slotMenu:Hide()
        -- The visible list is shared by all native dropdowns; the owner frame
        -- itself never shows. Keep its original submenu cleanup when dismissed.
        self:WatchSlotMenuDismissal()
        self.slotMenu.initialize=function()
            local selectedSlot=V.menuSlot
            local selected=V:SlotSelection(selectedSlot)
            if V:IsWeaponPosition(selectedSlot) then
                UIDropDownMenu_AddButton({text="Custom Item",checked=selected and 1 or nil,func=function() V:OpenBrowser(selectedSlot) end})
                UIDropDownMenu_AddButton({text="Game Default",checked=not selected and 1 or nil,func=function() V:CloseBrowser();V:ClearSlot(selectedSlot) end})
                UIDropDownMenu_AddButton({text="Placement Tuner",notCheckable=1,disabled=not selected and 1 or nil,func=function()
                    V:CloseBrowser();V:OpenPlacementTuner(selectedSlot)
                end})
                return
            end
            UIDropDownMenu_AddButton({text="Custom Item",checked=selected~=nil and selected~=0 and 1 or nil,func=function() V:OpenBrowser(selectedSlot) end})
            UIDropDownMenu_AddButton({text="Hide Slot",checked=selected==0 and 1 or nil,func=function()
                V:CloseBrowser();V:Select(selectedSlot,0)
            end})
            UIDropDownMenu_AddButton({text="Passthrough",checked=selected==nil and 1 or nil,func=function()
                V:CloseBrowser();V:ClearSlot(selectedSlot)
            end})
        end
    end
    if self.menuSlot~=slot then self:CloseOutfitMenu() end
    self.menuSlot=slot
    ToggleDropDownMenu(1,nil,self.slotMenu,self.slotButtons[slot]:GetName(),0,0)
    self:RefreshSlotHighlights()
end
function V:OpenBrowser(slot)
    if not self.slotButtons[slot] or (self:IsWeaponPosition(slot) and self.tab~="weaponry") then return end
    self:CancelDraft();self.slot=slot;self.offset=0
    self.quality=nil;self.material=nil;self.favoritesOnly=false
    self.query="";self.search:SetText("")
    -- Use the stock two-column panel manager to prevent overlap with other menus.
    self:SetPanelArea("doublewide")
    -- Each opening starts beside the wardrobe. Dragging is session-only.
    self.browser:ClearAllPoints();self.browser:SetPoint("TOPLEFT",self.frame,"TOPLEFT",384,0)
    self.browser:Show();self:CloseWeaponOptions();self:Refresh()
end
function V:CloseBrowser()
    CloseDropDownMenus()
    self:CancelDraft()
    if self.browser then self.browser:Hide();self.search:ClearFocus() end
    self:RefreshSlotHighlights()
    self:ReleaseNeighbor()
end
function V:CreateBrowser()
    local f=sheet("SaureksClosetBrowser",self.frame,"Armor Appearance")
    self.browser=f;f:Hide();f:SetPoint("TOPLEFT",self.frame,"TOPLEFT",384,0)
    f:SetMovable(true);f:SetClampedToScreen(true);f:RegisterForDrag("LeftButton")
    f:SetScript("OnDragStart",function() this:StartMoving() end)
    f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
    f:SetScript("OnShow",function() V:RefreshPortraits() end)
    f.close:SetScript("OnClick",function() V:CloseBrowser();V:Refresh() end)
    self.commitButton=button(f,"Apply Appearance",206,44,132,function() V:CommitDraft();V:Refresh() end)
    local filters=section(f,23,79,318,100,true)
    self.search=edit(filters,"SaureksClosetSearch",15,10,293)
    -- InputBoxTemplate extends 5px left: these bounds give 10px outer margins.
    -- The text starts 5px inside that border; reserve the same inset on the right.
    self.search:SetTextInsets(0,5,0,0)
    self.search:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.searchHint=label(self.search,"Search Item DB...",0,0,288,22,true)
    self.searchHint:SetFont("Fonts\\FRIZQT__.TTF",12);self.searchHint:SetJustifyV("MIDDLE")
    self.searchHint:SetTextColor(1,1,1,.45)
    local function updateHint()
        if V.search:GetText()=="" and not V.searchFocused then V.searchHint:Show() else V.searchHint:Hide() end
    end
    self.search:SetScript("OnTextChanged",function() V.query=this:GetText();V.offset=0;updateHint();V:RefreshList() end)
    self.search:SetScript("OnEditFocusGained",function() V.searchFocused=true;updateHint() end)
    self.search:SetScript("OnEditFocusLost",function() V.searchFocused=nil;updateHint() end)
    local function filterSelector(kind,name,x,width)
        local box=section(filters,x,39,width,27,true)
        local text=label(box,"",8,5,width-34,17,true)
        text:SetFont("Fonts\\FRIZQT__.TTF",11);text:SetJustifyV("MIDDLE")
        local control=CreateFrame("Button",name,box);control:SetAllPoints(box)
        control:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        texture(control,"Interface\\Buttons\\UI-ScrollBar-ScrollDownButton-Up",width-26,3,22,22,"ARTWORK")
        local menu=CreateFrame("Frame",name.."Menu",f);menu.displayMode="MENU";menu:Hide()
        menu.initialize=function() V:BuildItemFilterMenu(kind) end
        control:SetScript("OnClick",function() ToggleDropDownMenu(1,nil,menu,name,0,0) end)
        return control,text,menu
    end
    self.qualityButton,self.qualityLabel,self.qualityMenu=filterSelector("quality","SaureksClosetQualityFilter",7,148)
    self.materialButton,self.materialLabel,self.materialMenu=filterSelector("material","SaureksClosetTypeFilter",159,152)
    local listHeight=245 -- Leave 3px between the raised tile and the pane bottom.
    local list=section(f,23,183,318,listHeight)
    local rail="Interface\\PaperDollInfoFrame\\UI-Character-ScrollBar"
    local backing=texture(list,"Interface\\ClassTrainerFrame\\UI-ClassTrainer-ScrollBar",314,4,4,listHeight-8,"ARTWORK")
    backing:SetTexCoord(26/64,30/64,28/128,100/128)
    local railTop=texture(list,rail,288,4,28,128,"ARTWORK");railTop:SetTexCoord(0,31/64,0,.5)
    -- The native bottom cap ends at texture row 108; rows below it are transparent.
    -- Stretch only the straight track, then anchor the intact cap around the down arrow.
    local railMiddle=texture(list,rail,288,132,28,listHeight-160,"ARTWORK");railMiddle:SetTexCoord(33/64,1,8/256,84/256)
    local railBottom=texture(list,rail,288,listHeight-28,28,24,"ARTWORK");railBottom:SetTexCoord(33/64,1,84/256,108/256)
    self.scrollRailBottom=railBottom
    local s=CreateFrame("Slider","SaureksClosetItemScroll",list,"UIPanelScrollBarTemplate")
    self.scroll=s;s:SetPoint("TOPLEFT",list,"TOPLEFT",294,-24);s:SetWidth(16);s:SetHeight(listHeight-48);s:SetValueStep(1)
    self.scrollThumb=getglobal(s:GetName().."ThumbTexture")
    self.scrollUp=getglobal(s:GetName().."ScrollUpButton")
    self.scrollDown=getglobal(s:GetName().."ScrollDownButton")
    self.scrollUp:SetScript("OnClick",function() V.scroll:SetValue(math.max(0,V.offset-1)) end)
    self.scrollDown:SetScript("OnClick",function() V.scroll:SetValue(math.min(V.maxOffset or 0,V.offset+1)) end)
    s:SetScript("OnValueChanged",function()
        if V.updatingScroll then return end
        V.offset=math.floor(this:GetValue()+.5);V:RefreshList()
    end)
    f:EnableMouseWheel(true)
    f:SetScript("OnMouseWheel",function() V.offset=math.max(0,math.min(V.maxOffset or 0,V.offset-arg1*3));V:RefreshList() end)
    self.rows={};self.visibleItemRows=8
    local rowHeight=(listHeight-8)/self.visibleItemRows
    for i=1,self.visibleItemRows do
        local b=CreateFrame("Button",nil,list)
        b:SetPoint("TOPLEFT",list,"TOPLEFT",7,-(4+(i-1)*rowHeight));b:SetWidth(276);b:SetHeight(rowHeight)
        b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        b.selected=texture(b,"Interface\\QuestFrame\\UI-QuestTitleHighlight",0,0,276,rowHeight)
        b.selected:SetBlendMode("ADD")
        b.icon=texture(b,"Interface\\Icons\\INV_Misc_QuestionMark",0,3,24,24,"ARTWORK")
        b.nameLabel=label(b,"",29,1,245,13,true)
        b.metaLabel=label(b,"",29,16,245,12,true)
        b:SetScript("OnClick",function() if this.item then V:DraftSlot(V.slot,this.item[1]) end end)
        b:SetScript("OnEnter",function()
            if not this.item then return end
            GameTooltip:SetOwner(this,"ANCHOR_RIGHT")
            if GetItemInfo(this.item[1]) then GameTooltip:SetHyperlink(V:Link(this.item[1]))
            else GameTooltip:SetText(this.item[2],1,1,1);GameTooltip:AddLine("Item "..this.item[1],1,1,1) end
            if (this.item[11] or 0)==0 and this.item[12] then
                GameTooltip:AddLine("Quest minimum: Level "..this.item[12],1,.82,0)
            end
            GameTooltip:Show()
        end)
        b:SetScript("OnLeave",function() GameTooltip:Hide() end)
        self.rows[i]=b
    end
    self.emptyLabel=label(list,"No matches. Clear the search or change the filters.",12,(listHeight-50)/2,272,50,true)
    self.resultsLabel=label(filters,"",217,71,87,22,true)
    self.resultsLabel:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.resultsLabel:SetTextColor(.85,.85,.85)
    self.resultsLabel:SetJustifyH("RIGHT");self.resultsLabel:SetJustifyV("MIDDLE")
    local checkbox=CreateFrame("CheckButton","SaureksClosetHideHigherLevelItems",filters,"UICheckButtonTemplate")
    self.hideHigherLevelCheckbox=checkbox
    checkbox:SetPoint("TOPLEFT",filters,"TOPLEFT",7,-70);checkbox:SetWidth(24);checkbox:SetHeight(24)
    checkbox:SetHitRectInsets(0,-174,0,0)
    self.hideHigherLevelLabel=label(filters,"Hide Higher Level Items",34,71,171,22,true)
    self.hideHigherLevelLabel:SetJustifyV("MIDDLE")
    checkbox:SetScript("OnClick",function() V:SetHideHigherLevelItems(this:GetChecked()) end)
    checkbox:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_TOP");GameTooltip:SetText("Hide Higher Level Items",1,1,1)
        GameTooltip:AddLine("Hide items above your character's level.",1,1,1)
        GameTooltip:AddLine("Quest reward items without explicit level req. use the quest's minimum level.",1,1,1,true)
        GameTooltip:Show()
    end)
    checkbox:SetScript("OnLeave",function() GameTooltip:Hide() end)
end
function V:BuildItemFilterMenu(kind)
    local function option(text,value)
        UIDropDownMenu_AddButton({text=text,checked=self[kind]==value and 1 or nil,func=function()
            V[kind]=value;V.offset=0;V:RefreshList()
        end})
    end
    UIDropDownMenu_AddButton({text=kind=="quality" and "Quality" or "Type",isTitle=1,notCheckable=1})
    option(kind=="quality" and "All Rarities" or "All",nil)
    if kind=="quality" then
        for value=0,6 do option(self.qualityNames[value],value) end
        option("Unobtainable","unobtainable")
    else
        local maxLevel=VanityStudioDB.hideHigherLevelItems and UnitLevel("player") or nil
        for _,value in ipairs(self:ItemTypeOptions(self.slot,self.quality,maxLevel)) do option(self:ItemTypeName(value,self.slot),value) end
    end
end
function V:CreateBodyPage(p)
    p:SetFrameLevel(self.frame:GetFrameLevel()+12)
    local rail=sidebar(p,270);self.bodySidebar=rail
    self.bodyRows={}
    self.bodyMenu=CreateFrame("Frame","SaureksClosetBodyMenu",self.frame)
    self.bodyMenu.displayMode="MENU";self.bodyMenu:Hide()
    self.bodyMenu.initialize=function() V:BuildBodyMenu() end
    local function group(title,y,height)
        return sidebarGroup(rail,title,y,height)
    end
    local function choice(box,key,index,prefix)
        local k=key;local y=24+(index-1)*27
        local control=CreateFrame("Button","SaureksClosetBody"..key,box)
        control:SetPoint("TOPLEFT",box,"TOPLEFT",34,-(y+3));control:SetWidth(69);control:SetHeight(20)
        local value=label(control,"",0,0,69,20,true)
        value:SetFont("Fonts\\FRIZQT__.TTF",10)
        value:SetJustifyH("CENTER");value:SetJustifyV("MIDDLE")
        local highlight=texture(control,"Interface\\QuestFrame\\UI-QuestTitleHighlight",0,0,69,20,"OVERLAY")
        highlight:SetBlendMode("ADD");highlight:SetAlpha(.3);highlight:Hide()
        control:SetScript("OnEnter",function() highlight:Show() end)
        control:SetScript("OnLeave",function() highlight:Hide() end)
        control:SetScript("OnHide",function() highlight:Hide() end)
        control:SetScript("OnClick",function()
            if V.bodyMenuKey~=k then V:CloseOutfitMenu() end
            V.bodyMenuKey=k
            ToggleDropDownMenu(1,nil,V.bodyMenu,this:GetName(),0,0)
        end)
        local previous=arrow(box,"Prev",7,y+1,function() CloseDropDownMenus();V:CycleBody(k,-1) end)
        local nextChoice=arrow(box,"Next",106,y+1,function() CloseDropDownMenus();V:CycleBody(k,1) end)
        previous:SetWidth(24);previous:SetHeight(24)
        nextChoice:SetWidth(24);nextChoice:SetHeight(24)
        self.bodyRows[key]={value=value,button=control,previous=previous,next=nextChoice,prefix=prefix}
    end
    local character=group("Body",8,82)
    choice(character,"race",1);choice(character,"sex",2)
    local appearance=group("Appearance",98,170)
    choice(appearance,"skin",1,"Skin");choice(appearance,"face",2,"Face")
    choice(appearance,"hairStyle",3,"Hair");choice(appearance,"hairColor",4,"Color")
    choice(appearance,"facial",5,"Features")
    local captionWidth=72
    local resetWidth=104
    local reset=section(p,336-resetWidth,90,resetWidth,26,true,"Button",.75)
    self.realBodyButton=reset
    texture(reset,"Interface\\Buttons\\UI-CheckBox-Up",5,3,20,20,"ARTWORK")
    self.trueModelCheck=texture(reset,"Interface\\Buttons\\UI-CheckBox-Check",5,3,20,20,"OVERLAY")
    self.trueModelLabel=label(reset,"Disable",28,4,captionWidth,18,true)
    self.trueModelLabel:SetFont("Fonts\\FRIZQT__.TTF",11)
    self.trueModelLabel:SetJustifyV("MIDDLE")
    local hover=texture(reset,"Interface\\QuestFrame\\UI-QuestTitleHighlight",4,4,resetWidth-8,18,"ARTWORK")
    hover:SetBlendMode("ADD");hover:SetAlpha(.3);hover:Hide()
    reset:SetScript("OnEnter",function()
        hover:Show()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:ClearLines()
        GameTooltip:AddLine("Disable all race & body changes",1,1,1,true)
        GameTooltip:Show()
    end)
    reset:SetScript("OnLeave",function() hover:Hide();GameTooltip:Hide() end)
    reset:SetScript("OnHide",function() hover:Hide();GameTooltip:Hide() end)
    reset:SetScript("OnClick",function() CloseDropDownMenus();V:ClearBody() end)
end
function V:BodyChoiceLabel(key,value,index)
    if key=="race" then return VanityStudioRaces[value][1]
    elseif key=="sex" then return value==0 and "Male" or "Female"
    else return tostring(index) end
end
function V:BuildBodyMenu()
    local key=self.bodyMenuKey;if not key then return end
    local body=self:BodyDraft();local choices
    if not body then return end
    if key=="race" then choices={1,2,3,4,5,6,7,8}
    elseif key=="sex" then choices={0,1}
    else choices=self:BodyValues(body,key) end
    for i,value in ipairs(choices) do
        local selectedValue=value
        UIDropDownMenu_AddButton({text=self:BodyChoiceLabel(key,value,i),checked=body[key]==value and 1 or nil,
            func=function() V:SelectBodyValue(key,selectedValue) end})
    end
end
function V:RefreshBody()
    if not self.uiReady then return end
    local body=self:BodyDraft();local available=self:BodyAvailable() and body~=nil
    if self:UsingTrueModel() then self.trueModelCheck:Show() else self.trueModelCheck:Hide() end
    enabled(self.realBodyButton,self:UsingTrueModel() or self:BodyAvailable())
    for key,row in pairs(self.bodyRows) do
        local index=1
        if body and key~="race" and key~="sex" then
            for i,value in ipairs(self:BodyValues(body,key)) do if value==body[key] then index=i end end
        end
        local text=body and self:BodyChoiceLabel(key,body[key],index) or "Loading..."
        row.value:SetText(row.prefix and (row.prefix.." "..text) or text)
        local count=not body and 0 or key=="race" and 8 or key=="sex" and 2 or table.getn(self:BodyValues(body,key))
        enabled(row.button,available)
        enabled(row.previous,available and count>1);enabled(row.next,available and count>1)
    end
end
function V:CreateOutfitPage(p)
    local backdrop=texture(p,"Interface\\PaperDollInfoFrame\\UI-Character-General-TopLeft",20,75,320,352)
    backdrop:SetTexCoord(64/256,244/256,80/256,250/256)
    local list=CreateFrame("Frame",nil,p)
    list:SetPoint("TOPLEFT",p,"TOPLEFT",20,-75);list:SetWidth(320);list:SetHeight(352)
    self.outfitList=list
    local railX=297
    local contentWidth=291

    -- Unsaved work is an action panel, not another saved-outfit row.
    local draft=section(list,3,3,contentWidth,120,false,"Frame")
    self.unsavedLookPanel=draft
    draft:SetBackdropBorderColor(.78,.61,.2)
    self.unsavedLookTitle=label(draft,"Your current configuration is unsaved.",11,8,268,17)
    self.unsavedLookTitle:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.unsavedLookHelp=label(draft,"Type a name for your look and hit Save New below, or click the Update Existing button to update an existing look.",11,27,268,32,true)
    self.unsavedLookHelp:SetFont("Fonts\\FRIZQT__.TTF",9)
    self.unsavedLookHelp:SetTextColor(.78,.78,.78)
    self.unsavedOutfitName=edit(draft,"SaureksClosetUnsavedOutfitName",18,62,142,40)
    self.unsavedOutfitName:SetFont("Fonts\\FRIZQT__.TTF",11)
    self.unsavedOutfitNameHint=label(self.unsavedOutfitName,"Type name here",0,0,142,22,true)
    self.unsavedOutfitNameHint:SetFont("Fonts\\FRIZQT__.TTF",11)
    self.unsavedOutfitNameHint:SetJustifyV("MIDDLE");self.unsavedOutfitNameHint:SetTextColor(1,1,1,.42)
    self.unsavedOutfitName:SetScript("OnTextChanged",function() V.unsavedLookMessage=nil;V:RefreshUnsavedLookPanel() end)
    self.unsavedOutfitName:SetScript("OnEditFocusGained",function() V.unsavedNameFocused=true;V:RefreshUnsavedLookPanel() end)
    self.unsavedOutfitName:SetScript("OnEditFocusLost",function() V.unsavedNameFocused=nil;V:RefreshUnsavedLookPanel() end)
    self.saveUnsavedAsNewButton=button(draft,"Save New",168,62,112,function()
        if not V.saveUnsavedAsNewButton.closetEnabled then return end
        local ok,name=V:SaveOutfit(V.unsavedOutfitName:GetText(),false,V.UNSAVED)
        if ok then
            V.unsavedOutfitName:SetText("");V.unsavedOutfitName:ClearFocus()
            V.unsavedLookMessage="Saved as "..name..".";V:RefreshOutfits()
        else V.unsavedLookMessage=name;V:RefreshUnsavedLookPanel() end
    end)
    self.updateSavedLookButton=button(draft,"Update Existing",168,90,112,function()
        if V.updateSavedLookButton.closetEnabled then V:ToggleReplaceOutfitMenu(V.updateSavedLookButton) end
    end)

    self.savedLooksHeading=label(list,"SAVED LOOKS",10,129,180,16,true)
    self.savedLooksHeading:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.savedLooksHeading:SetTextColor(.72,.72,.72)
    self.savedLooksCount=label(list,"(0)",238,129,48,16,true)
    self.savedLooksCount:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.savedLooksCount:SetTextColor(.72,.72,.72)
    self.savedLooksCount:SetJustifyH("RIGHT")
    self.savedLooksDivider=texture(list,"Interface\\Buttons\\WHITE8X8",10,146,276,1,"ARTWORK")
    self.savedLooksDivider:SetVertexColor(.55,.49,.38,.55)

    self.outfitRows={};self.visibleOutfitRows=3;self.outfitOffset=0
    local rowHeight=58
    local tileHeight=54
    local padding=3
    -- The stock rail has two transparent columns before its visible left edge.
    local railVisibleInset=2
    local tileWidth=railX+railVisibleInset-2*padding
    for i=1,5 do
        local b=section(list,padding,152+(i-1)*rowHeight,tileWidth,tileHeight,false,"Button")
        b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        b.selected=texture(b,"Interface\\QuestFrame\\UI-QuestTitleHighlight",3,3,tileWidth-6,tileHeight-6);b.selected:SetBlendMode("ADD")
        b.active=texture(b,"Interface\\Buttons\\WHITE8X8",3,3,tileWidth-6,tileHeight-6);b.active:SetVertexColor(1,.72,.12,.065);b.active:Hide()
        b.status=label(b,"ACTIVE",tileWidth-72,18,56,18,true)
        b.status:SetFont("Fonts\\FRIZQT__.TTF",10);b.status:SetJustifyH("RIGHT")
        b.status:SetJustifyV("MIDDLE")
        b.status:SetTextColor(1,.82,0);b.status:Hide()
        b.portraitImage=texture(b,"Interface\\CharacterFrame\\TemporaryPortrait",10,(tileHeight-38)/2,38,38,"ARTWORK")
        -- Symmetric one-pixel rim sits outside the full portrait image.
        b.portraitBorder=texture(b,art.."PortraitBorder.tga",6,(tileHeight-46)/2,46,46,"ARTWORK")
        local textTop=(tileHeight-34)/2
        b.title=label(b,"",59,textTop,tileWidth-139,17)
        b.detail=label(b,"",59,textTop+18,tileWidth-139,16,true)
        b.title:SetJustifyV("MIDDLE");b.detail:SetJustifyV("MIDDLE")
        b:SetScript("OnClick",function() if this.outfit then V:OpenOutfitDetails(this.outfit) end end)
        self.outfitRows[i]=b
    end
    -- SkillFrame.xml uses this trainer texture's opaque right-hand stone strip.
    -- Keep the established rail/end-cap alignment and fill its seam into the window.
    local railBacking=texture(list,"Interface\\ClassTrainerFrame\\UI-ClassTrainer-ScrollBar",railX+26,-1,4,356,"ARTWORK")
    railBacking:SetTexCoord(26/64,30/64,28/128,100/128)
    -- Native ornamented scrollbar surround; crop the lower strip to retain its end cap.
    local rail="Interface\\PaperDollInfoFrame\\UI-Character-ScrollBar"
    local railTop=texture(list,rail,railX,-1,28,256,"ARTWORK")
    railTop:SetTexCoord(0,31/64,0,1)
    local railBottom=texture(list,rail,railX,255,28,100,"ARTWORK")
    railBottom:SetTexCoord(33/64,1,7/256,107/256)
    local scroll=CreateFrame("Slider","SaureksClosetOutfitScroll",list,"UIPanelScrollBarTemplate")
    self.outfitScroll=scroll;scroll:SetPoint("TOPLEFT",list,"TOPLEFT",railX+6,-19);scroll:SetWidth(16);scroll:SetHeight(317);scroll:SetValueStep(1)
    self.outfitScrollUp=getglobal(scroll:GetName().."ScrollUpButton");self.outfitScrollDown=getglobal(scroll:GetName().."ScrollDownButton")
    self.outfitScrollThumb=getglobal(scroll:GetName().."ThumbTexture")
    -- The native 16px arrows center at y=11 and y=344 inside the decorative end caps.
    self.outfitScrollUp:SetScript("OnClick",function() V:SetOutfitOffset(V.outfitOffset-1) end)
    self.outfitScrollDown:SetScript("OnClick",function() V:SetOutfitOffset(V.outfitOffset+1) end)
    scroll:SetScript("OnValueChanged",function() if not V.updatingOutfitScroll then V:SetOutfitOffset(this:GetValue()) end end)
    local empty=CreateFrame("Frame",nil,list);self.outfitEmpty=empty
    empty:SetPoint("TOPLEFT",list,"TOPLEFT",18,-116);empty:SetWidth(railX-42);empty:SetHeight(88)
    self.outfitEmptyTitle=label(empty,"No saved looks yet.",12,12,railX-66,20,true)
    self.outfitEmptyTitle:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.outfitEmptyTitle:SetJustifyH("CENTER");self.outfitEmptyTitle:SetJustifyV("MIDDLE")
    self.outfitEmptyHelp=label(empty,"Customize your current look, then save it above.",18,44,railX-78,30,true)
    self.outfitEmptyHelp:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.outfitEmptyHelp:SetTextColor(.72,.72,.72)
    self.outfitEmptyHelp:SetJustifyH("CENTER");self.outfitEmptyHelp:SetJustifyV("TOP")
    p:EnableMouseWheel(true)
    p:SetScript("OnMouseWheel",function()
        V:SetOutfitOffset(V.outfitOffset-arg1*3)
    end)
end
function V:SetOutfitOffset(value)
    self.outfitOffset=math.max(0,math.min(math.max(0,table.getn(self:OutfitNames())-self.visibleOutfitRows),math.floor(value+.5)))
    self:RefreshOutfits()
end
function V:CreateOutfitDetails()
    local f=sheet("SaureksClosetOutfitDetails",self.frame,"Outfit Preview")
    self.outfitDetails=f;f:Hide();f:SetPoint("TOPLEFT",self.frame,"TOPLEFT",384,0)
    f.close:SetScript("OnClick",function() V:CloseOutfitDetails();V:RefreshOutfits() end)
    local backdrop=texture(f,"Interface\\PaperDollInfoFrame\\UI-Character-General-TopLeft",20,75,320,352)
    backdrop:SetTexCoord(64/256,244/256,80/256,250/256)
    self.outfitModel=CreateFrame("DressUpModel","SaureksClosetOutfitModel",f)
    self.outfitBuffer=CreateFrame("DressUpModel","SaureksClosetOutfitModelBuffer",f)
    for _,m in ipairs({self.outfitModel,self.outfitBuffer}) do
        m:SetPoint("TOPLEFT",f,"TOPLEFT",61,-78);m:SetWidth(244);m:SetHeight(246);m:SetAlpha(0);m.rotation=.61
    end
    local controls=CreateFrame("Frame",nil,f);controls:SetPoint("TOPLEFT",f,"TOPLEFT",20,-76);controls:SetWidth(70);controls:SetHeight(35)
    for i,direction in ipairs({"Left","Right"}) do
        local b=CreateFrame("Button","SaureksClosetOutfitModelRotate"..direction.."Button",controls)
        b:SetPoint("TOPLEFT",controls,"TOPLEFT",(i-1)*35,0);b:SetWidth(35);b:SetHeight(35)
        b:SetNormalTexture("Interface\\Buttons\\UI-Rotation"..direction.."-Button-Up")
        b:SetPushedTexture("Interface\\Buttons\\UI-Rotation"..direction.."-Button-Down")
        b:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Round","ADD");b:RegisterForClicks("LeftButtonDown","LeftButtonUp")
        setglobal("SaureksClosetOutfitModelBufferRotate"..direction.."Button",b)
        b.direction=direction;b:SetScript("OnClick",function()
            if this.direction=="Left" then Model_RotateLeft(V.outfitModel) else Model_RotateRight(V.outfitModel) end
        end)
    end
    controls:SetScript("OnUpdate",function() Model_OnUpdate(arg1,V.outfitModel) end)
    self.detailPreviewNote=label(f,"",48,180,268,70,true);self.detailPreviewNote:SetJustifyH("CENTER")
    local actions=CreateFrame("Frame",nil,f);self.outfitActions=actions
    actions:SetPoint("TOPLEFT",f,"TOPLEFT",20,-353);actions:SetWidth(320);actions:SetHeight(72)
    self.outfitName=edit(actions,"SaureksClosetOutfitName",6,0,202,40)
    self.outfitName:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.outfitNameHint=label(self.outfitName,"Name your outfit...",0,0,202,22,true)
    self.outfitNameHint:SetFont("Fonts\\FRIZQT__.TTF",12);self.outfitNameHint:SetJustifyV("MIDDLE")
    self.outfitNameHint:SetTextColor(1,1,1,.45)
    self.outfitName:SetScript("OnTextChanged",function() V:RefreshOutfitNameHint() end)
    self.outfitName:SetScript("OnEditFocusGained",function() V.outfitNameFocused=true;V:RefreshOutfitNameHint() end)
    self.outfitName:SetScript("OnEditFocusLost",function() V.outfitNameFocused=nil;V:RefreshOutfitNameHint() end)
    self.saveOutfitButton=button(actions,"Save new",216,0,104,function()
        if not V.saveOutfitButton.closetEnabled then return end
        local key=V.detailKey;local ok,name
        if key==V.UNSAVED then ok,name=V:SaveOutfit(V.outfitName:GetText(),false,key)
        else ok,name=V:RenameOutfit(key,V.outfitName:GetText()) end
        if ok then
            V.detailKey=name;V.selectedOutfit=name;V.outfitMessage:SetText("")
            V.outfitName:ClearFocus();V:RefreshOutfits();V:RefreshOutfitDetails()
        else V.outfitMessage:SetText(name) end
    end)
    self.activateOutfitButton=button(actions,"Activate Outfit",0,25,212,function() V:ActivateSavedOutfit(V.detailKey) end)
    self.deleteOutfitButton=button(actions,"Delete",216,25,104,function()
        local key=V.detailKey
        if not key or not V:GetOutfit(key) then return end
        if V.confirmDelete==key then
            V:DeleteOutfit(key);V:CloseOutfitDetails();V:Refresh()
        else
            V.confirmDelete=key
            V.outfitMessage:SetText(key==V.UNSAVED and "Discard unsaved changes? Click Confirm." or "Delete this saved outfit? Click Confirm.")
            V.deleteOutfitButton:SetText("Confirm")
        end
    end)
    self.replaceOutfitButton=button(actions,"Save to existing outfit",0,50,320,function() V:ToggleReplaceOutfitMenu() end)
    self.replaceOutfitMenu=CreateFrame("Frame","SaureksClosetReplaceOutfitMenu",f)
    self.replaceOutfitMenu.displayMode="MENU";self.replaceOutfitMenu:Hide()
    self.replaceOutfitMenu.initialize=function() V:BuildReplaceOutfitMenu() end
    self.outfitMessage=label(actions,"",0,-18,320,16,true)
end
function V:SetPanelArea(area)
    if not self.frame or UIPanelWindows[self.frame:GetName()].area==area then return end
    UIPanelWindows[self.frame:GetName()].area=area
    if not self.frame:IsVisible() then return end
    -- Transfer our panel registration without hiding the frame. Native model
    -- OnHide releases its M2; resizing for a neighbor must not destroy previews.
    if UIParent.left==self.frame then UIParent.left=nil end
    if UIParent.center==self.frame then UIParent.center=nil end
    if UIParent.doublewide==self.frame then UIParent.doublewide=nil end
    if area=="doublewide" then SetDoublewideFrame(self.frame) else SetLeftFrame(self.frame) end
end
function V:ReleaseNeighbor()
    if not self.frame or (self.browser and self.browser:IsShown()) or (self.outfitDetails and self.outfitDetails:IsShown()) or (self.weaponOptions and self.weaponOptions:IsShown()) then return end
    self:SetPanelArea("left")
end
function V:CloseOutfitDetails()
    CloseDropDownMenus()
    if self.outfitDetails then self.outfitDetails:Hide() end
    self.detailPending=nil;self.detailPreviewSignature=nil;self.detailPreviewBodyKey=nil;self.detailMissing=nil;self.detailKey=nil;self.selectedOutfit=nil;self.confirmDelete=nil
    self:ReleaseNeighbor()
end
function V:OpenOutfitDetails(key)
    if not self:GetOutfit(key) then return end
    if self.outfitDetails:IsShown() and self.detailKey==key then return end
    self:CloseBrowser();self:CloseOutfitDetails()
    self:SetPanelArea("doublewide")
    self.detailKey=key;self.selectedOutfit=key;self.confirmDelete=nil
    self.outfitName:SetText(key==self.UNSAVED and "" or key);self.outfitMessage:SetText("")
    self.outfitDetails:Show();self:RefreshPortraits();self:RefreshOutfits();self:RefreshOutfitDetails();self:StartOutfitPreview()
end
function V:RefreshOutfitNameHint()
    if not self.outfitNameHint then return end
    if self.detailKey==self.UNSAVED and self.outfitName:GetText()=="" and not self.outfitNameFocused then
        self.outfitNameHint:Show()
    else self.outfitNameHint:Hide() end
    if self.saveOutfitButton then
        enabled(self.saveOutfitButton,string.find(self.outfitName:GetText() or "","%S")~=nil)
    end
end
function V:RefreshOutfitDetails()
    if not self.uiReady or not self.outfitDetails:IsShown() then return end
    if not self:GetOutfit(self.detailKey) then return end
    local unsaved=self.detailKey==self.UNSAVED
    local top=unsaved and 353 or 378
    self.outfitActions:ClearAllPoints();self.outfitActions:SetPoint("TOPLEFT",self.outfitDetails,"TOPLEFT",20,-top)
    self.outfitActions:SetHeight(425-top)
    for _,m in ipairs({self.outfitModel,self.outfitBuffer}) do m:SetHeight(top-79) end
    self:RefreshOutfitNameHint()
    self.saveOutfitButton:SetText(unsaved and "Save new" or "Rename")
    self.activateOutfitButton:SetText("Activate Outfit")
    enabled(self.activateOutfitButton,not self:IsOutfitActive(self.detailKey) or not VanityStudioCharacter.enabled)
    self.deleteOutfitButton:SetText(self.confirmDelete==self.detailKey and "Confirm" or "Delete")
    if self.detailKey==self.UNSAVED then self.replaceOutfitButton:Show() else self.replaceOutfitButton:Hide() end
end
function V:ToggleReplaceOutfitMenu(owner)
    self.replaceOutfitPage=1
    self.replaceOutfitOwner=owner or self.replaceOutfitButton
    ToggleDropDownMenu(1,nil,self.replaceOutfitMenu,self.replaceOutfitOwner:GetName(),0,0)
end
function V:BuildReplaceOutfitMenu()
    if not self:GetOutfit(self.UNSAVED) then return end
    local names=self:OutfitNames();local count=table.getn(names)
    local pages=math.max(1,math.ceil(count/10));local page=math.min(self.replaceOutfitPage or 1,pages)
    UIDropDownMenu_AddButton({text="Update a saved look",isTitle=1,notCheckable=1})
    if count==0 then UIDropDownMenu_AddButton({text="No saved outfits",disabled=1,notCheckable=1});return end
    if pages>1 then
        local turn=function(value)
            V.replaceOutfitPage=value;CloseDropDownMenus()
            ToggleDropDownMenu(1,nil,V.replaceOutfitMenu,V.replaceOutfitOwner:GetName(),0,0)
        end
        UIDropDownMenu_AddButton({text="Previous page",notCheckable=1,disabled=page==1 and 1 or nil,keepShownOnClick=1,func=turn,arg1=page-1})
        UIDropDownMenu_AddButton({text="Next page",notCheckable=1,disabled=page==pages and 1 or nil,keepShownOnClick=1,func=turn,arg1=page+1})
    end
    for i=(page-1)*10+1,math.min(page*10,count) do
        UIDropDownMenu_AddButton({text=names[i],notCheckable=1,arg1=names[i],func=function(name) V:ReplaceWithUnsaved(name) end})
    end
end
function V:ReplaceWithUnsaved(name)
    if not self:GetOutfit(self.UNSAVED) or not VanityStudioDB.outfits[name] then return end
    local updatingActive=self:IsOutfitActive(name)
    local ok,err=self:SaveOutfit(name,true,self.UNSAVED)
    if not ok then
        if self.outfitDetails:IsShown() then self.outfitMessage:SetText(err)
        else self.unsavedLookMessage=err;self:RefreshUnsavedLookPanel() end
        return
    end
    if updatingActive then self:LoadOutfit(name) end
    if self.outfitDetails:IsShown() then
        self.detailKey=name;self.selectedOutfit=name;self.outfitName:SetText(name)
        self.outfitMessage:SetText("Saved changes to "..name..".");self:Refresh();self:StartOutfitPreview()
    else
        self.unsavedLookMessage="Updated "..name..".";self:Refresh()
    end
end
function V:RefreshPortraits()
    for _,frame in ipairs({self.frame,self.browser,self.outfitDetails}) do
        if frame and frame:IsVisible() then frame.portrait:SetTexture(art.."Logo.tga") end
    end
end
function V:CreateSettingsPage(p)
    -- Share the wardrobe's existing texture and framing without another background asset.
    self.settingsBackground=texture(p,art.."Main.blp",19,75,323,355)
    -- Keep the title and navigation at their established positions.
    self.settingsTitlePanel=CreateFrame("Frame",nil,p)
    self.settingsTitlePanel:SetPoint("TOPLEFT",p,"TOPLEFT",60,-115)
    self.settingsTitlePanel:SetWidth(240);self.settingsTitlePanel:SetHeight(78)
    self.settingsTitle=label(self.settingsTitlePanel,"Saurek's Closet",16,16,208,22)
    self.settingsTitle:SetFont("Fonts\\FRIZQT__.TTF",16)
    self.settingsTagline=label(self.settingsTitlePanel,"A damn fine 1.12 transmog.",16,45,208,17,true)
    self.settingsNavigationButtons={}
    for i,name in ipairs({"privacy","links","updates"}) do
        local x=60+math.mod(i-1,2)*124
        local y=209+math.floor((i-1)/2)*35
        local b=settingsButton(p,({privacy="Internet Settings",links="Links",updates="Version Details"})[name],x,y,116,function() V:OpenInfoPage(this.infoPage) end)
        b.infoPage=name;self.settingsNavigationButtons[name]=b
    end

    local window=sheet("SaureksClosetInformation",UIParent,"Internet Settings")
    self.settingsInfoWindow=window
    window:Hide();window:SetFrameStrata("DIALOG");window:SetClampedToScreen(true)
    window:SetPoint("TOPLEFT",self.frame,"TOPRIGHT",-30,0)
    window:SetMovable(true);window:RegisterForDrag("LeftButton")
    window:SetScript("OnDragStart",function() this:StartMoving() end)
    window:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
    window.close:SetScript("OnClick",function() V.settingsInfoWindow:Hide() end)
    table.insert(UISpecialFrames,window:GetName())
    self.infoPages={privacy=page(window),updates=page(window),links=page(window)}
    local privacy=self.infoPages.privacy
    label(privacy,"Update checks",38,90,284,24)
    local checkbox=CreateFrame("CheckButton","SaureksClosetAutoUpdates",privacy,"UICheckButtonTemplate")
    self.autoUpdatesCheckbox=checkbox
    checkbox:SetPoint("TOPLEFT",privacy,"TOPLEFT",36,-126);checkbox:SetWidth(24);checkbox:SetHeight(24)
    label(privacy,"Automatically check for updates",67,131,254,36,true)
    checkbox:SetScript("OnClick",function() V:SetAutoUpdates(this:GetChecked()) end)
    self.autoUpdatesDescription=label(privacy,"Vanilla Closet can automatically check the program's repository on Github.com to see if a newer version is available. If you disable this feature Vanilla Closet will not check for updates.",67,158,254,0,true)
    self.autoUpdatesDescription:SetFont("Fonts\\FRIZQT__.TTF",11)
    self.autoUpdatesDescription:SetTextColor(.8,.8,.8)

    local updates=self.infoPages.updates
    self.addonVersionsHeading=label(updates,"This PC's Version",30,80,284,18)
    self.addonVersionsHeading:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.versionLegendIcon=texture(updates,"Interface\\Buttons\\UI-CheckBox-Check",31,102,15,15,"OVERLAY")
    self.versionLegendText=label(updates,"indicates version is up to date and no mismatch is detected",49,101,265,31,true)
    self.versionLegendText:SetFont("Fonts\\FRIZQT__.TTF",9);self.versionLegendText:SetTextColor(.72,.72,.72)
    self.versionStatusLabels={}
    self.versionStatusIcons={}
    for i=1,3 do
        local row=CreateFrame("Frame",nil,updates)
        row:SetPoint("TOPLEFT",updates,"TOPLEFT",38,-(136+(i-1)*19));row:SetWidth(190);row:SetHeight(18)
        local text=label(row,"",0,0,132,18,true);text:SetFont("Fonts\\FRIZQT__.TTF",12);text:SetJustifyV("MIDDLE")
        local icon=row:CreateTexture(nil,"OVERLAY");icon:SetWidth(16);icon:SetHeight(16);icon:SetPoint("LEFT",row,"LEFT",138,0)
        icon:Hide();self.versionStatusIcons[i]=icon
        self.versionStatusLabels[i]=text
    end
    self.availableVersionsHeading=label(updates,"Available Version (Github)",30,208,284,18)
    self.availableVersionsHeading:SetFont("Fonts\\FRIZQT__.TTF",12)
    self.availableVersionLabels={}
    for i=1,2 do
        local line=label(updates,"",38,231+(i-1)*19,276,18,true)
        line:SetFont("Fonts\\FRIZQT__.TTF",12);line:SetJustifyV("MIDDLE")
        self.availableVersionLabels[i]=line
    end
    self.lastVersionCheckLabel=label(updates,"Last checked: Never",38,274,276,18,true)
    self.lastVersionCheckLabel:SetFont("Fonts\\FRIZQT__.TTF",10);self.lastVersionCheckLabel:SetTextColor(.72,.72,.72)
    self.updatesSummary=label(updates,"",38,296,284,25,true)
    self.updatesSummary:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.updatesStatus=label(updates,"",38,323,284,20,true)
    self.checkUpdatesButton=settingsButton(updates,"Check for updates",38,352,284,function() V:CheckForUpdates(true) end)
    self.downloadUpdateButton=settingsButton(updates,"Open download page",38,387,284,function() V:OpenWebsite(2) end)

    local links=self.infoPages.links
    self.githubButton=settingsButton(links,"Open GitHub",38,96,284,function() V:OpenWebsite(1) end)
    self.releasesButton=settingsButton(links,"Open download page",38,135,284,function() V:OpenWebsite(2) end)
    self.discordButton=settingsButton(links,"Open support Discord",38,174,284,function() V:OpenWebsite(3) end)
    label(links,"Website address",38,247,284,20)
    self.websiteAddress=edit(links,"SaureksClosetWebsiteAddress",42,274,272,200)
    self.websiteAddress:SetText(self.websiteURLs[1])
    label(links,"You can copy this address if your browser does not open.",38,311,284,46,true)
    self:RefreshUpdateUI()
end
function V:OpenInfoPage(name)
    if not self.infoPages or not self.infoPages[name] then return end
    for key,p in pairs(self.infoPages) do if key==name then p:Show() else p:Hide() end end
    self.settingsInfoWindow.title:SetText(({privacy="Internet Settings",updates="Version Details",links="Links"})[name])
    self.settingsInfoWindow:Show()
    self:RefreshUpdateUI()
end
function V:SetHideHigherLevelItems(hide)
    VanityStudioDB.hideHigherLevelItems=hide and true or false
    self.offset=0
    local item=self.draft and self.index[self.draft.id]
    if hide and item and self:ItemBrowseLevel(item)>(UnitLevel("player") or 0) then
        self:CancelDraft();self:RefreshPreview()
    end
    self:RefreshList()
end
function V:RefreshList()
    if not self.uiReady or not self.rows then return end
    local playerLevel=UnitLevel("player") or 0
    local maxLevel=VanityStudioDB.hideHigherLevelItems and playerLevel or nil
    self.hideHigherLevelCheckbox:SetChecked(VanityStudioDB.hideHigherLevelItems and 1 or nil)
    local matches=self:Filter(self.slot,self.query,self.quality,self.material,self.favoritesOnly,maxLevel)
    self.maxOffset=math.max(0,table.getn(matches)-self.visibleItemRows);self.offset=math.max(0,math.min(self.offset,self.maxOffset))
    if self.offset==0 then self.scrollUp:Disable() else self.scrollUp:Enable() end
    if self.offset==self.maxOffset then self.scrollDown:Disable() else self.scrollDown:Enable() end
    self.updatingScroll=true;self.scroll:SetMinMaxValues(0,self.maxOffset);self.scroll:SetValue(self.offset);self.updatingScroll=nil
    if self.maxOffset>0 then self.scroll:Show();self.scrollThumb:Show() else self.scrollThumb:Hide();self.scroll:Hide() end
    self.qualityLabel:SetText(self.quality=="unobtainable" and "Unobtainable" or ("Quality: "..(self.qualityNames[self.quality] or "All")))
    self.materialLabel:SetText("Type: "..self:ItemTypeName(self.material,self.slot))
    self.browser.title:SetText(self.slotNames[self.slot].." Appearance")
    self.resultsLabel:SetText(table.getn(matches)..(table.getn(matches)==1 and " Item" or " Items"))
    if table.getn(matches)==0 then self.emptyLabel:Show() else self.emptyLabel:Hide() end
    for i,b in ipairs(self.rows) do
        local item=matches[self.offset+i];b.item=item
        if item then
            b:Show();b.nameLabel:SetText(item[2])
            local n,l,q,level,typ,sub,stack,loc,icon=GetItemInfo(item[1])
            b.icon:SetTexture(icon or self:CatalogIcon(item[1]) or "Interface\\PaperDoll\\UI-PaperDoll-Slot-"..icons[self.slot])
            -- Use the equip requirement, or a known quest minimum for rewards with none.
            local requiredLevel=self:ItemBrowseLevel(item)
            local levelColor=requiredLevel>playerLevel and (RED_FONT_COLOR_CODE or "|cffff2020") or (HIGHLIGHT_FONT_COLOR_CODE or "|cffffffff")
            b.metaLabel:SetText((item.unobtainable and "Unobtainable" or self.qualityNames[item[4]] or "").."  /  "..levelColor.."Level "..requiredLevel.."|r  /  "..item[1])
            if self.draft and self.draft.id==item[1] then b.selected:Show() else b.selected:Hide() end
        else b:Hide() end
    end
    enabled(self.commitButton,self.draft~=nil)
end
function V:ActivateSavedOutfit(name)
    if not name or not self:GetOutfit(name) then return false end
    local wasEnabled=VanityStudioCharacter.enabled
    VanityStudioCharacter.enabled=true
    if self:LoadOutfit(name) then
        self.outfitMessage:SetText("");self:RefreshOutfits();self:RefreshOutfitDetails();return true
    end
    VanityStudioCharacter.enabled=wasEnabled
    self.outfitMessage:SetText(self.bodyError or "Race renderer unavailable.");self:Refresh()
    return false
end
function V:CloseOutfitMenu()
    if (self.outfitMenu and UIDROPDOWNMENU_OPEN_MENU==self.outfitMenu:GetName()) or
        (self.slotMenu and UIDROPDOWNMENU_OPEN_MENU==self.slotMenu:GetName()) or
        (self.wardrobeMenu and UIDROPDOWNMENU_OPEN_MENU==self.wardrobeMenu:GetName()) or
        (self.bodyMenu and UIDROPDOWNMENU_OPEN_MENU==self.bodyMenu:GetName()) then CloseDropDownMenus() end
end
function V:ToggleOutfitMenu(pageNumber)
    GameTooltip:Hide()
    if pageNumber then self:CloseOutfitMenu();self.outfitMenuPage=pageNumber
    elseif UIDROPDOWNMENU_OPEN_MENU~=self.outfitMenu:GetName() or not DropDownList1:IsShown() then
        self.outfitMenuPage=1
        for i,name in ipairs(self:OutfitKeys()) do
            if self:IsOutfitActive(name) then self.outfitMenuPage=math.ceil(i/12);break end
        end
    end
    ToggleDropDownMenu(1,nil,self.outfitMenu,self.outfitSelector:GetName(),0,0)
end
function V:BuildOutfitMenu(level)
    if level and level~=1 then return end
    local function add(info) UIDropDownMenu_AddButton(info,1) end
    add({text="Saved Looks - Quick Selection",isTitle=1,notCheckable=1})
    local names=self:OutfitKeys();local count=table.getn(names)
    if count==0 then
        add({text="No outfits yet",disabled=1,notCheckable=1})
        add({text="Customize appearance...",notCheckable=1,func=function() V:SetTab("armor") end})
        return
    end
    local pages=math.ceil(count/12)
    local pageNumber=math.max(1,math.min(self.outfitMenuPage or 1,pages));self.outfitMenuPage=pageNumber
    if pages>1 then
        -- Keep paging controls at fixed indices. Blizzard's click handler toggles its
        -- check texture after callbacks, so leave these navigation rows checked to hide it.
        local function turnPage(number)
            local clicked=this;V:ToggleOutfitMenu(number);clicked.checked=1
        end
        add({text="Previous page",notCheckable=1,disabled=pageNumber==1 and 1 or nil,
            keepShownOnClick=1,func=turnPage,arg1=pageNumber-1})
        add({text="Next page",notCheckable=1,disabled=pageNumber==pages and 1 or nil,
            keepShownOnClick=1,func=turnPage,arg1=pageNumber+1})
    end
    for i=(pageNumber-1)*12+1,math.min(pageNumber*12,count) do
        local name=names[i]
        add({text=self:OutfitLabel(name),value=name,arg1=name,
            checked=VanityStudioCharacter.enabled and self:IsOutfitActive(name) and 1 or nil,
            func=function(outfitName) V:ActivateSavedOutfit(outfitName);V:CloseOutfitMenu() end})
    end
end
function V:RefreshUnsavedLookPanel()
    if not self.unsavedLookPanel then return end
    local look=self:GetOutfit(self.UNSAVED)
    local hasUnsaved=look~=nil and self:IsOutfitActive(self.UNSAVED)
    local savedCount=table.getn(self:OutfitNames())
    self.unsavedOutfitName:EnableMouse(hasUnsaved)
    -- Vanilla 1.12 fires OnEditFocusLost for a programmatic ClearFocus.
    -- Clear our state first so that callback's refresh cannot recurse here.
    if not hasUnsaved and self.unsavedNameFocused then
        self.unsavedNameFocused=nil
        self.unsavedOutfitName:ClearFocus()
    end
    self.unsavedOutfitName:SetAlpha(hasUnsaved and 1 or .4)
    local name=self.unsavedOutfitName:GetText() or ""
    if hasUnsaved and name=="" and not self.unsavedNameFocused then self.unsavedOutfitNameHint:Show()
    else self.unsavedOutfitNameHint:Hide() end
    enabled(self.saveUnsavedAsNewButton,hasUnsaved and string.find(name,"%S")~=nil)
    enabled(self.updateSavedLookButton,hasUnsaved and savedCount>0)
end
function V:RefreshOutfits()
    if not self.uiReady then return end
    local savedCount=table.getn(self:OutfitNames())
    local hasUnsaved=self:GetOutfit(self.UNSAVED)~=nil and self:IsOutfitActive(self.UNSAVED)
    if hasUnsaved then self.unsavedLookPanel:Show() else self.unsavedLookPanel:Hide() end
    local headingY=hasUnsaved and 129 or 10
    local dividerY=hasUnsaved and 146 or 27
    local rowY=hasUnsaved and 152 or 33
    local emptyY=hasUnsaved and 178 or 116
    self.visibleOutfitRows=hasUnsaved and 3 or 5
    self.savedLooksHeading:ClearAllPoints();self.savedLooksHeading:SetPoint("TOPLEFT",self.outfitList,"TOPLEFT",10,-headingY)
    self.savedLooksCount:ClearAllPoints();self.savedLooksCount:SetPoint("TOPLEFT",self.outfitList,"TOPLEFT",238,-headingY)
    self.savedLooksCount:SetText("("..savedCount..")")
    self.savedLooksDivider:ClearAllPoints();self.savedLooksDivider:SetPoint("TOPLEFT",self.outfitList,"TOPLEFT",10,-dividerY)
    self.outfitEmpty:ClearAllPoints();self.outfitEmpty:SetPoint("TOPLEFT",self.outfitList,"TOPLEFT",18,-emptyY)
    for i,row in ipairs(self.outfitRows) do
        row:ClearAllPoints();row:SetPoint("TOPLEFT",self.outfitList,"TOPLEFT",3,-(rowY+(i-1)*58))
    end
    self:RefreshUnsavedLookPanel()
    local keys=self:OutfitNames()
    self.outfitOffset=math.max(0,math.min(self.outfitOffset or 0,math.max(0,table.getn(keys)-self.visibleOutfitRows)))
    local maxOffset=math.max(0,table.getn(keys)-self.visibleOutfitRows)
    self.updatingOutfitScroll=true;self.outfitScroll:SetMinMaxValues(0,maxOffset);self.outfitScroll:SetValue(self.outfitOffset);self.updatingOutfitScroll=nil
    enabled(self.outfitScrollUp,self.outfitOffset>0);enabled(self.outfitScrollDown,self.outfitOffset<maxOffset)
    if maxOffset>0 then
        self.outfitScroll:Show();self.outfitScrollThumb:Show()
    else
        -- Like the Skills tab, hide the controls, leaving the decoration behind.
        self.outfitScrollThumb:Hide();self.outfitScroll:Hide()
    end
    if table.getn(keys)==0 then self.outfitEmpty:Show() else self.outfitEmpty:Hide() end
    for i,row in ipairs(self.outfitRows) do
        local key=i<=self.visibleOutfitRows and keys[self.outfitOffset+i] or nil;row.outfit=key
        if key then
            local outfit=self:GetOutfit(key)
            row:Show();row.title:SetText(key);self:BindOutfitPortrait(row,outfit)
            if self:IsOutfitActive(key) then
                row.active:Show();row.status:Show();row.selected:Hide();row:SetBackdropBorderColor(1,.75,.16);row.title:SetTextColor(1,1,1)
            elseif key==self.selectedOutfit then
                row.active:Hide();row.status:Hide();row.selected:Show();row:SetBackdropBorderColor(.6,.55,.45);row.title:SetTextColor(1,1,1)
            else
                row.active:Hide();row.status:Hide();row.selected:Hide();row:SetBackdropBorderColor(.6,.55,.45);row.title:SetTextColor(1,.82,0)
            end
            row.detail:SetText(date("%b %d, %Y  %I:%M %p",outfit.updatedAt or 0))
        else row:Hide() end
    end
end
function V:ActiveOutfitText()
    local c=VanityStudioCharacter
    local name=c.activeUnsaved and "(Unsaved Look)" or c.activeOutfit or "No outfit"
    return name..(not c.enabled and " (off)" or "")
end
function V:Refresh()
    if not self.uiReady or not self.frame or not self.frame:IsShown() then return end
    self:RefreshUpdateUI()
    local c=VanityStudioCharacter
    self.enabledButton.caption:SetText(c.enabled and "Disable" or "Enable")
    local storedAvailable=self:WeaponStoredVisibilityAvailable()
    for _,field in ipairs({"hideRangedWhenStored","hideMeleeWhenStored"}) do
        local checkbox=self[field.."Checkbox"]
        if checkbox then
            checkbox:SetChecked(c.weapons and c.weapons[field] and 1 or nil)
            enabled(checkbox,storedAvailable);checkbox:SetAlpha(storedAvailable and 1 or .45)
            self[field.."Label"]:SetAlpha(storedAvailable and 1 or .45)
        end
    end
    if self.weaponNotice then
        self.weaponNotice:SetText(not storedAvailable and "Update SaureksCloset.dll and restart WoW." or self.weaponError or "")
    end
    self.activeOutfitLabel:SetText(self:ActiveOutfitText())
    if self.bagSlots then
        self.bagSlots[1].icon:SetTexture(c.weapons and c.weapons.backBag==1 and "Interface\\Icons\\INV_Misc_Bag_08" or "Interface\\PaperDoll\\UI-PaperDoll-Slot-Bag")
    end
    for slot,b in pairs(self.slotButtons) do
        local id=self:SlotSelection(slot);local icon
        if id and id>0 then local n,l,q,lev,typ,sub,stack,loc,path=GetItemInfo(id);icon=path or self:CatalogIcon(id)
        end
        b.icon:SetTexture(icon or "Interface\\PaperDoll\\UI-PaperDoll-Slot-"..(icons[slot] or self.weaponIcons[slot-100]))
        if id==0 then b.hiddenOverlay:Show() else b.hiddenOverlay:Hide() end
    end
    self:RefreshSlotHighlights()
    if self.tab=="armor" or self.tab=="weaponry" or self.tab=="bags" or self.tab=="exposure" then self:RefreshPreview()
    elseif self.tab=="body" then self:RefreshBody();self:RefreshPreview()
    elseif self.tab=="outfits" then self:RefreshOutfits() end
    if self.browser:IsShown() then self:RefreshList() end
    if self.outfitDetails and self.outfitDetails:IsShown() then self:RefreshOutfitDetails() end
end
