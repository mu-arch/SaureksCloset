-- Exercise the real Settings page with 1.12-shaped frames. Layout checks cover
-- frame rectangles; fonts and QR scanability are verified separately with assets.
math.mod=math.mod or math.fmod
table.getn=table.getn or function(t) return #t end
UISpecialFrames={}
VanityStudioDB={autoCheckUpdates=false}
local messages,opened,checks={},{},0
VanityStudio={VERSION="4.0.11",Message=function(self,text) table.insert(messages,text) end}
local methods={}
local function node(kind,name,parent)
    local n=setmetatable({kind=kind,name=name,parent=parent,children={},scripts={},shown=true}, {__index=methods})
    if parent then table.insert(parent.children,n) end
    if name then _G[name]=n end
    return n
end
function CreateFrame(kind,name,parent) return node(kind,name,parent) end
function methods:CreateTexture(name) return node("Texture",name,self) end
function methods:CreateFontString(name) return node("FontString",name,self) end
function methods:SetPoint(...) self.anchor={...} end
function methods:ClearAllPoints() self.anchor=nil;self.allPoints=nil end
function methods:SetAllPoints(parent) self.allPoints=parent or self.parent end
function methods:SetWidth(value) self.width=value end
function methods:SetHeight(value) self.height=value end
function methods:GetName() return self.name end
function methods:GetFrameLevel() return self.level or 1 end
function methods:SetFrameLevel(value) self.level=value end
function methods:SetScript(event,callback) self.scripts[event]=callback end
function methods:GetScript(event) return self.scripts[event] end
function methods:SetText(value) self.text=value end
function methods:GetText() return self.text or "" end
function methods:SetTexture(path) self.texture=path end
function methods:SetFont(path,size) self.fontSize=size end
function methods:SetSpacing(value) self.spacing=value end
function methods:SetChecked(value) self.checked=value end
function methods:Enable() self.enabled=true end
function methods:Disable() self.enabled=false end
function methods:Show() self.shown=true end
function methods:Hide() self.shown=false end
function methods:IsShown() return self.shown end
function methods:IsVisible() return self.shown and (not self.parent or self.parent:IsVisible()) end
function methods:SetFocus() self.focused=true end
function methods:ClearFocus() self.focused=false end
function methods:HighlightText() self.highlighted=true end
for _,key in ipairs({"SetJustifyH","SetJustifyV","SetBackdrop","SetBackdropBorderColor","SetBackdropColor",
    "SetVertexColor","SetTexCoord","SetAlpha","SetTextColor","SetHighlightTexture","SetHitRectInsets",
    "EnableMouse","SetFrameStrata","SetClampedToScreen","SetMovable","RegisterForDrag","SetAutoFocus","SetMaxLetters"}) do
    methods[key]=function() end
end
UIParent=node("Frame","UIParent");UIParent.width=1920;UIParent.height=1080
GameTooltip={Hide=function() end,Show=function() end,SetOwner=function() end,
    SetText=function(self,text) self.title=text;self.lines={} end,
    AddLine=function(self,text) table.insert(self.lines,text) end}
SaureksClosetOpenWebsite=function(page) table.insert(opened,page);return 1 end
dofile("addon/SaureksCloset/Updates.lua")
dofile("addon/SaureksCloset/UI.lua")
local V=VanityStudio
V.frame=CreateFrame("Frame","SettingsTestFrame",UIParent);V.frame.width=384;V.frame.height=512
V.frame.title=V.frame:CreateFontString()
local settings=CreateFrame("Frame",nil,V.frame);settings:SetAllPoints(V.frame)
local function check(condition,message) assert(condition,message);checks=checks+1 end
local function event(widget,name)
    local previous=this;this=widget;assert(widget.scripts[name],name.." must be registered")();this=previous
end
local function click(widget)
    check(widget:IsVisible(),"Clicked control must be visible")
    event(widget,"OnClick")
end
local function pointOffset(point,w,h)
    local x=string.find(point,"LEFT",1,true) and 0 or (string.find(point,"RIGHT",1,true) and w or w/2)
    local y=string.find(point,"TOP",1,true) and 0 or (string.find(point,"BOTTOM",1,true) and h or h/2)
    return x,y
end
local function rect(n)
    if n==UIParent then return 0,0,1920,1080 end
    if n.allPoints then return rect(n.allPoints) end
    local a=n.anchor
    local x,y,w,h=rect(a and a[2] or n.parent)
    local nw,nh=n.width or w,n.height or h
    if a then
        local rx,ry=pointOffset(a[3],w,h);local ax,ay=pointOffset(a[1],nw,nh)
        x=x+rx-ax+(a[4] or 0);y=y+ry-ay-(a[5] or 0)
    end
    return x,y,nw,nh
end
local function inside(widget,container,left,top,right,bottom,description)
    local x,y,w,h=rect(widget);local px,py,pw,ph=rect(container)
    check(x>=px+left and x+w<=px+pw-right,description.." exceeds usable width")
    check(y>=py+top and y+h<=py+ph-bottom,description.." exceeds usable height")
end
local function separate(widgets,description)
    for i,a in ipairs(widgets) do
        local x,y,w,h=rect(a)
        for j=1,i-1 do
            local bx,by,bw,bh=rect(widgets[j])
            check(not (x<bx+bw and x+w>bx and y<by+bh and y+h>by),description.." overlap")
        end
    end
end
V:CreateSettingsPage(settings)
check(table.getn(opened)==0,"Constructing Settings must never launch a browser")
check(not V.settingsInfoWindow:IsShown(),"Information window starts closed")
local expectedMessage="World of Warcraft is more than a game to me. It’s a lifelong summation of memories, adventures, friendships, and countless little moments that somehow stick with you. I made this add-on as a love letter to a game I adore so much, and I hope it adds something special to your own adventures.\n\nIf you would like to support that work, a donation is very helpful. On top of that, sharing your custom looks on Reddit, Twitter, Youtube, etc and crediting the addon really helps me.\n\nIf there’s something you’d like to see added or improved, please let me know in my Discord or the Github!"
check(V.donationTitle and V.donationTitle.kind=="FontString" and V.donationTitle.text=="Thanks for using my addon!","The greeting is a separate title")
check(V.donationMessage.text==expectedMessage,"The full replacement letter, punctuation, paragraph breaks and signature are preserved")
check(V.donationTitle.fontSize==12 and V.donationTitle.fontSize>V.donationMessage.fontSize,"The greeting is larger than the letter body")
for _,text in ipairs({V.donationTitle.text,V.donationMessage.text}) do
    check(not string.find(text,"<[^>]+>"),"Donation text must not render HTML tags")
end
check(not V.settingsTagline and not V.settingsTitlePanel,"The tagline and repeated title do not consume Settings space")
check(V.donationMessage.fontSize==9 and V.donationMessage.height>0,"The letter has a 9-point body font and reserved height")
local function fixedPage(frame)
    check(frame.kind~="ScrollFrame","Settings must fit on one page without a scroll frame")
    check(not frame.scripts.OnMouseWheel,"Settings content must not require mouse-wheel scrolling")
    for _,child in ipairs(frame.children) do fixedPage(child) end
end
fixedPage(settings)
check(not V.donateKofiButton and not V.donateCashAppButton,"Settings has one combined donation action")
check(V.donationLinksButton.caption.text=="Kofi & Cashapp Links","The combined button names both destinations")
check(V.donationClosing.text=="Yours,\nSaurek","The closing accompanies the supplied signature")
check(V.donationSignature.texture=="Interface\\AddOns\\SaureksCloset\\Textures\\DonationSignature.tga","The letter uses the supplied signature artwork")
check(V.donationSignature.width==3*V.donationSignature.height,"The signature retains its original aspect ratio")
local settingsControls={V.donationPanel}
for _,button in pairs(V.settingsNavigationButtons) do
    check(button.width==136 and button.caption.fontSize==10,"Navigation buttons share the half-width size and readable font")
    table.insert(settingsControls,button)
end
check(V.donationLinksButton.caption.fontSize==V.settingsNavigationButtons.updates.caption.fontSize,"The combined action matches the navigation font")
local buttonX,buttonY,buttonWidth=rect(V.donationLinksButton)
local signatureX,signatureY,signatureWidth,signatureHeight=rect(V.donationSignature)
local _,messageY,_,messageHeight=rect(V.donationMessage)
check(buttonY>=messageY+messageHeight and signatureY>=messageY+messageHeight,"The action and signature sit below the complete letter")
check(buttonWidth==272 and V.donationLinksButton.caption.width==236,"The donation action uses the full card content width with room for its caption and arrow")
local panelX,panelY,panelWidth,panelHeight=rect(V.donationPanel)
local closingX,closingY=rect(V.donationClosing)
check(closingX==panelX+14,"The normal text sign-off is left-aligned with the letter")
check(signatureX+signatureWidth==panelX+panelWidth-14 and signatureY<closingY and signatureY+signatureHeight<=buttonY,"The artwork sits higher at the right, above the action row")
local _,navigationTop=rect(V.settingsNavigationButtons.privacy)
local _,donationTop=rect(V.donationPanel)
check(navigationTop<=82 and donationTop<=145,"Settings navigation and donation content move upward")
for _,control in ipairs(settingsControls) do inside(control,settings,23,80,43,84,"Settings control") end
separate(settingsControls,"Settings controls")
for _,control in ipairs(V.donationPanel.children) do inside(control,V.donationPanel,8,8,8,6,"Donation card control") end
separate(V.donationPanel.children,"Donation card controls")
check(not V.donationQRButton and not V.donationQRThumbnail,"Settings has no QR thumbnail or QR control")
check(not V.donationQRImage:IsVisible(),"The QR code starts hidden in the donation window")
settings:Hide();settings:Show()
check(table.getn(opened)==0,"Showing Settings must never launch a browser")
event(V.donationLinksButton,"OnEnter")
check(string.find(GameTooltip.title.." "..table.concat(GameTooltip.lines," "),"QR",1,true),"Combined donation button tooltip explains the QR code")
event(V.donationLinksButton,"OnLeave")
click(V.donationLinksButton)
check(V.settingsInfoWindow:IsVisible() and V.infoPages.donations:IsVisible(),"Combined donation button opens the donation window")
check(V.donationQRImage:IsVisible() and V.donationQRImage.width==256 and V.donationQRImage.height==256 and V.donationQRImage.texture=="Interface\\AddOns\\SaureksCloset\\Textures\\CashAppQR.tga","Donation window shows the supplied 256-pixel QR artwork")
check(V.donationCopyAddress.text=="https://cash.app/$saurek","QR window exposes the Cash App destination")
check(table.getn(opened)==0,"Combined donation button must show the QR code without launching a browser")
for _,control in ipairs(V.infoPages.donations.children) do inside(control,V.infoPages.donations,23,70,43,84,"Donation window control") end
separate(V.infoPages.donations.children,"Donation window controls")
for _,name in ipairs({"links","updates","privacy"}) do
    click(V.settingsNavigationButtons[name])
    for pageName,p in pairs(V.infoPages) do check(p:IsVisible()==(pageName==name),"Navigation shows only its chosen information page") end
end
check(table.getn(opened)==0,"Navigation must never launch a browser")
click(V.donationLinksButton)
for name,p in pairs(V.infoPages) do check(p:IsVisible()==(name=="donations"),"Combined donation button hides all other information pages") end
local buttons={{V.donationWindowKofiButton,4},{V.donationWindowCashAppButton,5}}
for _,entry in ipairs(buttons) do
    local button,page=entry[1],entry[2]
    event(button,"OnEnter")
    check(GameTooltip.title==V.websiteURLs[page],"Donation tooltip shows its exact destination")
    event(button,"OnLeave")
    local before=table.getn(opened)
    click(button)
    check(table.getn(opened)==before+1 and opened[before+1]==page,"Each explicit click opens its own allowlisted destination once")
    check(V.donationCopyAddress.text==V.websiteURLs[page],"Copy address follows the selected donation service")
end
local calls=table.getn(opened)
for _,page in ipairs({0,1,2,3,6,4.5,"https://example.com","4"}) do
    check(V:OpenDonationLink(page)==false,"Donation handler rejects non-donation inputs")
end
check(table.getn(opened)==calls,"Rejected inputs never reach native browser opening")
local failures={false,function(page) table.insert(opened,page);return 0 end,function(page) table.insert(opened,page);error("Browser unavailable") end}
for _,native in ipairs(failures) do
    SaureksClosetOpenWebsite=native or nil
    for _,entry in ipairs(buttons) do
        V:OpenInfoPage("links")
        local before=table.getn(opened)
        click(V.donationLinksButton)
        check(table.getn(opened)==before,"Showing the QR code never attempts browser opening, even when the browser is unavailable")
        V.donationCopyAddress.focused=false;V.donationCopyAddress.highlighted=false
        click(entry[1])
        check(V.infoPages.donations:IsVisible() and not V.infoPages.links:IsVisible(),"Unavailable browser opens the donation fallback page")
        check(V.donationCopyAddress.text==V.websiteURLs[entry[2]],"Fallback retains the exact selected URL, including Ko-fi")
        check(V.donationCopyAddress.focused and V.donationCopyAddress.highlighted,"Fallback address is focused and selected for Ctrl+C")
        check(messages[table.getn(messages)]=="Open this address in your browser: "..V.websiteURLs[entry[2]],"Fallback chat message gives the selected address")
    end
end
V.donationCopyAddress.highlighted=false;event(V.donationCopyAddress,"OnEditFocusGained")
check(V.donationCopyAddress.highlighted,"Clicking the copy field selects its address")
event(V.donationCopyAddress,"OnEscapePressed");check(not V.donationCopyAddress.focused,"Escape releases the copy field")
click(V.settingsInfoWindow.close);check(not V.settingsInfoWindow:IsVisible(),"Close dismisses the donation window")
print("PASS: "..checks.." donation content, explicit browser actions, copy fallback, page visibility and non-overlapping layout checks")
