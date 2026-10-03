local V=VanityStudio
function V:AnimationControlsAvailable()
    if type(SaureksClosetAnimationVersion)~="function" or type(SaureksClosetSetCapeAnimation)~="function" then return false end
    local ok,version=pcall(SaureksClosetAnimationVersion)
    return ok and type(version)=="number" and version>=1
end
local animationRaces={"Human","Orc","Dwarf","Night Elf","Undead","Tauren","Gnome","Troll"}
function V:RaceAnimationControlsAvailable()
    if type(SaureksClosetSetAnimationStyles)~="function" or not self:AnimationControlsAvailable() then return false end
    local ok,version=pcall(SaureksClosetAnimationVersion);return ok and version>=2
end
function V:AnimationStyleSelection(kind)
    local value=VanityStudioCharacter[kind.."AnimationStyle"]
    if type(value)=="number" and value>=1 and value<=16 and value==math.floor(value) then return value end
    local body=(VanityStudioCharacter.enabled and VanityStudioCharacter.body) or self:NativeBody()
    return body and ((body.race-1)*2+body.sex+1) or 2
end
function V:SetAnimationStyle(kind,value,on)
    if (kind~="body" and kind~="cape") or not self:RaceAnimationControlsAvailable() or type(value)~="number" or value<1 or value>16 or value~=math.floor(value) then return false end
    VanityStudioCharacter[kind.."AnimationStyle"]=value
    VanityStudioCharacter[kind.."AnimationEnabled"]=on and true or nil
    if kind=="cape" and on then VanityStudioCharacter.calmCape=nil end
    self:SyncAnimations();self:InvalidatePreviewModel(0,true);self:SyncWeapons();self:RefreshAnimationsPage();return true
end
function V:ResetAnimationStyle(kind)
    if not self:RaceAnimationControlsAvailable() then
        if kind=="cape" then return self:SetCalmCape(false) end
        return false
    end
    VanityStudioCharacter[kind.."AnimationEnabled"]=nil;VanityStudioCharacter[kind.."AnimationStyle"]=nil
    if kind=="cape" then VanityStudioCharacter.calmCape=nil end
    self:SyncAnimations();self:InvalidatePreviewModel(0,true);self:SyncWeapons();self:RefreshAnimationsPage();return true
end
function V:ResetBagAnimations()
    if not self:AnimationControlsAvailable() then return false end
    for _,bag in ipairs(self:GetBags()) do bag.physics=true;bag.amplitude=100 end
    self:BagChanged();self:RefreshAnimationsPage();return true
end
function V:SyncAnimations()
    if not VanityStudioCharacter or self.sharingWorldPaused or not self:AnimationControlsAvailable() then return end
    local c=VanityStudioCharacter
    local previousError=self.capeAnimationError
    local ok,status
    if self:RaceAnimationControlsAvailable() then
        local body=c.enabled and c.bodyAnimationEnabled and self:AnimationStyleSelection("body") or 0
        local cape=c.enabled and c.capeAnimationEnabled and self:AnimationStyleSelection("cape") or 0
        ok,status=pcall(SaureksClosetSetAnimationStyles,body,cape,c.enabled and c.calmCape and 1 or 0)
    else ok,status=pcall(SaureksClosetSetCapeAnimation,c.enabled and c.calmCape and 1 or 0) end
    self.capeAnimationError=(not ok or status~=1) and (status==-5 and "Animation files could not be loaded. Reinstall the complete addon and check folder write access." or "Waiting for the character model...") or nil
    if previousError~=self.capeAnimationError then self:RefreshAnimationsPage() end
end
function V:SetCalmCape(enabled)
    if not self:AnimationControlsAvailable() then return false end
    VanityStudioCharacter.calmCape=enabled and true or nil
    if enabled then VanityStudioCharacter.capeAnimationEnabled=nil end
    self:SyncAnimations();self:InvalidatePreviewModel(0,true);self:SyncWeapons();self:RefreshAnimationsPage();return true
end
function V:RefreshAnimationsPage()
    if not self.animationBagRows then return end
    self.animationsRefreshing=true
    local available=self:AnimationControlsAvailable()
    self.animationNotice:SetText(available and "" or "Update SaureksCloset.dll and fully restart WoW.")
    for i,row in ipairs(self.animationBagRows) do
        local bag=self:BagInSlot(i);local model=bag and self.bagCatalogByID[bag.model]
        row.bagID=bag and bag.id
        row.title:SetText(model and ("Bag "..i..": "..model.name) or ("Bag "..i..": Empty"))
        row.toggle:SetChecked(bag and bag.physics~=false)
        row.slider:SetValue(self:BagPhysicsAmount(bag))
        row.amount:SetText("Amplitude: "..self:BagPhysicsAmount(bag).."%")
        row.enable(row.toggle,bag and available)
        row.enable(row.reset,bag and available)
        -- Vanilla Slider inherits Frame, not Button; Enable/Disable are absent.
        row.slider.closetEnabled=bag and available and bag.physics~=false and true or false
        row.slider:EnableMouse(row.slider.closetEnabled)
        row.slider:SetAlpha(row.slider.closetEnabled and 1 or .4)
    end
    self.animationEnable(self.bagAnimationDefault,available)
    self.animationEnable(self.capeAnimationDefault,available)
    self.calmCapeCheckbox:SetChecked(VanityStudioCharacter.calmCape)
    self.animationEnable(self.calmCapeCheckbox,available)
    self.capeAnimationStatus:SetText(self.capeAnimationError or (VanityStudioCharacter.capeAnimationEnabled and "Cape and character styles are controlled separately." or VanityStudioCharacter.calmCape and "Gentler animation selected. Applies to the human-female model." or "Using the original cape animation."))
    if self.animationStyleRows then
        local raceAvailable=self:RaceAnimationControlsAvailable()
        for kind,row in pairs(self.animationStyleRows) do
            local value=self:AnimationStyleSelection(kind);local on=VanityStudioCharacter[kind.."AnimationEnabled"]
            row.toggle:SetChecked(on);self.animationEnable(row.toggle,raceAvailable)
            row.race.caption:SetText(animationRaces[math.floor((value-1)/2)+1]);row.sex.caption:SetText(math.mod(value-1,2)==0 and "Male" or "Female")
            self.animationEnable(row.race,raceAvailable);self.animationEnable(row.sex,raceAvailable)
            self.animationEnable(row.reset,raceAvailable or (kind=="cape" and available))
            row.status:SetText(self.capeAnimationError or (not raceAvailable and "Update your DLL to select another race's animations." or (on and "Selected style is active. Your character model stays the same." or "Using your character's original animations.")))
        end
    end
    self.animationsRefreshing=nil
end
function V:CreateAnimationsPage(p,sheet,section,label,settingsButton,enable)
    self.animationEnable=enable
    label(p,"Animations",38,84,284,24)
    label(p,"Choose what you want to customize.",38,116,284,22,true)
    settingsButton(p,"Cape physics",38,156,284,function() V:RefreshAnimationsPage();V.capeAnimationWindow:Show() end)
    settingsButton(p,"Bag physics",38,204,284,function() V:RefreshAnimationsPage();V.bagAnimationWindow:Show() end)
    settingsButton(p,"Character animations",38,252,284,function() V:RefreshAnimationsPage();V.bodyAnimationWindow:Show() end)
    self.animationNotice=label(p,"",38,308,284,46,true)
    local function window(name,title)
        local f=sheet(name,UIParent,title);f:Hide();f:SetFrameStrata("DIALOG");f:SetClampedToScreen(true)
        f:SetPoint("TOPLEFT",V.frame,"TOPRIGHT",-30,0);f:SetMovable(true);f:RegisterForDrag("LeftButton")
        f:SetScript("OnDragStart",function() this:StartMoving() end)
        f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
        f.close:SetScript("OnClick",function() f:Hide() end)
        table.insert(UISpecialFrames,f:GetName());return f
    end
    local function help(control,title,text)
        control:SetScript("OnEnter",function() GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText(title);GameTooltip:AddLine(text,1,1,1,true);GameTooltip:Show() end)
        control:SetScript("OnLeave",function() GameTooltip:Hide() end)
    end
    self.bagAnimationWindow=window("SaureksClosetBagAnimations","Bag Physics")
    self.animationBagRows={}
    for i=1,self.MAX_BAGS do
        local row=section(self.bagAnimationWindow,27,82+(i-1)*59,308,56,false)
        row.enable=enable;self.animationBagRows[i]=row
        row.title=label(row,"",9,5,201,18);row.title:SetFont("Fonts\\FRIZQT__.TTF",10)
        row.toggle=CreateFrame("CheckButton","SaureksClosetBagPhysics"..i,row,"UICheckButtonTemplate")
        row.toggle:ClearAllPoints();row.toggle:SetPoint("TOPRIGHT",row,"TOPRIGHT",-5,-2);row.toggle:SetWidth(24);row.toggle:SetHeight(24)
        label(row,"Physics",219,7,59,16,true):SetFont("Fonts\\FRIZQT__.TTF",9)
        row.toggle:SetScript("OnClick",function() if row.bagID then V:SetBagPhysics(row.bagID,this:GetChecked(),V:BagPhysicsAmount(V:BagInstance(row.bagID))) end end)
        help(row.toggle,"Dynamic bag physics","Off: the bag follows its mounting point on the animated player, without extra bounce, swing or deformation. This setting is saved with the look.")
        row.amount=label(row,"",9,32,100,18,true);row.amount:SetFont("Fonts\\FRIZQT__.TTF",9)
        row.slider=CreateFrame("Slider","SaureksClosetBagAmplitude"..i,row,"OptionsSliderTemplate")
        row.slider:ClearAllPoints();row.slider:SetPoint("TOPLEFT",row,"TOPLEFT",110,-32);row.slider:SetWidth(110);row.slider:SetHeight(16)
        row.slider:SetMinMaxValues(0,200);row.slider:SetValueStep(5)
        for _,suffix in ipairs({"Low","High","Text"}) do local t=getglobal(row.slider:GetName()..suffix);if t then t:Hide() end end
        row.slider:SetScript("OnValueChanged",function()
            local amount=math.floor(this:GetValue()+.5);row.amount:SetText("Amplitude: "..amount.."%")
            if not V.animationsRefreshing and this.closetEnabled and row.bagID then V:SetBagPhysics(row.bagID,true,amount) end
        end)
        row.reset=section(row,232,29,66,22,true,"Button",.75)
        local caption=label(row.reset,"Default",3,2,60,18,true);caption:SetFont("Fonts\\FRIZQT__.TTF",10);caption:SetJustifyH("CENTER");caption:SetJustifyV("MIDDLE")
        row.reset:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        row.reset:SetScript("OnClick",function() if row.bagID then V:SetBagPhysics(row.bagID,true,100);V:RefreshAnimationsPage() end end)
        help(row.reset,"Default bag movement","Restore this bag's original dynamic motion: physics on, amplitude 100%. Placement and size stay unchanged.")
        help(row.slider,"Physics amplitude","100% preserves the existing movement. Lower values reduce secondary motion; 0% follows the player without dynamics. Higher values increase movement without speeding it up. Rigid bags never stretch.")
    end
    self.bagAnimationDefault=settingsButton(self.bagAnimationWindow,"Default (all bags)",36,386,298,function() V:ResetBagAnimations() end)
    self.animationStyleRows={}
    local function styleControls(kind,f,title)
        local row={};V.animationStyleRows[kind]=row
        label(f,title,36,84,295,24)
        row.toggle=CreateFrame("CheckButton","SaureksCloset"..kind.."AnimationToggle",f,"UICheckButtonTemplate")
        row.toggle:ClearAllPoints();row.toggle:SetPoint("TOPLEFT",f,"TOPLEFT",33,-120);row.toggle:SetWidth(24);row.toggle:SetHeight(24)
        label(f,"Use another race's animations",63,125,263,24,true)
        row.toggle:SetScript("OnClick",function() V:SetAnimationStyle(kind,V:AnimationStyleSelection(kind),this:GetChecked()) end)
        help(row.toggle,title,kind=="cape" and "Transfer only cape motion. Character movement is controlled separately. Turning this off restores your own cape movement." or "Transfer character motion while keeping your current model, textures and equipment. Cape style is controlled separately. Missing animations keep their original movement.")
        local function picker(part)
            local b
            b=settingsButton(f,part=="race" and "Race" or "Female",part=="race" and 38 or 191,162,part=="race" and 145 or 133,function()
                if not V.animationStyleMenu then V.animationStyleMenu=CreateFrame("Frame","SaureksClosetAnimationStyleMenu",V.frame);V.animationStyleMenu.displayMode="MENU";V.animationStyleMenu:Hide() end
                V.animationStyleMenu.initialize=function()
                    local selected=V:AnimationStyleSelection(kind)
                    local function option(index)
                        local choice=part=="race" and ((index-1)*2+math.mod(selected-1,2)+1) or (math.floor((selected-1)/2)*2+index)
                        UIDropDownMenu_AddButton({text=part=="race" and animationRaces[index] or (index==1 and "Male" or "Female"),checked=choice==selected and 1 or nil,func=function() V:SetAnimationStyle(kind,choice,true) end})
                    end
                    for i=1,part=="race" and 8 or 2 do option(i) end
                end
                ToggleDropDownMenu(1,nil,V.animationStyleMenu,b.styleAnchor:GetName(),0,0)
            end)
            b.styleAnchor=CreateFrame("Frame","SaureksCloset"..kind..part.."StyleAnchor",b);b.styleAnchor:SetAllPoints(b)
            return b
        end
        row.race=picker("race");row.sex=picker("sex")
        row.status=label(f,"",38,218,286,62,true)
        row.reset=settingsButton(f,"Default",38,378,160,function() V:ResetAnimationStyle(kind) end)
        return row
    end
    self.bodyAnimationWindow=window("SaureksClosetBodyAnimations","Character Animations")
    styleControls("body",self.bodyAnimationWindow,"Character animation style")
    label(self.bodyAnimationWindow,"Uses matching animations from the selected race and gender. Your body shape, appearance and equipment remain your own. Actions missing from the selected style use your original animation.",38,285,286,78,true)
    self.capeAnimationWindow=window("SaureksClosetCapeAnimations","Cape Animation")
    local capeRow=styleControls("cape",self.capeAnimationWindow,"Cape animation style")
    self.capeAnimationDefault=capeRow.reset
    self.calmCapeCheckbox=CreateFrame("CheckButton","SaureksClosetCalmCape",self.capeAnimationWindow,"UICheckButtonTemplate")
    self.calmCapeCheckbox:ClearAllPoints();self.calmCapeCheckbox:SetPoint("TOPLEFT",self.capeAnimationWindow,"TOPLEFT",33,-289);self.calmCapeCheckbox:SetWidth(24);self.calmCapeCheckbox:SetHeight(24)
    label(self.capeAnimationWindow,"Gentler human-female cape",63,294,263,24,true)
    self.calmCapeCheckbox:SetScript("OnClick",function() V:SetCalmCape(this:GetChecked()) end)
    help(self.calmCapeCheckbox,"Gentler cape movement","The original calmer human-female option. Replaces the selected cape style without changing the character animation selection.")
    self.capeAnimationStatus=label(self.capeAnimationWindow,"",38,326,286,42,true)
    self:RefreshAnimationsPage()
end
