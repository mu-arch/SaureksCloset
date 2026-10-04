local V=VanityStudio
local controls={
    {"walk","Walking motion","Adjust lower-cape movement during walking and backward walking."},
    {"run","Running motion","Adjust lower-cape movement during running."},
    {"idle","Idle motion","Adjust lower-cape movement while standing."},
    {"air","Airborne motion","Adjust lower-cape movement during jumping, falling and landing."}
}
function V:CapePhysicsAvailable()
    return self:PhysicsControlsAvailable(3) and type(SaureksClosetSetCapeMotion)=="function"
end
function V:CapePhysicsConfig()
    local saved=VanityStudioCharacter.capeMotion or {};local config={enabled=saved.enabled and true or false}
    for _,entry in ipairs(controls) do
        local n=tonumber(saved[entry[1]]) or 100;if n~=n then n=100 end
        config[entry[1]]=math.floor(math.max(0,math.min(200,n))+.5)
    end
    return config
end
function V:SyncCapePhysics()
    if not self:CapePhysicsAvailable() or self.sharingWorldPaused then return false end
    local config=self:CapePhysicsConfig()
    local ok,status=pcall(SaureksClosetSetCapeMotion,VanityStudioCharacter.enabled and config.enabled and 1 or 0,config.walk,config.run,config.idle,config.air)
    return ok and status==1
end
function V:ApplyCapePhysics(reset)
    if not self:CapePhysicsAvailable() then return false end
    local config=reset and {enabled=false,walk=100,run=100,idle=100,air=100} or self.capePhysicsDraft
    if not config then return false end
    local previous=VanityStudioCharacter.capeMotion;VanityStudioCharacter.capeMotion=config
    if not self:SyncCapePhysics() then
        VanityStudioCharacter.capeMotion=previous
        self.capePhysicsStatus:SetText("Could not apply. Check the DLL and cape files.");return false
    end
    self:RefreshCapePhysics(true)
    if self.InvalidatePreviewModel then self:InvalidatePreviewModel() end
    return true
end
function V:RefreshCapePhysics(reset)
    if not self.capePhysicsRows then return end
    if reset or not self.capePhysicsDraft then self.capePhysicsDraft=self:CapePhysicsConfig() end
    local available=self:CapePhysicsAvailable();local config=self.capePhysicsDraft
    self.capePhysicsRefreshing=true
    self.capePhysicsToggle:SetChecked(config.enabled)
    self.physicsEnable(self.capePhysicsToggle,available);self.physicsEnable(self.capePhysicsApply,available);self.physicsEnable(self.capePhysicsDefault,available)
    self.capePhysicsStatus:SetText(available and "" or "Update the DLL and fully restart WoW.")
    for _,row in ipairs(self.capePhysicsRows) do
        row.slider:SetValue(config[row.key]);row.amount:SetText(config[row.key].."%")
        row.slider.closetEnabled=available and config.enabled
        row.slider:EnableMouse(row.slider.closetEnabled);row.slider:SetAlpha(row.slider.closetEnabled and 1 or .4)
    end
    self.capePhysicsRefreshing=nil
end
function V:CreateCapePhysicsWindow(window,section,label,settingsButton,enable,help)
    self.capePhysicsWindow=window("SaureksClosetCapePhysics","Cape Physics")
    local f=self.capePhysicsWindow
    self.capePhysicsToggle=CreateFrame("CheckButton","SaureksClosetCapeMotionToggle",f,"UICheckButtonTemplate")
    self.capePhysicsToggle:ClearAllPoints();self.capePhysicsToggle:SetPoint("TOPLEFT",f,"TOPLEFT",33,-80);self.capePhysicsToggle:SetWidth(24);self.capePhysicsToggle:SetHeight(24)
    label(f,"Customize cape motion",63,85,263,20,true)
    self.capePhysicsToggle:SetScript("OnClick",function() V.capePhysicsDraft.enabled=this:GetChecked() and true or false;V:RefreshCapePhysics() end)
    help(self.capePhysicsToggle,"Cape motion","Adjust the existing cape animation, without live cloth simulation or collisions. The upper attachment and character animations stay native. Click Apply to use your changes.")
    self.capePhysicsRows={}
    for i,entry in ipairs(controls) do
        local row=section(f,34,116+(i-1)*57,296,52,false);row.key=entry[1];self.capePhysicsRows[i]=row
        label(row,entry[2],10,5,206,18,true)
        row.amount=label(row,"",222,5,62,18,true);row.amount:SetJustifyH("RIGHT")
        row.slider=CreateFrame("Slider","SaureksClosetCapeMotion"..i,row,"OptionsSliderTemplate")
        row.slider:ClearAllPoints();row.slider:SetPoint("TOPLEFT",row,"TOPLEFT",14,-29);row.slider:SetWidth(268);row.slider:SetHeight(16)
        row.slider:SetMinMaxValues(0,200);row.slider:SetValueStep(5)
        for _,suffix in ipairs({"Low","High","Text"}) do local t=getglobal(row.slider:GetName()..suffix);if t then t:Hide() end end
        row.slider:SetScript("OnValueChanged",function()
            local amount=math.floor(this:GetValue()+.5);row.amount:SetText(amount.."%")
            if not V.capePhysicsRefreshing and this.closetEnabled then V.capePhysicsDraft[row.key]=amount end
        end)
        help(row.slider,entry[2],entry[3].." 100% is the original motion; 0% holds the lower cape at that animation's average pose while it follows your body. Click Apply when ready.")
    end
    self.capePhysicsStatus=label(f,"",38,345,288,27,true)
    self.capePhysicsDefault=settingsButton(f,"Default",38,378,138,function() V:ApplyCapePhysics(true) end)
    self.capePhysicsApply=settingsButton(f,"Apply",190,378,138,function() V:ApplyCapePhysics() end)
    help(self.capePhysicsDefault,"Default cape motion","Restore the original cape animations and reset every amount to 100%.")
    self:RefreshCapePhysics(true)
end
