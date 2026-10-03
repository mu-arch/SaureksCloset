local V=VanityStudio
function V:PhysicsControlsAvailable()
    if type(SaureksClosetPhysicsVersion)~="function" or type(SaureksClosetSetWeaponPhysics)~="function" then return false end
    local ok,version=pcall(SaureksClosetPhysicsVersion);return ok and type(version)=="number" and version>=1
end
function V:ResetBagPhysics()
    if not self:PhysicsControlsAvailable() then return false end
    for _,bag in ipairs(self:GetBags()) do bag.physics=true;bag.amplitude=100 end
    self:BagChanged();self:RefreshPhysicsPage();return true
end
function V:SyncPhysics()
    local c=VanityStudioCharacter;if not c then return end
    c.calmCape=nil;c.bodyAnimationStyle=nil;c.bodyAnimationEnabled=nil;c.capeAnimationStyle=nil;c.capeAnimationEnabled=nil
    if self.sharingWorldPaused or not self:PhysicsControlsAvailable() then return end
    pcall(SaureksClosetSetWeaponPhysics,c.enabled and c.weaponPhysics and 1 or 0)
end
function V:SetWeaponPhysics(enabled)
    if not self:PhysicsControlsAvailable() then return false end
    VanityStudioCharacter.weaponPhysics=enabled and true or nil
    self:SyncPhysics();self:RefreshPhysicsPage();return true
end
function V:RefreshPhysicsPage()
    if not self.bagPhysicsRows then return end
    self.physicsRefreshing=true
    local available=self:PhysicsControlsAvailable()
    self.physicsNotice:SetText(available and "" or "Update SaureksCloset.dll and fully restart WoW.")
    for i,row in ipairs(self.bagPhysicsRows) do
        local bag=self:BagInSlot(i);local model=bag and self.bagCatalogByID[bag.model]
        row.bagID=bag and bag.id
        row.title:SetText(model and ("Bag "..i..": "..model.name) or ("Bag "..i..": Empty"))
        row.toggle:SetChecked(bag and bag.physics~=false)
        row.slider:SetValue(self:BagPhysicsAmount(bag))
        row.amount:SetText("Amplitude: "..self:BagPhysicsAmount(bag).."%")
        row.enable(row.toggle,bag and available);row.enable(row.reset,bag and available)
        -- Vanilla sliders are Frames, not Buttons: they have no Enable/Disable.
        row.slider.closetEnabled=bag and available and bag.physics~=false and true or false
        row.slider:EnableMouse(row.slider.closetEnabled);row.slider:SetAlpha(row.slider.closetEnabled and 1 or .4)
    end
    self.physicsEnable(self.bagPhysicsDefault,available)
    self.physicsEnable(self.weaponPhysicsToggle,available);self.physicsEnable(self.weaponPhysicsDefault,available)
    self.weaponPhysicsToggle:SetChecked(VanityStudioCharacter.weaponPhysics)
    self.physicsRefreshing=nil
end
function V:CreatePhysicsPage(p,sheet,section,label,settingsButton,enable)
    self.physicsEnable=enable
    label(p,"Physics",38,84,284,24)
    label(p,"Choose what you want to customize.",38,116,284,22,true)
    settingsButton(p,"Bag physics",38,156,284,function() V:RefreshPhysicsPage();V.bagPhysicsWindow:Show() end)
    settingsButton(p,"Weapon physics",38,204,284,function() V:RefreshPhysicsPage();V.weaponPhysicsWindow:Show() end)
    self.physicsNotice=label(p,"",38,260,284,46,true)
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
    self.bagPhysicsWindow=window("SaureksClosetBagPhysicsWindow","Bag Physics")
    self.bagPhysicsRows={}
    for i=1,self.MAX_BAGS do
        local row=section(self.bagPhysicsWindow,27,82+(i-1)*59,308,56,false)
        row.enable=enable;self.bagPhysicsRows[i]=row
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
            if not V.physicsRefreshing and this.closetEnabled and row.bagID then V:SetBagPhysics(row.bagID,true,amount) end
        end)
        row.reset=section(row,232,29,66,22,true,"Button",.75)
        local caption=label(row.reset,"Default",3,2,60,18,true);caption:SetFont("Fonts\\FRIZQT__.TTF",10);caption:SetJustifyH("CENTER");caption:SetJustifyV("MIDDLE")
        row.reset:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        row.reset:SetScript("OnClick",function() if row.bagID then V:SetBagPhysics(row.bagID,true,100);V:RefreshPhysicsPage() end end)
        help(row.reset,"Default bag movement","Restore this bag's original dynamic motion: physics on, amplitude 100%. Placement and size stay unchanged.")
        help(row.slider,"Physics amplitude","100% preserves the existing movement. Lower values reduce secondary motion; 0% follows the player without dynamics. Higher values increase movement without speeding it up. Rigid bags never stretch.")
    end
    self.bagPhysicsDefault=settingsButton(self.bagPhysicsWindow,"Default (all bags)",36,386,298,function() V:ResetBagPhysics() end)
    self.weaponPhysicsWindow=window("SaureksClosetWeaponPhysics","Weapon Physics")
    label(self.weaponPhysicsWindow,"Rigid-body movement",36,84,295,24)
    self.weaponPhysicsToggle=CreateFrame("CheckButton","SaureksClosetWeaponPhysicsToggle",self.weaponPhysicsWindow,"UICheckButtonTemplate")
    self.weaponPhysicsToggle:ClearAllPoints();self.weaponPhysicsToggle:SetPoint("TOPLEFT",self.weaponPhysicsWindow,"TOPLEFT",33,-124)
    self.weaponPhysicsToggle:SetWidth(24);self.weaponPhysicsToggle:SetHeight(24)
    label(self.weaponPhysicsWindow,"Enable weapon physics",63,129,263,24,true)
    self.weaponPhysicsToggle:SetScript("OnClick",function() V:SetWeaponPhysics(this:GetChecked()) end)
    help(self.weaponPhysicsToggle,"Rigid-body weapon physics","Apply the bags' secondary motion to stowed and body-carried weapons. Weapons bob and rock as solid objects and never stretch. Drawn weapons keep their normal hand animation.")
    label(self.weaponPhysicsWindow,"Stowed and body-carried weapons gain a little weight and momentum as you move, using the same rigid-body motion as bags.\n\nWeapons stay solid and settle when you stop. Weapons in your hands keep their normal animation.\n\nDefault turns weapon physics off.",38,180,286,160,true)
    self.weaponPhysicsDefault=settingsButton(self.weaponPhysicsWindow,"Default",38,378,160,function() V:SetWeaponPhysics(false) end)
    self:RefreshPhysicsPage()
end
