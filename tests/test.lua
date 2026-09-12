function UnitRace() return "Human", "Human" end
function UnitSex() return 2 end
-- Run with the actual Lua 5.0.3 interpreter, from the VanityStudio directory.
local passed = 0
local function check(condition, message)
    if not condition then error(message or "Assertion failed") end
    passed=passed+1
end
local frames = {}
local methods = {}
local function noop() end
for _,name in ipairs({"SetFont","SetTextColor","SetJustifyH","SetBackdrop","SetBackdropColor","SetBackdropBorderColor","SetHighlightTexture","SetTextInsets","SetAutoFocus","SetMaxLetters","ClearFocus","SetClampedToScreen","SetMovable","RegisterForDrag","StartMoving","StopMovingOrSizing","SetFrameStrata","EnableMouse","EnableMouseWheel","SetNormalTexture","SetOwner","AddLine","RegisterEvent","SetAllPoints","SetAlpha","RegisterForClicks","SetFocus","SetJustifyV","SetHitRectInsets","SetPushedTexture","SetVertexColor","SetValueStep","SetDisabledTexture","SetTexCoord","SetBlendMode"}) do methods[name]=noop end
function methods:SetFont(path,size) self.fontPath=path;self.fontSize=size end
function methods:SetJustifyV(v) self.justifyV=v end
function methods:SetAlpha(v) self.alpha=v end
function methods:SetBlendMode(v) self.blend=v end
function methods:SetWidth(v) self.width=v end
function methods:SetHeight(v) self.height=v end
function methods:GetWidth() return self.width or 1920 end
function methods:GetHeight()
    if self.kind=="FontString" and self.height==0 then
        local lines=0;local columns=math.max(1,math.floor((self.width or 273)/6))
        for line in string.gfind((self:GetText()).."\n","([^\n]*)\n") do
            lines=lines+math.max(1,math.ceil(string.len(line)/columns))
        end
        return lines*(self.fontSize or 12)
    end
    return self.height or 1080
end
function methods:SetScrollChild(child) self.scrollChild=child end
function methods:SetVerticalScroll(value) self.verticalScroll=value end
function methods:SetScale(v) self.scale=v end
function methods:SetPoint(...) self.point=arg end
function methods:ClearAllPoints() self.point=nil end
function methods:SetText(v)
    self.textValue=v
    if self.scripts.OnTextChanged then local old=this; this=self; self.scripts.OnTextChanged(); this=old end
end
function methods:GetName() return self.name end
function methods:GetParent() return self.parent end
function methods:GetFrameLevel() return self.frameLevel or (self.parent and self.parent:GetFrameLevel()+1) or 0 end
function methods:SetFrameLevel(level) self.frameLevel=level end
function getglobal(name) return _G[name] end
function setglobal(name,value) _G[name]=value end
function methods:GetCenter() return 1750,950 end
function methods:GetEffectiveScale() return 1 end
function SetPortraitToTexture(texture,path) texture.portrait=true;texture.portraitUpdates=(texture.portraitUpdates or 0)+1;texture:SetTexture(path) end
function methods:GetText() return self.textValue or "" end
function methods:GetTextWidth() return string.len(self:GetText())*6 end
function methods:SetTexture(v) check(type(v)=="string","Texture must be a path") self.texture=v end
function methods:SetScript(name,func) self.scripts[name]=func end
function methods:Show()
    local was=self.shown;self.shown=true
    if not was and self.scripts.OnShow then local old=this;this=self;self.scripts.OnShow();this=old end
end
function methods:Hide()
    local was=self.shown;self.shown=false
    if was and self.scripts.OnHide then local old=this;this=self;self.scripts.OnHide();this=old end
end
function methods:IsVisible() return self.shown and (not self.parent or self.parent:IsVisible()) end
function methods:SetMinMaxValues(a,b) self.min=a;self.max=b end
function methods:SetValue(v)
    self.value=v
    if self.scripts.OnValueChanged then local old=this;this=self;self.scripts.OnValueChanged();this=old end
end
function methods:GetValue() return self.value end
function methods:SetChecked(value) self.checked=value and true or false end
function methods:GetChecked() return self.checked and 1 or nil end
function methods:IsOwned() return false end
function methods:GetButtonState() return "NORMAL" end
function methods:ClearModel() self.clears=(self.clears or 0)+1;self.loaded=nil end
function methods:SetCamera(v) self.camera=v end
function methods:GetLeft() return 30 end
function methods:GetBottom() return 300 end
function methods:GetRight() return 78 end
function methods:GetTop() return 348 end
function methods:SetSequenceTime(sequence,time) self.sequence=sequence;self.sequenceTime=time end
function methods:SetSequence(v) self.sequence=v end
function methods:SetVertexColor(r,g,b,a) self.color={r,g,b,a} end
function methods:SetRotation(v) self.rotation=v end
function methods:IsShown() return self.shown end
function methods:Disable() self.disabled=true end
function methods:Enable() self.disabled=false end
function methods:SetHyperlink(link) self.link=link end
function methods:SetFacing(v) self.facing=v end
function methods:SetPosition(x,y,z) self.position={x,y,z} end
function methods:SetUnit(unit)
    if unit~="player" then error("Unknown unit name: "..tostring(unit)) end
end
function methods:Undress() self.tried={} end
function methods:TryOn(link) check(type(link)=="string" and tonumber(link),"1.12 TryOn receives numeric ID string");table.insert(self.tried,link) end
function CreateFrame(kind,name,parent,template)
    local f=setmetatable({kind=kind,name=name,parent=parent,template=template,scripts={},shown=true}, {__index=function(t,k)
        if not methods[k] and string.find(k,"^[A-Z]") then error("Unexpected frame method: "..k) end
        return methods[k]
    end})
    table.insert(frames,f); if name then _G[name]=f end
    if name=="SaureksClosetModel" then
        local v=VanityStudio;local shown=v.frame.shown
        v.frame.shown=true
        check(not v.uiReady,"Exercise reentrant display before construction finishes")
        v.frame.scripts.OnShow()
        v.frame.shown=shown
        check(not v.uiReady,"Window remains unready until all pages are constructed")
    end
    if (template=="UIPanelButtonTemplate" or template=="UIPanelButtonTemplate2") then _G[name.."Text"]=CreateFrame("FontString",nil,f) end
    if template=="UIPanelButtonTemplate2" then
        for _,part in ipairs({"Left","Middle","Right"}) do _G[name..part]=CreateFrame("Texture",nil,f) end
    end
    if template=="UIPanelScrollBarTemplate" then
        _G[name.."ThumbTexture"]=CreateFrame("Texture",nil,f)
        _G[name.."ScrollUpButton"]=CreateFrame("Button",nil,f)
        _G[name.."ScrollDownButton"]=CreateFrame("Button",nil,f)
    end
    if template=="ItemButtonTemplate" then _G[name.."IconTexture"]=CreateFrame("Texture",nil,f) end
    if template=="CharacterFrameTabButtonTemplate" then
        for _,suffix in ipairs({"Left","Middle","Right","LeftDisabled","MiddleDisabled","RightDisabled","Text","HighlightTexture"}) do
            _G[name..suffix]=CreateFrame("Texture",nil,f);_G[name..suffix]:SetWidth(16)
        end
        f.scripts.OnClick=function() error("Inherited CharacterFrame callback must be replaced") end
    end
    return f
end
function methods:CreateFontString() return CreateFrame("FontString",nil,self) end
function methods:CreateTexture() return CreateFrame("Texture",nil,self) end
-- Mock dropdown placement/widgets, but exercise the client's original click lifecycle.
local menuEntries={}
DropDownList1=CreateFrame("Frame","DropDownList1");DropDownList1:Hide()
DropDownList2=CreateFrame("Frame","DropDownList2");DropDownList2:Hide()
function CloseDropDownMenus(level) if not level or level==1 then DropDownList1:Hide() end;DropDownList2:Hide() end
function UIDropDownMenu_AddButton(info,level)
    level=level or 1
    check(table.getn(menuEntries)<32,"Dropdown stays within vanilla's native row limit")
    table.insert(menuEntries,info)
    local name="DropDownList"..level.."Button"..table.getn(menuEntries)
    local b=getglobal(name) or CreateFrame("Button",name,getglobal("DropDownList"..level))
    if not getglobal(name.."Check") then CreateFrame("Texture",name.."Check",b) end
    b.func=info.func;b.arg1=info.arg1;b.arg2=info.arg2;b.checked=info.checked
    b.keepShownOnClick=info.keepShownOnClick;b.disabled=info.disabled
    b:SetScript("OnClick",function() UIDropDownMenuButton_OnClick() end)
end
function ToggleDropDownMenu(level,value,owner,anchor,x,y)
    check((level==2 and value=="savedlooks") or (level==1 and (anchor=="SaureksClosetOutfitSelector" or (VanityStudio.replaceOutfitButton and anchor==VanityStudio.replaceOutfitButton:GetName()) or string.find(anchor,"SaureksClosetSlot",1,true)==1 or anchor=="SaureksClosetQualityFilter" or anchor=="SaureksClosetTypeFilter" or anchor=="SaureksClosetWardrobeSelector" or string.find(anchor,"SaureksClosetBody",1,true)==1)),"Dropdown anchors to its nameplate, slot or filter")
    local list=getglobal("DropDownList"..level)
    if list:IsShown() and UIDROPDOWNMENU_OPEN_MENU==owner:GetName() then CloseDropDownMenus(level);return end
    UIDROPDOWNMENU_OPEN_MENU=owner:GetName();UIDROPDOWNMENU_MENU_LEVEL=level;UIDROPDOWNMENU_MENU_VALUE=value
    menuEntries={};owner.initialize(level);list:Show()
end
function UIDropDownMenu_Initialize(owner,initialize,displayMode,level)
    menuEntries={};initialize(level)
end
local dropdownFile=assert(io.open("research/client-data/UIDropDownMenu.lua","r"))
local dropdownSource=dropdownFile:read("*a");dropdownFile:close()
local clickStart=assert(string.find(dropdownSource,"function UIDropDownMenuButton_OnClick()",1,true))
local clickEnd=assert(string.find(dropdownSource,"function HideDropDownMenu",clickStart,true))
assert(loadstring(string.sub(dropdownSource,clickStart,clickEnd-1)))()
UIParent=CreateFrame("Frame","UIParent")
Minimap=CreateFrame("Frame","Minimap")
GameTooltip=CreateFrame("GameTooltip","GameTooltip")
DEFAULT_CHAT_FRAME={AddMessage=noop}
UISpecialFrames={}; SlashCmdList={};UIPanelWindows={}
UIChildWindows={};ROTATIONS_PER_SECOND=.5;PI=math.pi
function SetPortraitTexture(t,unit) check(unit=="player");t.portraitUpdates=(t.portraitUpdates or 0)+1;t:SetTexture("player-portrait") end
function UnitIsDead() return false end
function CloseAllBags() end
function PlaySound() end
-- Exercise Blizzard's actual panel positioning/lifecycle and rotation functions.
local f=assert(io.open("research/client-data/UIParent.lua","r"));local source=f:read("*a");f:close()
local start=assert(string.find(source,"function ShowUIPanel",1,true))
local finish=assert(string.find(source,"function CloseAllWindows_WithExceptions",start,true))
assert(loadstring(string.sub(source,start,finish-1)))()
start=assert(string.find(source,"function Model_OnLoad",1,true))
finish=assert(string.find(source,"-- Function that handles the escape",start,true))
assert(loadstring(string.sub(source,start,finish-1)))()
dofile("research/client-data/UIPanelTemplates.lua")
local now=100
function GetTime() return now end
function GetCursorPosition() return 400,300 end
local cache={}
function GetItemInfo(id)
    if cache[id] then return "Cached item", "item:"..id..":0:0:0", 4,60,"Armor","Plate",1,"INVTYPE_HEAD","Interface\\Icons\\INV_Helmet_01" end
end
local real={[1]=111,[3]=222,[5]=333,[7]=444}
function GetInventoryItemLink(unit,slot) return real[slot] and "item:"..real[slot]..":0:0:0" end
function GetInventoryItemTexture() return "Interface\\Icons\\INV_Helmet_01" end
function GetBuildInfo() return "1.12.1","5875" end
function GetAddOnMetadata(name,key)
    return "3.4.1" -- Simulate stale startup metadata after an addon-only UI reload.
end

local world={}
local calls={}
local fail=false
function SetUnitVisibleItemID(unit,slot,...)
    check(unit=="player","Never modify other players")
    check(arg.n==0 or (arg.n==1 and arg[1]>=0),"Only explicit nonnegative IDs can be overrides; nil must be omitted")
    if fail then error("simulated helper failure") end
    table.insert(calls,{slot=slot,id=arg[1],count=arg.n})
    world[slot]=arg[1]
end
local function visible(slot) return world[slot] or real[slot] end
local function click(b)
    local old=this; this=b; if not b.disabled then b.scripts.OnClick() end; this=old
end
dofile("addon/SaureksCloset/Catalog.lua")
dofile("addon/SaureksCloset/ItemIcons.lua")
dofile("addon/SaureksCloset/BodyData.lua")
dofile("addon/SaureksCloset/WeaponData.lua")
dofile("addon/SaureksCloset/Core.lua")
dofile("addon/SaureksCloset/Updates.lua")
dofile("addon/SaureksCloset/Weaponry.lua")
dofile("addon/SaureksCloset/Body.lua")
dofile("addon/SaureksCloset/Preview.lua")
dofile("addon/SaureksCloset/OutfitPreview.lua")
dofile("addon/SaureksCloset/OutfitPortraits.lua")
dofile("addon/SaureksCloset/UI.lua")
local V=VanityStudio
VanityStudioDB={outfits={["Legacy armor"]={[1]=16955}}}
event="ADDON_LOADED"; arg1="SaureksCloset"; V.events.scripts.OnEvent()
check(V.ready,"Addon initialization")
check(V:UsingTrueModel() and VanityStudioCharacter.body==nil,"First run defaults to True Model without creating a body override")
check(VanityStudioDB.outfits["Legacy armor"].version==2 and VanityStudioDB.outfits["Legacy armor"].slots[1]==16955,"Legacy outfits migrate without losing armor")
check(table.getn(VanityStudioCatalog)==9665,"Complete pinned catalog")
local head=V.slots[1][1][1]
local other=V.slots[1][2][1]
local shoulder=V.slots[3][1][1]
cache[head]=true; cache[shoulder]=true
V:Sync()
check(table.getn(calls)==0,"Empty defaults must not create overrides")
check(visible(1)==111,"Defaults pass through")
V:Select(1,head)
check(visible(1)==head,"Selected appearance applies")
check(real[1]==111,"Selection never equips an item")
real[1]=555; V:Sync()
check(visible(1)==head,"Override survives real equipment changes")
V:ClearSlot(1)
check(visible(1)==555,"Clear restores CURRENT real gear, not original gear")
check(calls[table.getn(calls)].count==0,"Clear omits the third helper argument")
real[1]=777; V:Sync()
check(visible(1)==777,"Real appearance remains live after clearing")
V:Select(1,head); V:Select(3,shoulder); V:SetEnabled(false)
check(visible(1)==777 and visible(3)==222,"Pause releases every owned override")
check(VanityStudioCharacter.selected[1]==head,"Pause retains outfit choices")
V:SetEnabled(true); check(visible(1)==head,"Resume reapplies selected pieces")
check(V:SaveOutfit(" First "),"Save outfit")
check(not V:SaveOutfit("First"),"Do not silently overwrite an outfit")
check(not V:SaveOutfit("  "),"Reject empty names")
V:ClearSlot(3); V:SaveOutfit("Head only")
V:LoadOutfit("First"); check(visible(3)==shoulder,"Load multi-slot outfit")
V:LoadOutfit("Head only"); check(visible(3)==222,"Unspecified loaded outfit slots restore real gear")
V:Select(1,other)
check(visible(1)==777,"Uncached choice releases previous appearance")
check(V.pending[1] and V.pending[1].attempts==1,"Uncached item queued once")
for i=1,30 do now=now+.5; V:Sync() end
check(V.pending[1].attempts==3,"Missing items have bounded retries")
check(V.errors[1]~=nil,"Unavailable data shown as an error")
cache[other]=true; V:Sync(); check(visible(1)==other,"Late cache data applies")
V:Select(1,head); fail=true; V:ClearSlot(1)
check(VanityStudioCharacter.managed[1],"Failed reset keeps ownership for retry")
fail=false; V:Retry(); check(visible(1)==777,"Retry clears a failed reset")
check(not V:Select(3,head),"Reject wrong slot")
check(V:Select(1,0),"Explicit hide is supported")
check(visible(1)==0 and not V.pending[1],"Hidden slot does not query item zero")
V:ClearSlot(1);check(visible(1)==777,"Clearing a hidden slot restores real gear")
check(table.getn(V:Filter(1,tostring(head)))==1,"Find by item ID")
VanityStudioDB.favorites[head]=true
check(table.getn(V:Filter(1,"",nil,nil,true))==1,"Favorite filter")
for _,item in ipairs(V:Filter(5,"",4,404,false)) do check(item[4]==4 and item[6]==4,"Quality and material filters") end
V:Toggle(true)
check(V.tabButtons.character:GetText()=="Wardrobe" and V.tabButtons.outfits:GetText()=="Saved Looks" and V.tabButtons.settings:GetText()=="Settings","Primary tabs use the new navigation labels")
local savedTab=V.tabButtons.outfits
local initialTabWidth=savedTab:GetWidth()
for i=1,3 do savedTab:Hide();savedTab:Show() end
check(savedTab:GetWidth()==initialTabWidth and getglobal(savedTab:GetName().."Text"):GetWidth()>=savedTab:GetTextWidth()+12,"Saved Looks retains full single-line text width and padding across repeated shows")
click(V.wardrobeSelector)
check(table.getn(menuEntries)==4 and menuEntries[2].text=="Weaponry" and menuEntries[4].text=="Bags" and not menuEntries[1].isTitle and menuEntries[1].text=="Outfit" and menuEntries[1].checked and menuEntries[3].text=="Body","Wardrobe selector orders Outfit, Weaponry, Body, Bags with the active choice checked")
click(getglobal("DropDownList1Button3"))
check(V.tab=="body" and V.pagesByName.body:IsVisible() and V.wardrobeSelectorLabel:GetText()=="Body" and not DropDownList1:IsShown(),"Race selection switches pages and closes the menu")
click(V.tabButtons.outfits)
check(V.tab=="outfits" and not V.wardrobeSelectorBox:IsVisible(),"Saved Looks hides the wardrobe selector")
click(V.tabButtons.character)
check(V.tab=="body" and V.wardrobeSelectorBox:IsVisible(),"Returning to Wardrobe restores its selected page")
click(V.wardrobeSelector);click(getglobal("DropDownList1Button1"))
check(V.tab=="armor" and V.pagesByName.armor:IsVisible() and V.wardrobeSelectorLabel:GetText()=="Outfit","Outfit choice returns to the outfit editor")
click(V.tabButtons.settings)
check(V.pagesByName.settings:IsVisible() and not V.wardrobeSelectorBox:IsVisible(),"Settings hides the wardrobe selector")
click(V.tabButtons.character)
click(V.wardrobeSelector);HideUIPanel(V.frame)
check(not DropDownList1:IsShown(),"Closing Wardrobe dismisses the page dropdown")
V:Toggle(true)
check(V.frame:IsShown(),"Window opens")
check(V.uiReady and V.slotButtons[8] and V.browser,"Every required widget exists before the first refresh")
check(not V.quickLook and not V.quickPrev and not V.quickNext,"Armor footer has been removed")
check(table.getn(V.armorSlotPanels)==1 and V.armorDecorationFrame.width==327 and V.armorDecorationFrame.height==357,"One combined decoration and shadow texture preserves the Outfit control frame")
check(V.slotButtons[1]:GetParent()==V.armorDecorationFrame and V.armorDecorationFrame:GetFrameLevel()>V.model:GetFrameLevel(),"Item buttons sit above the decoration panels and character model")
for _,slot in ipairs(V.wardrobeSlots) do
    check(V.slotButtons[slot].width==V.slotButtons[slot].height and V.slotButtons[slot].icon.width==V.slotButtons[slot].icon.height,"Every armor icon is square")
end
check(V.model.height==340 and V.previewBuffer.height==340 and V.groundShadow,"Larger matching model viewports share a ground shadow")
for _,slot in ipairs(V.wardrobeSlots) do check(not V.slotButtons[slot].mark,"Per-slot unsaved markers are removed") end
local manifest=assert(io.open("addon/SaureksCloset/SaureksCloset.toc","r"))
local manifestText=manifest:read("*a");manifest:close()
local _,_,releaseVersion=string.find(manifestText,"## Version: ([^\n]+)")
check(V.VERSION==releaseVersion,"Loaded code version matches the release manifest")
check(V.settingsVersion:GetText()=="Version "..releaseVersion,"Settings displays the loaded release even when client metadata is stale")
V:SetTab("settings")
click(V.settingsNavigationButtons.privacy)
check(V.settingsInfoWindow:IsShown() and V.infoPages.privacy:IsVisible(),"Internet Settings opens a separate window")
check(V.autoUpdatesCheckbox:IsVisible(),"Automatic update preference is visible in Internet Settings")
check(not V.infoPages.about and not V.pagesByName.about and not V.aboutBlocks,"About page and content are removed")
check(V.autoUpdatesCheckbox:GetChecked()==1,"Automatic checks are checked on first installation")
V.autoUpdatesCheckbox:SetChecked(nil);click(V.autoUpdatesCheckbox)
check(VanityStudioDB.autoCheckUpdates==false and V.checkUpdatesButton.disabled and not V.updateDue,"Unchecking automatic updates disables and unschedules network checks")
V.autoUpdatesCheckbox:SetChecked(true);click(V.autoUpdatesCheckbox)
check(VanityStudioDB.autoCheckUpdates==true and not V.checkUpdatesButton.disabled and V.updateDue,"Checking automatic updates re-enables checks")
click(V.settingsNavigationButtons.updates)
check(V.infoPages.updates:IsVisible() and not V.infoPages.privacy:IsVisible() and not V.autoUpdatesCheckbox:IsVisible(),"Version Details opens directly and keeps the preference in Internet Settings")
check(V.settingsInfoWindow.title:GetText()=="Version Details","Version window has its own title")
click(V.settingsNavigationButtons.links)
check(V.infoPages.links:IsVisible() and V.websiteAddress:GetText()==V.websiteURLs[1],"Links page exposes the verified project address")
click(V.settingsInfoWindow.close)
check(not V.settingsInfoWindow:IsShown() and V.frame:IsShown(),"Closing the information window leaves the main addon window open")
V:SetTab("armor")
check(V.frame.width==384 and V.frame.height==512,"Native character-sheet dimensions")
check(V.frame.scripts.OnDragStart==nil,"Main window is fixed")
check(V.model and V.model:IsVisible(),"DressUpModel restored")
check(GetLeftFrame()==V.frame,"Native panel manager owns the main window")
local slotState=V:Copy(VanityStudioCharacter)
local slotCalls=table.getn(calls)
click(V.slotButtons[1])
check(table.getn(menuEntries)==3 and menuEntries[1].text=="Custom Item" and menuEntries[2].text=="Hide Slot" and menuEntries[3].text=="Passthrough","Slot dropdown exposes all three requested actions")
check(not V.browser:IsShown() and table.getn(calls)==slotCalls,"Opening slot actions never changes armor or opens browser")
click(getglobal("DropDownList1Button2"))
check(VanityStudioCharacter.selected[1]==0 and V.slotButtons[1].hiddenOverlay:IsShown() and V.slotButtons[1].icon.texture=="Interface\\PaperDoll\\UI-PaperDoll-Slot-Head","Hidden eye overlays the native default slot image")
check(not V.slotButtons[1].mark,"Hidden slot uses its icon without a text marker")
click(V.slotButtons[1])
check(menuEntries[2].checked and not menuEntries[1].checked and not menuEntries[3].checked,"Only Hide Slot is checked for a hidden slot")
click(getglobal("DropDownList1Button3"))
check(VanityStudioCharacter.selected[1]==nil and visible(1)==real[1] and not V.slotButtons[1].hiddenOverlay:IsShown(),"Passthrough immediately releases the visual override")
check(V.slotButtons[1].icon.texture=="Interface\\PaperDoll\\UI-PaperDoll-Slot-Head","Passthrough displays the native empty-slot icon")
click(V.slotButtons[1])
check(menuEntries[3].checked and not menuEntries[1].checked and not menuEntries[2].checked,"Only Passthrough is checked for an unmodified slot")
V:CloseOutfitMenu();V:Select(1,head);click(V.slotButtons[1])
check(menuEntries[1].checked and not menuEntries[2].checked and not menuEntries[3].checked and not V.slotButtons[1].hiddenOverlay:IsShown(),"Only Custom Item is checked for a custom item")
V:CloseOutfitMenu()
VanityStudioCharacter=slotState;V:Sync();V:Refresh()
for _,slot in ipairs({16,17,18}) do
    local options=V:ItemTypeOptions(slot)
    check(table.getn(options)>0,"Weapon slots offer applicable types")
    for _,key in ipairs(options) do
        local filtered=V:Filter(slot,"",nil,key)
        check(table.getn(filtered)>0,"Every type option has compatible items")
        for _,item in ipairs(filtered) do check(V:ItemTypeKey(item,slot)==key,"Type filter distinguishes item class and subclass") end
        if slot~=17 then check((math.floor(key/100)==2 or key==400),"Weapon slots put old non-weapon props under Other instead of armor materials") end
    end
end
local hasShield=false;for _,key in ipairs(V:ItemTypeOptions(17)) do if key==406 then hasShield=true end end
check(hasShield and V:ItemTypeName(406,17)=="Shield","Off hand retains shields alongside weapons")
for _,slot in ipairs({16,17,18}) do
    check(not V.slotButtons[slot],"Legacy inventory selectors are replaced by body positions")
    V:OpenBrowser(slot);check(not V.browser:IsShown(),"Removed weapon selectors cannot open the item browser")
end
V:SetTab("weaponry")
check(V.model:IsVisible() and V.slotButtons[101]:IsVisible() and not V.slotButtons[1]:IsVisible() and V.wardrobeSelectorLabel:GetText()=="Weaponry","Weaponry shows the shared character and only weapon selectors")
for _,slot in ipairs(V.weaponOrder) do
    click(V.slotButtons[slot]);check(table.getn(menuEntries)==2,"Body positions use Custom Item and Empty")
    click(getglobal("DropDownList1Button1"))
    check(V.browser:IsShown() and V.slot==slot,"Each weapon slot opens the matching appearance browser")
    V:CloseBrowser()
end
V:SetTab("bags")
check(V.model:IsVisible() and V.pagesByName.bags:IsVisible() and not V.slotButtons[101]:IsVisible() and V.wardrobeSelectorLabel:GetText()=="Bags","Bags has its own page and retains the character preview")
V:SetTab("outfits");V:SetTab("character")
check(V.tab=="bags","Wardrobe remembers the Bags section")
V:SetTab("armor")
-- Weapon-category filtering remains shared with the dedicated section.
V:OpenBrowser(1);V.slot=16;click(V.materialButton)
local axeOption
for _,entry in ipairs(menuEntries) do if entry.text=="1H Axes" then axeOption=entry end end
check(axeOption and V.material==nil,"Opening the type dropdown does not change the filter")
axeOption.func();check(V.material==200 and V.materialLabel:GetText()=="Type: 1H Axes","Weapon dropdown applies the chosen weapon type")
V.offset=3;click(V.qualityButton)
local poorOption
for _,entry in ipairs(menuEntries) do if entry.text=="Poor" then poorOption=entry end end
poorOption.func();check(V.quality==0 and V.offset==0 and V.qualityLabel:GetText()=="Quality: Poor","Quality zero selects Poor and resets scrolling")
click(V.qualityButton)
for _,entry in ipairs(menuEntries) do if entry.text=="All" then entry.func() end end
check(V.quality==nil,"All clears the quality filter")
click(V.materialButton)
for _,entry in ipairs(menuEntries) do if entry.text=="All" then entry.func() end end
check(V.material==nil and V.materialLabel:GetText()=="Type: All","All clears the type filter")
check(not V.selectionLabel and V.commitButton:GetText()=="Activate Item","Appearance footer has only the renamed activation button")
V:CloseBrowser()
local priorIcon=V.slotButtons[1].icon.texture
click(V.slotButtons[1]);click(getglobal("DropDownList1Button1"))
check(V.slotButtons[1].icon.texture==priorIcon and V.slotButtons[1].selected.blend=="ADD","Slot selection preserves the image under an additive highlight")
check(V.browser:IsVisible() and GetDoublewideFrame()==V.frame,"Slot opens neighboring window and reserves both columns")
check(table.getn(V.rows)==8,"Scrollable rows initialized")
local selected=VanityStudioCharacter.selected[1];local count=table.getn(calls)
click(V.rows[1])
local draft=V.rows[1].item[1]
check(V.draft.id==draft and VanityStudioCharacter.selected[1]==selected,"Click creates a draft only")
check(table.getn(calls)==count,"Browsing never calls the world helper")
click(V.commitButton)
check(VanityStudioCharacter.selected[1]==draft and not V.draft,"Commit applies chosen item")
V:DraftSlot(1,0);check(V:PreviewItems()[1]==0,"Hide preview omits the slot")
check(VanityStudioCharacter.selected[1]==draft,"Hide draft leaves saved selection intact")
V:CloseBrowser();check(not V.draft and not V.browser:IsVisible(),"Closing cancels draft")
check(GetDoublewideFrame()==nil and GetLeftFrame()==V.frame,"Closing browser releases second column")
click(V.slotButtons[1]);click(getglobal("DropDownList1Button1"));V:DraftSlot(1,0);V:CommitDraft()
check(visible(1)==0,"Commit hidden slot applies zero")
V:DraftSlot(1,nil);check(V:PreviewItems()[1]==real[1],"Real-armor preview uses current actual equipment")
V:CommitDraft();check(visible(1)==real[1],"Real-armor commit releases override")
V:DraftSlot(1,head);V:SetTab("outfits")
check(not V.draft and not V.browser:IsVisible(),"Switching tabs discards draft")
check(V.pagesByName.outfits:IsVisible() and not V.model:IsVisible(),"Bottom tabs switch only closet pages")
V:SetTab("armor");click(V.slotButtons[3]);click(getglobal("DropDownList1Button1"));check(V.slot==3 and V.offset==0,"Slot change resets scroll")
V.search:SetText("no-such-item-at-all");check(V.emptyLabel:IsVisible(),"Empty search")
V.search:SetText("");V.scroll:SetValue(3);check(V.offset==3,"Scrollbar drives list offset")
local old=this;this=V.browser;arg1=-1;V.browser.scripts.OnMouseWheel();this=old
check(V.offset==6,"Mouse wheel scrolls list")
click(V.scrollUp);check(V.offset==5,"Arrow moves one item, not a pixel-derived hundred items")
click(V.scrollDown);check(V.offset==6,"Down arrow moves one item")
check(V.rows[1].star==nil,"Item rows have no unexplained favorite control")
V:DraftSlot(3,shoulder);V:Toggle();check(not V.frame:IsShown() and not V.draft,"Closing main cancels preview")
check(GetDoublewideFrame()==nil,"Closing main releases doublewide state")
V:Toggle(true);check(GetLeftFrame()==V.frame,"Reopening restores ordinary native panel")
V:ClearAll();check(visible(1)==777 and visible(3)==222,"Reset all restores real armor")
VanityStudioCharacter.selected={[1]=head};V.applied={};V.needsSync=true
event="PLAYER_ENTERING_WORLD";V.events.scripts.OnEvent()
arg1=.6;V.events.scripts.OnUpdate();check(visible(1)==head,"World entry restores saved armor")
V:ClearAll()
local helper=SetUnitVisibleItemID;SetUnitVisibleItemID=nil
cache[other]=nil;V:Select(1,other)
check(V.pending[1]~=nil and not V:Available(),"Missing helper permits preview data loading")
check(visible(1)==777,"No helper means no world changes")
SetUnitVisibleItemID=helper
-- Old body functions must remain unreachable, even if an old DLL is still loaded.
function SetUnitDisplayID() error("Forbidden old morph call") end
function SaureksClosetBodyInfo() error("Forbidden old body info") end
function SaureksClosetSetBody() error("Forbidden descriptor write") end
function SaureksClosetClearBody() error("Forbidden old rebuild") end
check(not V:BodyAvailable() and not V:EditBody("race",4),"Old bridge cannot enable unsafe controls")
VanityStudioCharacter.body=V:NormalizeBody({race=4,sex=1,skin=2,face=2})
VanityStudioCharacter.bodyManaged=true
V:HideArmor();V:SaveOutfit("Elf combo")
local saved=V:Copy(VanityStudioDB.outfits["Elf combo"])
V:Sync();V:SyncBody();V:SetEnabled(false);V:SetEnabled(true)
for i=1,20 do arg1=.6;V.events.scripts.OnUpdate() end
check(VanityStudioDB.outfits["Elf combo"].body.face==saved.body.face,"Missing renderer preserves saved appearance")
V:ClearAll();check(not V:LoadOutfit("Elf combo"),"Missing renderer cannot partially load an armor/body combo")
local bodyCalls=0;local renderedBody=nil;local bodyFailure=nil
-- Run body/dropdown/preview regression coverage against the shipped renderer.
local rendererFile=assert(io.open("native/SaureksCloset.cpp","r"))
local rendererSource=rendererFile:read("*a");rendererFile:close()
local _,_,rendererNumber=string.find(rendererSource,"static int __fastcall version.-result%(L,(%d+)%)")
local shippedRenderer=assert(tonumber(rendererNumber),"Native renderer version must be readable")
local testedRenderer=shippedRenderer
function SaureksClosetRendererVersion() return testedRenderer end
function SaureksClosetRealBody() return 1,0,0,0,0,0,0 end
function SaureksClosetSetAppearance(race,sex,skin,face,hair,color,facial)
    if bodyFailure then return bodyFailure end
    bodyCalls=bodyCalls+1;renderedBody={race=race,sex=sex,skin=skin,face=face,hairStyle=hair,hairColor=color,facial=facial};return 1
end
function SaureksClosetClearAppearance() renderedBody=nil;return 1 end
check(V:BodyAvailable(),"Shipped renderer API enables body controls")
for _,compatible in ipairs({30001,30002,30003,30004,30005,30006,30400,30422,30424,30426}) do
    testedRenderer=compatible;check(V:BodyAvailable(),"Released compatible renderer enables body customization")
end
for _,incompatible in ipairs({0,30000,30007,40000,"30400"}) do
    testedRenderer=incompatible;check(not V:BodyAvailable(),"Unrecognized renderer remains unavailable")
end
testedRenderer=shippedRenderer
check(V:LoadOutfit("Elf combo"),"Full armor and body combo loads")
check(visible(1)==0 and VanityStudioCharacter.body.race==4,"Hidden armor loads and race definition remains saved")
check(V:SaveOutfit("Elf combo",true),"Updating preserves body combination")
check(VanityStudioDB.outfits["Elf combo"].body.race==4,"Update does not lose saved race")
V:SetTab("body")
check(not V.bodyRows.race.button.disabled,"Race dropdown is enabled")
check(V.model:IsVisible() and V.pagesByName.armor:IsVisible() and not V.armorDecorationFrame:IsVisible(),"Race view keeps the live wardrobe model visible without armor decorations")
local function chooseNextBody(key)
    click(V.bodyRows[key].button)
    local index=1
    for i,entry in ipairs(menuEntries) do if entry.checked then index=i end end
    local nextIndex=math.mod(index,table.getn(menuEntries))+1
    click(getglobal("DropDownList1Button"..nextIndex))
    check(not DropDownList1:IsShown(),"Choosing a body value closes its dropdown")
end
click(V.bodyRows.race.button);click(V.bodyRows.sex.button)
check(DropDownList1:IsShown() and table.getn(menuEntries)==2 and menuEntries[1].text=="Male" and menuEntries[2].text=="Female","Switching open body dropdowns shows the new field choices")
V:SetTab("armor")
check(not DropDownList1:IsShown() and V.armorDecorationFrame:IsVisible() and not V.pagesByName.body:IsVisible(),"Returning to Outfit restores decorations and dismisses body controls")
V:SetTab("body")
local currentRace=VanityStudioCharacter.body.race;local count=bodyCalls
chooseNextBody("race")
check(VanityStudioCharacter.body.race~=currentRace and renderedBody.race==VanityStudioCharacter.body.race and not V.editingBody,"Race dropdown applies immediately without an Apply button")
check(bodyCalls==count+1,"Each dropdown selection sends exactly one body change")
check(V.applyBodyButton==nil,"The Race page has no Apply button")
local goodRace=renderedBody.race;bodyFailure=-4
chooseNextBody("race")
check(VanityStudioCharacter.body.race==goodRace and renderedBody.race==goodRace and not V.editingBody,"Rejected instant changes restore the last applied selection")
bodyFailure=nil
local appliedRace=renderedBody.race
V:EditBody("race",7);bodyFailure=-3
check(not V:ApplyBody() and VanityStudioCharacter.body.race==appliedRace and renderedBody.race==appliedRace,"Failed body change preserves current saved and rendered selection")
bodyFailure=nil;V:ApplyBody()
check(renderedBody.race==7,"Corrected body selection can be retried")
V:SetEnabled(false);check(renderedBody==nil,"Pause restores real body")
V:SetEnabled(true);check(renderedBody.race==7,"Resume reapplies full body")
V:ClearBody();check(renderedBody==nil and not VanityStudioCharacter.body,"Use real body clears selection")
V:LoadOutfit("Elf combo");check(renderedBody.race==4,"Outfit restores race combo")
local before=bodyCalls;V.needsSync=false;V.pending={};V:SetTab("armor")
for i=1,20 do now=now+.6;arg1=.6;V.events.scripts.OnUpdate() end
check(bodyCalls==before,"Idle and preview-loading timers never reapply body")
local first=V:OutfitNames()[1];VanityStudioCharacter.activeOutfit=first;V:CycleOutfit(-1)
local names=V:OutfitNames();check(VanityStudioCharacter.activeOutfit==names[table.getn(names)],"Previous wraps")
V:CycleOutfit(1);check(VanityStudioCharacter.activeOutfit==first,"Next wraps")
V:Diagnose();check(string.find(VanityStudioDB.diagnostics,"5875",1,true),"Diagnostics persist without file API")
for race=1,8 do for sex=0,1 do
    local b=V:NormalizeBody({race=race,sex=sex,skin=999,face=-1,hairStyle=999,hairColor=-1,facial=999})
    for _,key in ipairs(V.bodyKeys) do
        local found=false
        for _,value in ipairs(V:BodyValues(b,key)) do if b[key]==value then found=true end end
        check(found,"Saved body values remain valid")
    end
end end
-- List-only outfits and independent previews preserve the saved original.
V:SetTab("armor");V:LoadOutfit("Elf combo")
local originalSaved=V:Copy(VanityStudioDB.outfits["Elf combo"])
V:Select(1,head);V:Select(5,0)
check(VanityStudioCharacter.activeUnsaved and not VanityStudioCharacter.activeOutfit and V:ActiveOutfitText()=="(Unsaved)","Edits activate (Unsaved)")
check(VanityStudioDB.outfits["Elf combo"].slots[1]==originalSaved.slots[1],"Editing never overwrites saved original")
local unsaved=V:Copy(VanityStudioCharacter.unsaved)
check(V:OutfitKeys()[1]==V.UNSAVED,"Single unsaved entry sorts first")
V:LoadOutfit("Elf combo")
check(VanityStudioCharacter.unsaved.slots[1]==unsaved.slots[1],"Activating another outfit retains unsaved work")
V:SetTab("outfits")
check(V.outfitRows[1].outfit==V.UNSAVED and not V.updateOutfitButton and not V.outfitPrev,"List-only outfits has no old controls")
local priorCalls=table.getn(calls);local priorBodyCalls=bodyCalls;local priorActive=VanityStudioCharacter.activeOutfit
click(V.outfitRows[1])
check(V.outfitDetails:IsShown() and GetDoublewideFrame()==V.frame,"Row opens neighboring detail window")
check(VanityStudioCharacter.activeOutfit==priorActive and bodyCalls==priorBodyCalls and table.getn(calls)==priorCalls,"Opening details never activates")
check(V.outfitName.parent.parent==V.outfitDetails,"Naming controls belong to detail window")
local scopeBody,scopeArmed,scopeToken=nil,false,0
function SaureksClosetBeginPreview(race,sex,skin,face,hairStyle,hairColor,facial)
    scopeBody={race=race,sex=sex,skin=skin,face=face,hairStyle=hairStyle,hairColor=hairColor,facial=facial};scopeArmed=true;return 1
end
function SaureksClosetEndPreview() scopeArmed=false;scopeToken=scopeToken+1;return scopeToken end
function SaureksClosetPreviewStatus(token) check(token==scopeToken,"Preview owns its token");return 1 end
local detailSetUnit=methods.SetUnit
function methods:SetUnit(unit)
    if scopeArmed then check(self==V.outfitBuffer,"Only the staging model receives the selected body");self.previewBody=V:Copy(scopeBody) end
    detailSetUnit(self,unit)
end
for i=1,3 do now=now+.5;V:UpdatePreviewLoading() end
check(not scopeArmed and V.outfitModel.alpha==1 and V.outfitModel.previewBody.race==unsaved.body.race,"Preview composes selected body and ends scope")
check(VanityStudioCharacter.activeOutfit==priorActive and table.getn(calls)==priorCalls and bodyCalls==priorBodyCalls,"Preview never calls world setters")
V:SetEnabled(false);click(V.activateOutfitButton)
check(VanityStudioCharacter.activeUnsaved and VanityStudioCharacter.enabled,"Unsaved outfit reactivates")
V.outfitName:SetText("Saved from detail");click(V.saveOutfitButton)
check(VanityStudioDB.outfits["Saved from detail"] and not VanityStudioCharacter.unsaved and not VanityStudioCharacter.activeUnsaved,"Save converts unsaved into permanent outfit")
check(V.detailKey=="Saved from detail" and V.saveOutfitButton:GetText()=="Rename","Saved details offer Rename")
local savedGear=V:Copy(VanityStudioDB.outfits["Saved from detail"].slots)
V.outfitName:SetText("Renamed detail");click(V.saveOutfitButton)
check(not VanityStudioDB.outfits["Saved from detail"] and VanityStudioDB.outfits["Renamed detail"].slots[1]==savedGear[1] and VanityStudioCharacter.activeOutfit=="Renamed detail","Rename preserves data and active identity")
V.outfitName:SetText("Elf combo");click(V.saveOutfitButton)
check(V.detailKey=="Renamed detail" and VanityStudioDB.outfits["Elf combo"].slots[1]==originalSaved.slots[1],"Rename rejects collision")
click(V.deleteOutfitButton)
check(VanityStudioDB.outfits["Renamed detail"] and V.deleteOutfitButton:GetText()=="Confirm","Delete requires confirmation")
click(V.deleteOutfitButton)
check(not VanityStudioDB.outfits["Renamed detail"] and VanityStudioCharacter.activeUnsaved and not V.outfitDetails:IsShown(),"Deleting active saved outfit retains look as unsaved")
V:OpenOutfitDetails(V.UNSAVED)
function methods:SetUnit() error("simulated preview API error") end
now=now+.5;V:UpdateOutfitPreview()
check(not scopeArmed and not V.detailPending,"Preview exception still ends scope")
methods.SetUnit=detailSetUnit
V:CloseOutfitDetails();check(GetDoublewideFrame()==nil,"Closing detail releases neighbor")
VanityStudioCharacter.unsaved=nil;VanityStudioCharacter.activeUnsaved=nil;VanityStudioCharacter.outfitDirty=nil
-- Header dropdown applies complete outfits, pages long lists, and closes with the wardrobe.
V:SetTab("armor");local priorOutfits=V:Copy(VanityStudioDB.outfits)
local dropdownCharacter=V:Copy(VanityStudioCharacter)
VanityStudioDB.outfits={}
click(V.outfitSelector)
check(menuEntries[1].text=="Saved Looks - Quick Selection" and menuEntries[1].isTitle and not menuEntries[1].hasArrow,"Saved Looks is a native title above the same-level choices")
check(menuEntries[2].disabled and menuEntries[3].text=="Customize appearance...","Empty dropdown directs users to saving")
click(getglobal("DropDownList1Button3"))
check(V.tab=="armor" and not DropDownList1:IsShown(),"Empty-state action opens Armor")
for i=1,27 do VanityStudioDB.outfits[string.format("Look %02d",i)]={version=2,slots={[1]=head},body={race=4,sex=1,skin=0,face=0,hairStyle=0,hairColor=0,facial=0}} end
V:SetTab("armor");VanityStudioCharacter.activeOutfit=nil
click(V.outfitSelector)
check(menuEntries[1].text=="Saved Looks - Quick Selection" and menuEntries[1].isTitle and not menuEntries[1].hasArrow,"Saved Looks is a native title above the same-level choices")
check(DropDownList1:IsShown() and table.getn(menuEntries)==15,"Long list shows twelve outfits and paging controls")
check(menuEntries[2].disabled and not menuEntries[3].disabled,"First page only permits forward navigation")
click(getglobal("DropDownList1Button3"))
check(DropDownList1:IsShown() and V.outfitMenuPage==2 and menuEntries[4].text=="Look 13","Native click keeps next page open")
check(not getglobal("DropDownList1Button3Check"):IsShown(),"Paging does not leave a stray checkmark")
click(getglobal("DropDownList1Button3"))
check(table.getn(menuEntries)==6 and menuEntries[3].disabled,"Last page exposes every remaining outfit")
V:SetEnabled(false);click(getglobal("DropDownList1Button4"))
check(VanityStudioCharacter.activeOutfit=="Look 25" and VanityStudioCharacter.enabled and VanityStudioCharacter.body.race==4 and VanityStudioCharacter.selected[1]==head,"Dropdown selection activates the complete armor/body outfit")
check(not DropDownList1:IsShown() and V.activeOutfitLabel:GetText()=="Look 25","Selection closes menu and updates nameplate")
click(V.outfitSelector)
check(menuEntries[1].text=="Saved Looks - Quick Selection" and menuEntries[1].isTitle and not menuEntries[1].hasArrow,"Saved Looks is a native title above the same-level choices")
check(V.outfitMenuPage==3 and menuEntries[4].checked,"Reopening finds and checks the active outfit")
click(V.outfitSelector);check(not DropDownList1:IsShown(),"Second header click closes dropdown")
click(V.outfitSelector);HideUIPanel(V.frame)
check(not DropDownList1:IsShown(),"Hiding the wardrobe closes its dropdown")
VanityStudioDB.outfits=priorOutfits;VanityStudioCharacter=dropdownCharacter;V:Sync();V:Toggle(true)
V:SetTab("body");V.editingBody=nil;V:RefreshBody()
for _,key in ipairs({"sex","skin","face","hairStyle","hairColor","facial"}) do
    local before=bodyCalls;chooseNextBody(key)
    check(bodyCalls==before+1 and not V.editingBody,"Every body row applies immediately once")
    check(renderedBody[key]==VanityStudioCharacter.body[key],"Displayed body choice is committed")
end
-- The arrow controls cycle instantly, wrap, and preserve the center dropdown.
for _,key in ipairs({"race","sex","skin","face","hairStyle","hairColor","facial"}) do
    local row=V.bodyRows[key];local original=VanityStudioCharacter.body[key];local before=bodyCalls
    click(row.next)
    if not row.next.disabled then
        check(bodyCalls==before+1 and renderedBody[key]==VanityStudioCharacter.body[key],"Next arrow applies one body change immediately")
        click(row.previous)
        check(VanityStudioCharacter.body[key]==original,"Previous arrow returns to the starting body value")
    else check(bodyCalls==before,"Single-choice arrows are disabled") end
end
V:SelectBodyValue("race",8);click(V.bodyRows.race.next)
check(VanityStudioCharacter.body.race==1,"Next race wraps to the first race")
click(V.bodyRows.race.previous)
check(VanityStudioCharacter.body.race==8,"Previous race wraps to the last race")
local arrowRace=VanityStudioCharacter.body.race;bodyFailure=-4
click(V.bodyRows.race.next)
check(VanityStudioCharacter.body.race==arrowRace and not V.editingBody,"Failed arrow changes retain the previous body")
bodyFailure=nil
click(V.realBodyButton)
check(VanityStudioCharacter.body==nil and renderedBody==nil,"Reset body still restores the native appearance")
check(V:UsingTrueModel() and V.trueModelCheck:IsShown() and V.trueModelLabel:GetText()=="Use True Model","Real body is an explicit checked control")
local nativeBodyReader=SaureksClosetRealBody
local originalCharacter=V:Copy(VanityStudioCharacter)
local trueBody=V:NormalizeBody({race=2,sex=0})
for _,key in ipairs(V.bodyKeys) do
    local values=V:BodyValues(trueBody,key);trueBody[key]=values[table.getn(values)]
end
SaureksClosetRealBody=function() return trueBody.race,trueBody.sex,trueBody.skin,trueBody.face,trueBody.hairStyle,trueBody.hairColor,trueBody.facial end
local callsBeforeTrueBody=bodyCalls
V.trueBody={race=1,sex=0,skin=0,face=0,hairStyle=0,hairColor=0,facial=0}
V.bodyControlValues=V:Copy(V.trueBody)
V:RefreshBody()
for _,key in ipairs({"race","sex","skin","face","hairStyle","hairColor","facial"}) do
    check(V.bodyControlValues[key]==trueBody[key],"True Model explicitly sets every control value from the current character")
end
check(V:BodyDraft().race==2 and not VanityStudioCharacter.body and bodyCalls==callsBeforeTrueBody,"Opening Body uses the real Orc without enabling customization")
for _,key in ipairs(V.bodyKeys) do check(V:BodyDraft()[key]==trueBody[key],"True Model preserves the player's actual appearance settings") end
V:SelectBodyValue("race",2)
check(V:UsingTrueModel() and bodyCalls==callsBeforeTrueBody,"Selecting the existing real race does not create an override")
click(V.realBodyButton)
check(V:UsingTrueModel() and bodyCalls==callsBeforeTrueBody,"Clicking the checked True Model control is idempotent")
local previousSkin=trueBody.skin;trueBody.skin=255
check(V:NativeBody().skin==255,"Real player data is never silently replaced by catalog defaults")
trueBody.skin=previousSkin
SaureksClosetRealBody=function() return -1 end
click(V.realBodyButton)
V:RefreshBody()
check(V:BodyDraft()==nil and V.bodyRows.race.button.disabled,"Unavailable real data waits instead of inventing a default human")
check(not V:SelectBodyValue("hairStyle",1) and V:UsingTrueModel(),"Editing before real data loads cannot enable a default body")
SaureksClosetRealBody=function() return trueBody.race,trueBody.sex,trueBody.skin,trueBody.face,trueBody.hairStyle,trueBody.hairColor,trueBody.facial end
local bodyTickArg=arg1;arg1=.5;V.events.scripts.OnUpdate();arg1=bodyTickArg
check(not V.trueBodyPending and V.bodyControlValues and V.bodyControlValues.race==2,"A late character-data arrival populates the controls without reopening the page")
V:RefreshBody();click(V.bodyRows.skin.next)
check(not V:UsingTrueModel() and not V.trueModelCheck:IsShown() and VanityStudioCharacter.body.race==2,"First real change clears the check and starts from the actual Orc")
local customizedLook=V:CurrentLook();check(customizedLook.body~=nil,"Customized saved looks retain body settings")
local clearBody=SaureksClosetClearAppearance
SaureksClosetClearAppearance=function() return -4 end
click(V.realBodyButton)
check(VanityStudioCharacter.body~=nil and not V.trueModelCheck:IsShown(),"Failed reset preserves customization and does not falsely check True Model")
SaureksClosetClearAppearance=clearBody
local armorBeforeReset=VanityStudioCharacter.selected;local weaponsBeforeReset=VanityStudioCharacter.weapons
click(V.realBodyButton)
check(V:UsingTrueModel() and V.trueModelCheck:IsShown() and V:BodyDraft().skin==trueBody.skin,"True Model resets controls and restores the check")
for _,key in ipairs({"race","sex","skin","face","hairStyle","hairColor","facial"}) do
    check(V.bodyControlValues[key]==trueBody[key] and V.trueBody[key]==trueBody[key],"Reset repopulates all seven appearance values from the character")
end
-- Reset must re-read even when already checked, not reuse a previous snapshot.
trueBody=V:NormalizeBody({race=4,sex=1,skin=2,face=3,hairStyle=4,hairColor=3,facial=0})
click(V.realBodyButton)
for _,key in ipairs({"race","sex","skin","face","hairStyle","hairColor","facial"}) do
    check(V.bodyControlValues[key]==trueBody[key],"Repeated True Model reset replaces every value with the latest real character data")
end
check(V:CurrentLook().body==nil and VanityStudioCharacter.selected==armorBeforeReset and VanityStudioCharacter.weapons==weaponsBeforeReset,"Real-body looks save without body overrides and reset preserves armor and weapons")
VanityStudioDB.outfits["True model regression"]=V:CurrentLook()
VanityStudioDB.outfits["Custom model regression"]=customizedLook
check(V:LoadOutfit("Custom model regression") and not V.trueModelCheck:IsShown(),"Loading a custom-body look restores the unchecked state")
check(V:LoadOutfit("True model regression") and V.trueModelCheck:IsShown(),"Loading a real-body look restores the checked state")
VanityStudioDB.outfits["True model regression"]=nil;VanityStudioDB.outfits["Custom model regression"]=nil
VanityStudioCharacter=originalCharacter;SaureksClosetRealBody=nativeBodyReader;V:Sync();V:Refresh()

V:SetTab("armor")
-- Simulate an asynchronous player-model clone that initially contains default robes.
local setUnit=methods.SetUnit;local undress=methods.Undress
local clones=0;local livePreviewBody="custom"
function methods:SetUnit(unit)
    check(self==V.previewBuffer and self.alpha==0,"Model copies are prepared invisibly in the staging model")
    setUnit(self,unit);clones=clones+1
    self.pendingPreviewModel={at=now+.15,body=livePreviewBody}
end
function methods:Undress()
    check(not self.pendingPreviewModel,"Dressing waits for the asynchronous clone")
    undress(self)
end
local function previewTick()
    now=now+.5
    for _,model in ipairs({V.model,V.previewBuffer}) do
        local pending=model.pendingPreviewModel
        if pending and now>=pending.at then
            model.previewBody=pending.body;model.tried={"default robe"};model.pendingPreviewModel=nil
        end
    end
    V:UpdatePreviewLoading()
    check((V.model.alpha==0 and (V.previewBaseDirty or V.previewDressAt)) or (V.model.alpha~=0 and not V.model.pendingPreviewModel),"Opening stays hidden while loading; a visible model never exposes an unfinished clone")
end
V:Select(1,head);V:Select(5,0);V:SetEnabled(true)
for i=1,8 do previewTick() end
check(V.model.previewBody=="custom" and not V.previewBaseDirty and not V.previewDressAt,"Preview finishes cloning and dressing")
local stableClones=clones
V:DraftSlot(1,other)
check(clones==stableClones,"Trying another item reuses the finished preview model")
check(V.previewNote:GetText()~="Preview only","Draft does not put redundant text over the model")
V:CancelDraft();V:SetEnabled(false)
check(V:PreviewItems()[1]==real[1] and V:PreviewItems()[5]==real[5],"Disabled preview follows real armor, including formerly hidden slots")
previewTick();livePreviewBody="real" -- live model finishes after the first copy
for i=1,8 do previewTick() end
check(V.model.previewBody=="real","Bounded recovery replaces a clone captured during a body transition")
check(not V.draft and not V.previewDressAt and not V.model.pendingPreviewModel,"Toggle leaves no stale preview draft or unfinished clone")
for _,item in ipairs(V.model.tried) do check(item~="default robe","Default robes are replaced after asynchronous loading") end
local quietClones=clones;local quietCalls=bodyCalls
local oldSignature=V.previewSignature
V:Sync();V:Refresh()
check(V.previewSignature==oldSignature and not V.previewBaseDirty,"Unchanged world synchronization does not reload or redress the preview")
local oldArg=arg1;event="UNIT_INVENTORY_CHANGED";arg1="player";V.events.scripts.OnEvent();arg1=oldArg
check(not V.previewBaseDirty,"Equipment notifications do not recreate the character model")
for i=1,10 do previewTick() end
check(clones==quietClones and bodyCalls==quietCalls,"Preview recovery stops and never touches world appearance")
local portraitTexture=V.frame.portrait.texture
local savedArg=arg1;event="UNIT_PORTRAIT_UPDATE";arg1="player";V.events.scripts.OnEvent();arg1=savedArg
check(V.frame.portrait.texture==portraitTexture and string.find(portraitTexture,"Logo.tga",1,true),"Portrait updates preserve the direct logo texture")
local savedArg=arg1;event="UNIT_MODEL_CHANGED";arg1="target";V.events.scripts.OnEvent();arg1=savedArg
check(not V.previewBaseDirty,"Other units cannot reset the player's preview")
local savedArg=arg1;event="UNIT_MODEL_CHANGED";arg1="player";V.events.scripts.OnEvent();arg1=savedArg
check(V.previewBaseDirty,"Player model events schedule a fresh clone")
for i=1,8 do previewTick() end
-- A compositor-aware bridge must not replace a finished model on a timer.
SaureksClosetInspectPreview=function() return 1 end
V:InvalidatePreviewModel(0,true)
local trackedClones=clones
for i=1,10 do previewTick() end
check(clones==trackedClones+1 and not V.previewRecoveries,"Tracked preview composes once without a delayed replacement")
SaureksClosetInspectPreview=nil
methods.SetUnit=setUnit;methods.Undress=undress
V:OpenBrowser(1)
check(V.searchHint:IsShown() and V.query=="" and V.searchHint:GetText()=="Search Item DB...","Correct placeholder is visible but never becomes the search query")
check(V.searchHint.fontSize==V.search.fontSize and V.searchHint.justifyV=="MIDDLE" and V.searchHint.height==V.search.height,"Search placeholder matches input size and is vertically centered")
V.search:SetText("helm")
check(not V.searchHint:IsShown() and V.query=="helm","Typing hides the placeholder")
V.search:SetText("")
local foundUncachedIcon=false
for _,row in ipairs(V.rows) do
    if row.item and not GetItemInfo(row.item[1]) and V:CatalogIcon(row.item[1]) then
        check(row.icon.texture==V:CatalogIcon(row.item[1]),"Uncached items show their actual native catalog icon")
        foundUncachedIcon=true
    end
end
check(foundUncachedIcon,"Icon test exercises items absent from the server cache")
check(string.find(V.frame.portrait.texture,"Textures",1,true)~=nil,"Window uses the supplied logo, not a character portrait")
V:CloseBrowser()
-- Migration preserves legacy dirty selections as one persisted unsaved entry.
local migrationCharacter=V:Copy(VanityStudioCharacter)
VanityStudioCharacter.activeOutfit="Elf combo";VanityStudioCharacter.activeUnsaved=nil
VanityStudioCharacter.unsaved=nil;VanityStudioCharacter.outfitDirty=true
local migrationSlots=V:Copy(VanityStudioCharacter.selected)
V:Initialize();V:Initialize()
check(VanityStudioCharacter.activeUnsaved and not VanityStudioCharacter.activeOutfit and VanityStudioCharacter.unsaved.baseName=="Elf combo","Legacy dirty state migrates to the singleton unsaved outfit")
for slot,id in pairs(migrationSlots) do check(VanityStudioCharacter.unsaved.slots[slot]==id,"Migration retains unsaved armor") end
local unsavedCount=0;for _,key in ipairs(V:OutfitKeys()) do if key==V.UNSAVED then unsavedCount=unsavedCount+1 end end
check(unsavedCount==1,"Reloading never duplicates (Unsaved)")
check(not V:SaveOutfit("(Unsaved)"),"Special entry name cannot become a duplicate saved outfit")
VanityStudioCharacter=migrationCharacter
-- Static race/gender icons must work without a renderer or any model activity.
local savedDB=V:Copy(VanityStudioDB);local savedChar=V:Copy(VanityStudioCharacter)
local body=assert(V:NativeBody())
local oldVersion=SaureksClosetRendererVersion
SaureksClosetRendererVersion=nil
local races={"Human","Orc","Dwarf","NightElf","Scourge","Tauren","Gnome","Troll"}
for race,name in ipairs(races) do
    for sex=0,1 do
        local texture=V:OutfitPortraitTexture({body={race=race,sex=sex}})
        check(texture=="Interface\\CharacterFrame\\TemporaryPortrait-"..(sex==0 and "Male" or "Female").."-"..name,"Saved race and gender select their exact stock portrait")
    end
end
local realRace,realSex=UnitRace,UnitSex
function UnitRace() return "Localized undead", "Scourge" end
function UnitSex() return 3 end
check(V:OutfitPortraitTexture({})=="Interface\\CharacterFrame\\TemporaryPortrait-Female-Scourge","Real-body icon uses native English race and unit sex without DLL")
function UnitRace() return "Localized human", "Human" end
function UnitSex() return 2 end
check(V:OutfitPortraitTexture({})=="Interface\\CharacterFrame\\TemporaryPortrait-Male-Human","Male unit sex maps to the male portrait")
check(V:OutfitPortraitTexture({body={race=99,sex=0}})=="Interface\\CharacterFrame\\TemporaryPortrait","Unknown race uses the stock fallback")
UnitRace=realRace;UnitSex=realSex
VanityStudioDB.outfits={}
for i=1,14 do VanityStudioDB.outfits[string.format("Portrait %02d",i)]={slots={[1]=100+i,[7]=200+i},body={race=math.mod(i-1,8)+1,sex=math.mod(i,2)}} end
VanityStudioCharacter.activeOutfit="Portrait 01";VanityStudioCharacter.activeUnsaved=nil;VanityStudioCharacter.unsaved=nil
local oldSetUnit=methods.SetUnit
function methods:SetUnit() error("Static list must not load any model") end
V:SetTab("outfits");V:SetOutfitOffset(0)
check(V.visibleOutfitRows==7 and V.outfitScroll.template=="UIPanelScrollBarTemplate","Portrait list retains standard scrollbar")
check(V.outfitScroll.min==0 and V.outfitScroll.max==7 and V.outfitScrollUp.disabled,"Scrollbar range reflects visible rows")
for i,row in ipairs(V.outfitRows) do
    check(not row.portrait and not row.portraitPlaceholder,"No 3D thumbnail or temporary logo is created")
    check(row.portraitImage:IsShown() and row.portraitImage.texture==V:OutfitPortraitTexture(V:GetOutfit(row.outfit)),"Visible row immediately displays correct static icon")
end
check(V.outfitRows[1].active:IsShown(),"Active outfit retains its subtle highlight")
local firstIcon=V.outfitRows[1].portraitImage.texture
V:OpenOutfitDetails("Portrait 02");V:CloseOutfitDetails()
check(V.outfitRows[1].portraitImage.texture==firstIcon,"Opening large viewer cannot replace static icons")
click(V.outfitScrollDown);check(V.outfitOffset==1,"Scrollbar arrow advances list")
V.outfitScroll:SetValue(7);check(V.outfitOffset==7 and V.outfitScrollDown.disabled,"Scrollbar reaches last outfit")
check(V.outfitRows[1].portraitImage.texture==V:OutfitPortraitTexture(VanityStudioDB.outfits["Portrait 08"]),"Reused row immediately selects correct icon")
V:SetOutfitOffset(0);check(V.outfitRows[1].portraitImage.texture==firstIcon,"Scrolling back restores original portrait")
VanityStudioDB.outfits["Portrait 01"].body={race=8,sex=0};V:RefreshOutfits()
check(V.outfitRows[1].portraitImage.texture=="Interface\\CharacterFrame\\TemporaryPortrait-Male-Troll","Changing an outfit body updates the icon")
V:SetTab("armor")
methods.SetUnit=oldSetUnit;SaureksClosetRendererVersion=oldVersion
-- Overwrite is explicit and copies both armor and body from the singleton unsaved look.
VanityStudioCharacter.selected={[1]=777};VanityStudioCharacter.body=V:Copy(body);V:TrackUnsaved()
V:SetTab("outfits");V:OpenOutfitDetails(V.UNSAVED)
check(V.replaceOutfitButton:IsShown() and not V.detailName,"Unsaved details offer replacement and omit the repeated title")
click(V.replaceOutfitButton)
local replacement
for _,entry in ipairs(menuEntries) do if entry.text=="Portrait 02" then replacement=entry end end
check(replacement~=nil,"Replacement dropdown lists existing outfits")
replacement.func(replacement.arg1)
check(VanityStudioDB.outfits["Portrait 02"].slots[1]==777 and VanityStudioDB.outfits["Portrait 02"].body.race==body.race,"Replacement saves unsaved armor and body together")
check(not VanityStudioCharacter.unsaved and VanityStudioCharacter.activeOutfit=="Portrait 02" and V.detailKey=="Portrait 02","Replacing active unsaved changes keeps the saved result active and removes the singleton")
check(not V.replaceOutfitButton:IsShown(),"Saved details do not offer the unsaved replacement action")
check(VanityStudioDB.outfits["Portrait 01"].slots[1]==101,"Other saved outfits remain intact")
V:SetTab("armor");VanityStudioDB=savedDB;VanityStudioCharacter=savedChar


-- Physical placements persist independently and never use fake inventory slots.
local weaponCalls={};local weaponWorld={}
function SaureksClosetSetWeapons(token,...)
    check(arg.n==10,"Weapon API has seven positions and three real equipment IDs")
    local record={token=token,values=arg};table.insert(weaponCalls,record)
    if token==0 then weaponWorld=V:Copy(arg) end
    return 1
end
function SaureksClosetPreviewStatus(token) return token>0 and 1 or -1 end
function methods:SetUnit(unit)
    if scopeArmed then
        check(self==V.outfitBuffer or self==V.previewBuffer,"Only a staging preview receives an independent model")
        self.previewBody=V:Copy(scopeBody)
    end
    detailSetUnit(self,unit)
end
V:ClearAll();V:SetEnabled(true);V:SetTab("weaponry")
local weaponIDs={}
for _,slot in ipairs(V.weaponOrder) do
    check(table.getn(V.slots[slot])>0,"Each position has verified model assets")
    weaponIDs[slot]=V.slots[slot][1][1]
    for _,item in ipairs(V.slots[slot]) do check(V:WeaponCompatible(item[1],slot),"Position filtering excludes incompatible model types") end
end
real[16]=35;real[17]=nil;real[18]=nil
weaponIDs[103]=35
for _,slot in ipairs(V.weaponOrder) do check(V:Select(slot,weaponIDs[slot]),"Each of seven positions is selectable") end
check(VanityStudioCharacter.activeUnsaved,"Placement changes create the unsaved look")
for _,slot in ipairs(V.weaponOrder) do check(VanityStudioCharacter.unsaved.weapons[slot]==weaponIDs[slot],"All placements enter the saved-look transaction") end
check(weaponWorld[8]==35 and weaponWorld[9]==0 and weaponWorld[10]==0,"Drawing follows actual equipment, including empty ranged slot")
check(V:SaveOutfit("Seven placements"),"Save simultaneous weapon placements")
V:ClearSlot(103)
check(not VanityStudioCharacter.weapons[103] and VanityStudioDB.outfits["Seven placements"].weapons[103]==35,"Clearing a placement preserves its saved original")
check(V:LoadOutfit("Seven placements") and VanityStudioCharacter.weapons[103]==35,"Activating a saved look restores all placements")
local beforeWorld=V:Copy(weaponWorld);local callsBefore=table.getn(calls)
V:OpenBrowser(103);V:DraftSlot(103,nil)
now=now+3;V:RefreshPreview();now=now+.5;V:RefreshPreview()
check(weaponWorld[3]==beforeWorld[3] and table.getn(calls)==callsBefore,"Browsing and draft previews never change the world")
check(V.model.weaponToken and V.model.weaponToken>0,"Wardrobe preview has an independently owned model token")
check(not scopeArmed,"Wardrobe preview always closes the native copy scope")
local last=weaponCalls[table.getn(weaponCalls)]
check(last.token>0 and last.values[3]==35,"Clearing the draft appearance previews the equipped staff independently")
V:CloseBrowser();V:SetEnabled(false)
for i=1,7 do check(weaponWorld[i]==0,"Toggle removes all attached cosmetic weapons") end
V:SetEnabled(true)
for i,slot in ipairs(V.weaponOrder) do check(weaponWorld[i]==weaponIDs[slot],"Toggle restores all saved attachments") end
V:ClearAll()
for i=1,7 do check(weaponWorld[i]==(i==3 and 35 or 0),"Reset clears customs and restores the equipped staff") end
-- Ranged appearance alone stays on the back; no fake equipped weapon is reported.
V:Select(106,weaponIDs[106]);check(V:PreviewWeaponRoutes(VanityStudioCharacter.weapons)[18]==nil,"Cosmetic ranged weapon is not drawn without real ranged equipment")
V:ClearAll();V:Select(16,35)
check(V:Select(101,weaponIDs[101]),"A legacy weapon look can be edited with physical positions")
check(not VanityStudioCharacter.selected[16] and VanityStudioCharacter.weapons[103]==35,"Legacy staff is moved to a back position before clearing its old inventory override")
for _,call in ipairs(calls) do check(call.slot<100,"Physical positions never reach SetUnitVisibleItemID") end
V:ClearAll();real[16]=nil;real[18]=nil

-- Reported client failure: real sword 4939 and bow 2507 disappear when stored.
do
    V:ClearAll();real[16]=4939;real[18]=2507
    V:SyncWeapons()
    check(weaponWorld[3]==4939 and weaponWorld[6]==2507,"Real sword and bow receive independent back homes without any custom selections")
    check(not next(VanityStudioCharacter.weapons),"Equipped fallbacks never become saved transmogs")
    local defaults=V:PreviewWeapons();local routes=V:PreviewWeaponRoutes(defaults)
    check(defaults[103]==4939 and defaults[106]==2507 and routes[16]==103 and routes[18]==106,"Wardrobe routes real items once instead of also using native TryOn placements")
    local quiver=V.slots[107][1][1]
    V:Select(107,quiver)
    check(weaponWorld[3]==4939 and weaponWorld[6]==2507 and weaponWorld[7]==quiver,"Forced quiver preserves both real stored weapons")
    V:Select(106,2506)
    check(weaponWorld[6]==2506,"Explicit compatible bow appearance replaces the real bow")
    V:ClearSlot(106)
    check(weaponWorld[6]==2507 and not VanityStudioCharacter.weapons[106],"Clearing bow transmog restores actual bow without saving it")
    local equippedBefore=real[18];real[18]=2506;V:SyncWeapons()
    check(weaponWorld[6]==2506,"Changing real equipment updates fallback without an appearance edit")
    real[18]=equippedBefore
    V:SetEnabled(false)
    for i=1,7 do check(weaponWorld[i]==0,"Disabling releases custom and fallback routes") end
    V:SetEnabled(true)
    check(weaponWorld[3]==4939 and weaponWorld[6]==2507,"Re-enabling repairs actual weapon storage")
    check(not V:EffectiveWeapons({}, {[16]=0,[18]=0})[106] and not V:EffectiveWeapons({}, {[16]=0,[18]=0})[103],"Explicit legacy hides suppress equipped fallback")
    local savedLook={slots={},weapons={}}
    local outfitWeapons=V:EffectiveWeapons(savedLook.weapons,savedLook.slots)
    check(outfitWeapons[103]==4939 and outfitWeapons[106]==2507 and not next(savedLook.weapons),"Uncustomized outfit previews use real weapons without modifying the outfit")
    real[16]=25;real[17]=25;real[18]=nil
    local dual=V:EffectiveWeapons({})
    check(dual[101]==25 and dual[102]==25,"Identical dual-wield items each retain a separate storage home")
    real[17]=143
    local shield=V:EffectiveWeapons({})
    check(shield[102]==25 and shield[105]==143,"Equipped shield and one-handed weapon use separate positions")
    real[16]=999999;real[17]=nil
    check(not next(V:EffectiveWeapons({})),"Unknown real items are left to the client without guessed models")
    real[16]=nil;real[18]=nil;V:ClearAll()
end

-- Preview scheduling: one copy, native composition gates, no event/timer snaps.
do
    V:CloseOutfitDetails();V:CloseBrowser();V:SetTab("armor")
    local originalSetUnit=methods.SetUnit;local originalUndress=methods.Undress
    local originalStatus=SaureksClosetPreviewStatus;local originalInspect=SaureksClosetInspectPreview
    local copies,dresses=0,0;local ready=false;local dirty=0
    function methods:SetUnit(unit) copies=copies+1;originalSetUnit(self,unit) end
    function methods:Undress() dresses=dresses+1;originalUndress(self) end
    function SaureksClosetPreviewStatus(token) return ready and 1 or 0 end
    function SaureksClosetInspectPreview(token) return ready and 1 or 0,1,1,0,0,0,0,0,0,dirty end
    local function frame()
        now=now+.016
        local old=arg1;arg1=.016;V.events.scripts.OnUpdate();arg1=old
    end
    V:HidePreviewUntilReady();V:InvalidatePreviewModel(.25,true);V:RefreshPreview()
    check(copies==1,"Tracked preview starts immediately without the quarter-second delay")
    local before=dresses
    for i=1,4 do frame();V:RefreshPreviewForModelEvent() end
    check(copies==1 and dresses==before and V.model.alpha==0,"Duplicate world notifications cannot restart an unfinished independent preview")
    ready=true;frame()
    check(dresses==before+1 and V.previewReveal and V.model.alpha==0,"Preview advances within a screen frame, then waits after dressing before reveal")
    dirty=1;frame();check(V.model.alpha==0,"A dirty outfit compositor is not exposed")
    dirty=0;frame()
    check(V.model.alpha==1 and not V.previewDressAt and not V.previewReveal,"Finished preview is revealed exactly once")
    local settled=V.model;local count=copies;before=dresses
    for i=1,10 do V:RefreshPreviewForModelEvent();frame() end
    V:SetTab("body");V:SetTab("weaponry");V:SetTab("armor");frame()
    check(copies==count and dresses==before and V.model==settled,"Same-body notifications and wardrobe page changes preserve the finished model")
    V:DraftSlot(1,head);V:DraftSlot(1,other);V:CancelDraft();V:RefreshPreview()
    for i=1,3 do frame() end
    check(copies==count and not V.previewReveal and V.previewSignature,"Rapid item changes and cancellation settle without another model copy")
    -- A real appearance change still requests one fresh model.
    local reader=SaureksClosetRealBody
    SaureksClosetRealBody=function() return 2,0,1,2,3,4,5 end
    V:RefreshPreviewForModelEvent();for i=1,3 do frame() end
    check(copies==count+1,"Changed real appearance invalidates the preview once")
    SaureksClosetRealBody=reader
    V:RefreshPreviewForModelEvent();for i=1,3 do frame() end
    -- Stock panel resizing must not invoke native model OnHide at all.
    local hide=V.frame.Hide;local hides=0
    V.frame.Hide=function(self) hides=hides+1;hide(self) end
    count=copies
    V:OpenBrowser(1);V:CloseBrowser();for i=1,3 do frame() end
    check(hides==0 and copies==count,"Opening/closing the item browser keeps the wardrobe and its model alive")
    -- Saved-outfit loads use the same readiness gate and coalesce identical work.
    V:OpenOutfitDetails("Seven placements")
    ready=false;count=copies;frame()
    for i=1,4 do V:StartOutfitPreview();frame() end
    check(copies==count+1 and V.outfitModel.alpha==0,"Saved outfit creates one hidden copy despite repeated loading requests")
    ready=true;frame();dirty=1;frame()
    check(V.detailPending and V.detailPending.phase=="reveal" and V.outfitModel.alpha==0,"Saved outfit remains hidden until its dressed textures finish")
    dirty=0;frame()
    check(not V.detailPending and V.outfitModel.alpha==1,"Saved outfit reveals a finished model")
    count=copies;before=dresses
    for i=1,10 do V:StartOutfitPreview();frame() end
    check(copies==count and dresses==before,"Repeated saved-outfit refresh requests leave a completed preview alone")
    local info=GetItemInfo
    GetItemInfo=function(id) if id==head then return nil end return info(id) end
    V.detailMissing={[head]=true};V.detailItemRetries=0;V.detailRetryAt=now
    frame();check(dresses==before,"Missing saved-outfit items retry without undressing the whole model")
    V:CloseOutfitDetails()
    check(hides==0,"Opening/closing saved previews never hides the wardrobe for layout")
    V.frame.Hide=hide
    V.previewWaiting={[head]=true};V.previewRequests[head]={last=now-3,attempts=1}
    before=dresses;frame()
    check(dresses==before,"Missing wardrobe items retry without undressing the whole model")
    GetItemInfo=info
    frame()
    methods.SetUnit=originalSetUnit;methods.Undress=originalUndress
    SaureksClosetPreviewStatus=originalStatus;SaureksClosetInspectPreview=originalInspect
end

local seen={}
for _,item in ipairs(VanityStudioCatalog) do
    check(not seen[item[1]],"Unique item IDs");seen[item[1]]=true
    check(V.inventorySlots[item[3]] and item[8]>0,"Visible item record")
end
-- Unknown templates / game-play callbacks must never be used by a vanity menu.
for _,f in ipairs(frames) do
    check(f.template~="PaperDollItemSlotButtonTemplate","No real equipment callbacks")
    check(not f.scripts.OnKeyDown and not f.scripts.OnKeyUp,"No keyboard interception")
end
print("PASS: "..passed.." assertions; Lua ".._VERSION..". Mocked rendering, actual extracted Blizzard panel functions. Not an in-game test.")
