local V=VanityStudio
function V:AnimationControlsAvailable()
    if type(SaureksClosetAnimationVersion)~="function" or type(SaureksClosetSetCapeAnimation)~="function" then return false end
    local ok,version=pcall(SaureksClosetAnimationVersion)
    return ok and type(version)=="number" and version>=1
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
    local ok,status=pcall(SaureksClosetSetCapeAnimation,c.enabled and c.calmCape and 1 or 0)
    self.capeAnimationError=(not ok or status~=1) and (status==-5 and "The cape animation file is missing. Reinstall the complete addon." or "Waiting for the character model...") or nil
    if previousError~=self.capeAnimationError then self:RefreshAnimationsPage() end
end
function V:SetCalmCape(enabled)
    if not self:AnimationControlsAvailable() then return false end
    VanityStudioCharacter.calmCape=enabled and true or nil
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
    self.capeAnimationStatus:SetText(self.capeAnimationError or (VanityStudioCharacter.calmCape and "Gentler animation selected. Applies to the human-female model." or "Using the original cape animation."))
    self.animationsRefreshing=nil
end
function V:CreateAnimationsPage(p,sheet,section,label,settingsButton,enable)
    self.animationEnable=enable
    label(p,"Animations",38,84,284,24)
    label(p,"Choose what you want to customize.",38,116,284,22,true)
    settingsButton(p,"Cape physics",38,156,284,function() V:RefreshAnimationsPage();V.capeAnimationWindow:Show() end)
    settingsButton(p,"Bag physics",38,204,284,function() V:RefreshAnimationsPage();V.bagAnimationWindow:Show() end)
    self.animationNotice=label(p,"",38,260,284,46,true)
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
    self.capeAnimationWindow=window("SaureksClosetCapeAnimations","Cape Animation")
    label(self.capeAnimationWindow,"Human female",36,84,295,24)
    self.calmCapeCheckbox=CreateFrame("CheckButton","SaureksClosetCalmCape",self.capeAnimationWindow,"UICheckButtonTemplate")
    self.calmCapeCheckbox:ClearAllPoints();self.calmCapeCheckbox:SetPoint("TOPLEFT",self.capeAnimationWindow,"TOPLEFT",33,-124);self.calmCapeCheckbox:SetWidth(24);self.calmCapeCheckbox:SetHeight(24)
    label(self.capeAnimationWindow,"Gentler cape movement",63,129,263,24,true)
    self.calmCapeCheckbox:SetScript("OnClick",function() V:SetCalmCape(this:GetChecked()) end)
    help(self.calmCapeCheckbox,"Gentler cape movement","Uses a baked animation with less lower-cape swing when walking, running and walking backwards. No dynamic cloth simulation. Other bodies keep their original animation.")
    label(self.capeAnimationWindow,"The cape keeps its original attachment and animation timing, with a calmer lower edge.\n\nApplies to the human-female model, including a human-female body appearance. Other races and genders keep their original animation.",38,171,286,132,true)
    self.capeAnimationStatus=label(self.capeAnimationWindow,"",38,318,286,54,true)
    self.capeAnimationDefault=settingsButton(self.capeAnimationWindow,"Default",38,378,160,function() V:SetCalmCape(false) end)
    self:RefreshAnimationsPage()
end
