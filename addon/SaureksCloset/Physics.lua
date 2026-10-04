local V=VanityStudio
local weaponControls={
    {key="weaponBounce",default=40,title="Bounce",tip="Adjust up-and-down movement while walking and running. This does not change animation speed."},
    {key="weaponRocking",default=115,title="Rocking",tip="Adjust how far weapons rock and swing around their mounting point. Weapons remain rigid at every setting."},
    {key="weaponJumpLift",default=200,title="Jump lift",tip="Adjust the extra outward lift when jumping or falling. 0% removes this extra lift while retaining the normal player animation."}
}
function V:PhysicsControlsAvailable(minimum)
    if type(SaureksClosetPhysicsVersion)~="function" or type(SaureksClosetSetWeaponPhysics)~="function" then return false end
    local ok,version=pcall(SaureksClosetPhysicsVersion);return ok and type(version)=="number" and version>=(minimum or 1)
end
function V:WeaponPhysicsConfig(slot)
    slot=slot or self.weaponPhysicsSlot or 0
    local slots=VanityStudioCharacter.weaponPhysicsSlots
    return slot~=0 and slots and slots[slot] or VanityStudioCharacter
end
function V:SelectWeaponPhysicsSlot(slot)
    if not self:CommitWeaponPhysicsEditors() then return false end
    self.weaponPhysicsSlot=slot;self:RefreshPhysicsPage();return true
end
function V:SetWeaponPhysicsCustom(custom)
    local slot=self.weaponPhysicsSlot or 0
    if slot==0 or not self:PhysicsControlsAvailable(3) then return false end
    local c=VanityStudioCharacter;c.weaponPhysicsSlots=c.weaponPhysicsSlots or {}
    if custom then
        local config={weaponPhysics=c.weaponPhysics}
        for _,control in ipairs(weaponControls) do config[control.key]=self:WeaponPhysicsAmount(control.key,0) end
        c.weaponPhysicsSlots[slot]=config
    else c.weaponPhysicsSlots[slot]=nil end
    self:SyncPhysics();self:RefreshPhysicsPage();return true
end
function V:WeaponPhysicsAmount(key,slot)
    local amount=tonumber(self:WeaponPhysicsConfig(slot)[key])
    if not amount or amount~=amount then
        for _,control in ipairs(weaponControls) do if control.key==key then return control.default end end
        return 100
    end
    return math.floor(math.max(0,math.min(200,amount))+.5)
end
function V:SetWeaponPhysicsAmount(key,amount)
    if not self:PhysicsControlsAvailable(2) or type(amount)~="number" or amount~=amount or amount<0 or amount>200 then return false end
    local valid=false;for _,control in ipairs(weaponControls) do if key==control.key then valid=true end end
    if not valid then return false end
    if (self.weaponPhysicsSlot or 0)~=0 and self:WeaponPhysicsConfig()==VanityStudioCharacter then return false end
    self:WeaponPhysicsConfig()[key]=math.floor(amount+.5)
    self:SyncPhysics();self:RefreshPhysicsPage();return true
end
function V:CommitWeaponPhysicsEditor(editor)
    if self.physicsRefreshing or not editor.editing then return true end
    editor.editing=nil
    local amount=tonumber(editor:GetText())
    local valid=editor.closetEnabled and editor.targetSlot==(self.weaponPhysicsSlot or 0) and editor.targetConfig==self:WeaponPhysicsConfig()
        and amount and amount==amount and amount>=0 and amount<=200
    if valid then valid=self:SetWeaponPhysicsAmount(editor.key,amount) end
    self.weaponPhysicsInvalid=not valid
    self.weaponPhysicsStatus:SetText(valid and "" or "Enter a value from 0 to 200.")
    self:RefreshPhysicsPage();return valid and true or false
end
function V:CommitWeaponPhysicsEditors()
    for _,row in ipairs(self.weaponPhysicsRows or {}) do
        if row.editor.editing then
            local ok=self:CommitWeaponPhysicsEditor(row.editor);row.editor:ClearFocus()
            if not ok then self.weaponPhysicsInvalid=nil;return false end
        end
    end
    if self.weaponPhysicsInvalid then self.weaponPhysicsInvalid=nil;return false end
    return true
end
function V:ResetWeaponPhysics()
    if not self:PhysicsControlsAvailable() then return false end
    for _,row in ipairs(self.weaponPhysicsRows or {}) do row.editor.editing=nil;row.editor:ClearFocus() end
    self.weaponPhysicsInvalid=nil;self.weaponPhysicsStatus:SetText("")
    if (self.weaponPhysicsSlot or 0)~=0 then return self:SetWeaponPhysicsCustom(false) end
    for _,control in ipairs(weaponControls) do VanityStudioCharacter[control.key]=control.default end
    return self:SetWeaponPhysics(false)
end
function V:OpenPhysicsPanel(kind)
    self.bagPhysicsWindow:Hide();self.weaponPhysicsWindow:Hide();if self.capePhysicsWindow then self.capePhysicsWindow:Hide();self.capeAdvancedWindow:Hide() end
    self:RefreshPhysicsPage()
    if kind=="bags" then self.bagPhysicsWindow:Show()
    elseif kind=="weapons" then self.weaponPhysicsWindow:Show()
    elseif kind=="cape" then self:RefreshCapePhysics(true);self.capePhysicsWindow:Show() end
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
    pcall(SaureksClosetSetWeaponPhysics,c.enabled and c.weaponPhysics and 1 or 0,
        self:WeaponPhysicsAmount("weaponBounce",0),self:WeaponPhysicsAmount("weaponRocking",0),self:WeaponPhysicsAmount("weaponJumpLift",0))
    if self:PhysicsControlsAvailable(3) and type(SaureksClosetSetWeaponSlotPhysics)=="function" then
        for slot=101,110 do if slot~=107 then
            local custom=c.weaponPhysicsSlots and c.weaponPhysicsSlots[slot]
            pcall(SaureksClosetSetWeaponSlotPhysics,slot,custom and (c.enabled and custom.weaponPhysics and 2 or 1) or 0,
                self:WeaponPhysicsAmount("weaponBounce",slot),self:WeaponPhysicsAmount("weaponRocking",slot),self:WeaponPhysicsAmount("weaponJumpLift",slot))
        end end
    end
    if self.SyncCapePhysics then self:SyncCapePhysics() end
end
function V:SetWeaponPhysics(enabled)
    if not self:PhysicsControlsAvailable() then return false end
    if (self.weaponPhysicsSlot or 0)~=0 and self:WeaponPhysicsConfig()==VanityStudioCharacter then return false end
    self:WeaponPhysicsConfig().weaponPhysics=enabled and true or nil
    self:SyncPhysics();self:RefreshPhysicsPage();return true
end
function V:RefreshPhysicsPage()
    if not self.bagPhysicsRows then return end
    self.physicsRefreshing=true
    local available=self:PhysicsControlsAvailable()
    self.physicsNotice:SetText(self:PhysicsControlsAvailable(3) and "" or "Update SaureksCloset.dll and fully restart WoW.")
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
    local selected=self.weaponPhysicsSlot or 0
    local custom=selected~=0 and self:WeaponPhysicsConfig()~=VanityStudioCharacter
    local editable=available and (selected==0 or (custom and self:PhysicsControlsAvailable(3)))
    self.physicsEnable(self.weaponPhysicsToggle,editable);self.physicsEnable(self.weaponPhysicsDefault,available)
    self.weaponPhysicsToggle:SetChecked(self:WeaponPhysicsConfig().weaponPhysics)
    self.weaponPhysicsSelector.caption:SetText(selected==0 and "Shared defaults" or self.weaponNames[selected-100])
    self.weaponPhysicsCustom:SetChecked(custom)
    if selected==0 then self.weaponPhysicsCustom:Hide();self.weaponPhysicsCustomLabel:Hide()
    else self.weaponPhysicsCustom:Show();self.weaponPhysicsCustomLabel:Show() end
    self.physicsEnable(self.weaponPhysicsCustom,self:PhysicsControlsAvailable(3))
    for _,row in ipairs(self.weaponPhysicsRows) do
        local active=editable and self:PhysicsControlsAvailable(2) and self:WeaponPhysicsConfig().weaponPhysics and true or false
        local editor=row.editor
        if editor.editing and (not active or editor.targetSlot~=selected or editor.targetConfig~=self:WeaponPhysicsConfig()) then
            editor.editing=nil;editor:ClearFocus()
        end
        if not editor.editing then editor:SetText(self:WeaponPhysicsAmount(row.key)) end
        editor.closetEnabled=active;editor:EnableMouse(active);editor:SetAlpha(active and 1 or .4)
        self.physicsEnable(row.minus,active);self.physicsEnable(row.plus,active)
    end
    self.physicsRefreshing=nil
end
function V:CreatePhysicsPage(p,sheet,section,label,settingsButton,enable,edit,redButton)
    self.physicsEnable=enable
    label(p,"Physics",38,84,284,24)
    self.physicsSubtitle=label(p,"These are experimental features that are not complete and are only included for testing.",38,116,284,58,true)
    self.bagPhysicsButton=settingsButton(p,"Bag physics",38,184,284,function() V:OpenPhysicsPanel("bags") end)
    self.weaponPhysicsButton=settingsButton(p,"Weapon physics",38,232,284,function() V:OpenPhysicsPanel("weapons") end)
    self.capePhysicsButton=settingsButton(p,"Cape physics",38,280,284,function() V:OpenPhysicsPanel("cape") end)
    self.physicsNotice=label(p,"",38,326,284,46,true)
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
    self.weaponPhysicsSelector=settingsButton(self.weaponPhysicsWindow,"Shared defaults",36,80,294,function()
        ToggleDropDownMenu(1,nil,V.weaponPhysicsMenu,"SaureksClosetWeaponPhysicsMenuAnchor",0,0)
    end)
    local anchor=CreateFrame("Frame","SaureksClosetWeaponPhysicsMenuAnchor",self.weaponPhysicsSelector);anchor:SetAllPoints(self.weaponPhysicsSelector)
    self.weaponPhysicsMenu=CreateFrame("Frame","SaureksClosetWeaponPhysicsMenu",self.weaponPhysicsWindow,"UIDropDownMenuTemplate")
    UIDropDownMenu_Initialize(self.weaponPhysicsMenu,function()
        local function entry(slot,title)
            UIDropDownMenu_AddButton({text=title,checked=(V.weaponPhysicsSlot or 0)==slot,func=function() V:SelectWeaponPhysicsSlot(slot) end})
        end
        entry(0,"Shared defaults")
        for _,slot in ipairs({108,109,110,101,102,103,104,105,106}) do entry(slot,V.weaponNames[slot-100]) end
    end,"MENU")
    self.weaponPhysicsCustom=CreateFrame("CheckButton","SaureksClosetWeaponPhysicsCustom",self.weaponPhysicsWindow,"UICheckButtonTemplate")
    self.weaponPhysicsCustom:ClearAllPoints();self.weaponPhysicsCustom:SetPoint("TOPLEFT",self.weaponPhysicsWindow,"TOPLEFT",33,-116)
    self.weaponPhysicsCustom:SetWidth(24);self.weaponPhysicsCustom:SetHeight(24)
    self.weaponPhysicsCustomLabel=label(self.weaponPhysicsWindow,"Custom settings for this slot",63,121,263,18,true)
    self.weaponPhysicsCustom:SetScript("OnClick",function() if V:CommitWeaponPhysicsEditors() then V:SetWeaponPhysicsCustom(this:GetChecked()) else V:RefreshPhysicsPage() end end)
    help(self.weaponPhysicsCustom,"Custom slot physics","Enable to give this slot its own physics settings. Off uses Shared defaults. Default returns just this slot to Shared defaults.")
    self.weaponPhysicsToggle=CreateFrame("CheckButton","SaureksClosetWeaponPhysicsToggle",self.weaponPhysicsWindow,"UICheckButtonTemplate")
    self.weaponPhysicsToggle:ClearAllPoints();self.weaponPhysicsToggle:SetPoint("TOPLEFT",self.weaponPhysicsWindow,"TOPLEFT",33,-144)
    self.weaponPhysicsToggle:SetWidth(24);self.weaponPhysicsToggle:SetHeight(24)
    label(self.weaponPhysicsWindow,"Enable weapon physics",63,149,263,24,true)
    self.weaponPhysicsToggle:SetScript("OnClick",function() if V:CommitWeaponPhysicsEditors() then V:SetWeaponPhysics(this:GetChecked()) else V:RefreshPhysicsPage() end end)
    help(self.weaponPhysicsToggle,"Rigid-body weapon physics","Apply the bags' secondary motion to stowed and body-carried weapons. Weapons bob and rock as solid objects and never stretch. Drawn weapons keep their normal hand animation.")
    self.weaponPhysicsRows={}
    for i,control in ipairs(weaponControls) do
        local row=section(self.weaponPhysicsWindow,34,178+(i-1)*62,296,56,false)
        row.key=control.key;self.weaponPhysicsRows[i]=row
        label(row,control.title,10,5,206,18,true)
        row.editor=edit(row,"SaureksClosetWeaponAmount"..i,108,29,72,16)
        row.editor.key=control.key;row.editor:SetJustifyH("CENTER")
        row.editor:SetScript("OnEditFocusGained",function()
            this.editing=true;this.targetSlot=V.weaponPhysicsSlot or 0;this.targetConfig=V:WeaponPhysicsConfig()
        end)
        row.editor:SetScript("OnEditFocusLost",function() V:CommitWeaponPhysicsEditor(this) end)
        row.editor:SetScript("OnEnterPressed",function() V:CommitWeaponPhysicsEditor(this);this:ClearFocus() end)
        row.editor:SetScript("OnEscapePressed",function() this.editing=nil;this:ClearFocus();V:RefreshPhysicsPage() end)
        label(row,"%",186,29,16,20,true)
        help(row.editor,control.title,control.tip.." Enter a value from 0 to 200. Default: "..control.default.."%.")
        local function nudge(x,text,direction)
            local b=section(row,x,27,22,22,true,"Button",.75);b.key=row.key;b.direction=direction
            local caption=label(b,text,0,1,22,20,true);caption:SetFont("Fonts\\FRIZQT__.TTF",14);caption:SetJustifyH("CENTER");caption:SetJustifyV("MIDDLE")
            b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
            b:SetScript("OnMouseDown",function() this.targetSlot=V.weaponPhysicsSlot or 0;this.targetConfig=V:WeaponPhysicsConfig() end)
            b:SetScript("OnClick",function()
                if not this.closetEnabled then return end
                local slot=this.targetSlot or V.weaponPhysicsSlot or 0;local config=this.targetConfig or V:WeaponPhysicsConfig()
                this.targetSlot=nil;this.targetConfig=nil
                if not V:CommitWeaponPhysicsEditors() or slot~=(V.weaponPhysicsSlot or 0) or config~=V:WeaponPhysicsConfig() then return end
                local step=IsShiftKeyDown and IsShiftKeyDown() and 10 or 1
                V:SetWeaponPhysicsAmount(this.key,math.max(0,math.min(200,V:WeaponPhysicsAmount(this.key)+this.direction*step)))
                V.weaponPhysicsStatus:SetText("")
            end)
            help(b,control.title,"Adjust by 1 percentage point. Hold Shift for 10. Range: 0 to 200%.")
            return b
        end
        row.minus=nudge(76,"-",-1);row.plus=nudge(207,"+",1)
    end
    self.weaponPhysicsStatus=label(self.weaponPhysicsWindow,"",38,357,290,18,true)
    self.weaponPhysicsDefault=settingsButton(self.weaponPhysicsWindow,"Default",38,378,160,function() V:ResetWeaponPhysics() end)
    help(self.weaponPhysicsDefault,"Default weapon movement","For a custom slot, return to Shared defaults. For Shared defaults, turn physics off and restore Bounce 40%, Rocking 115%, and Jump lift 200%. Other custom slots stay unchanged.")
    self:CreateCapePhysicsWindow(window,section,label,settingsButton,enable,help,edit,redButton)
    self:RefreshPhysicsPage()
end
