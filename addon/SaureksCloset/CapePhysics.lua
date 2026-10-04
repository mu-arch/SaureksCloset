local V=VanityStudio
local controls={
    {"walk","Walking motion","Adjust lower-cape movement during walking and backward walking."},
    {"run","Running motion","Adjust lower-cape movement during running."},
    {"idle","Idle motion","Adjust lower-cape movement while standing."},
    {"air","Airborne motion","Adjust lower-cape movement during jumping, falling and landing."}
}
local advancedControls={
    {key="forward",title="Forward/back swing",default=100,min=0,max=200,tip="Motion away from and toward your back. Lower this to reduce the human cape's repeated flapping."},
    {key="side",title="Side-to-side swing",default=100,min=0,max=200,tip="Motion across your back, from left to right."},
    {key="twist",title="Twist",default=100,min=0,max=200,tip="Rotation around the cape's vertical axis."},
    {key="lean",title="Resting lean",default=0,min=-30,max=30,tip="Degrees of lower-cape lean. Positive leans away from your back; negative leans toward it. The upper attachment stays in place."},
    {key="smooth",title="Smoothing",default=0,min=0,max=100,tip="Round off abrupt changes of direction in the existing animation. Higher values make reversals softer without speeding up or slowing down the character."},
    {key="tip",title="Hem motion",default=100,min=0,max=200,tip="Extra control over the cape's bottom joint. Reduce it for a quieter hem without reducing the whole cape's movement."}
}
function V:CapePhysicsAvailable()
    return self:PhysicsControlsAvailable(4) and type(SaureksClosetSetCapeMotion)=="function"
end
function V:CapePhysicsConfig()
    local saved=VanityStudioCharacter.capeMotion or {};local config={enabled=saved.enabled and true or false}
    for _,entry in ipairs(controls) do
        local n=tonumber(saved[entry[1]]) or 100;if n~=n then n=100 end
        config[entry[1]]=math.floor(math.max(0,math.min(200,n))+.5)
    end
    for _,entry in ipairs(advancedControls) do
        local n=tonumber(saved[entry.key]);if not n or n~=n then n=entry.default end
        config[entry.key]=math.floor(math.max(entry.min,math.min(entry.max,n))+.5)
    end
    return config
end
function V:SyncCapePhysics()
    if not self:CapePhysicsAvailable() or self.sharingWorldPaused then return false end
    local config=self:CapePhysicsConfig()
    local ok,status=pcall(SaureksClosetSetCapeMotion,VanityStudioCharacter.enabled and config.enabled and 1 or 0,config.walk,config.run,config.idle,config.air,config.forward,config.side,config.twist,config.lean+30,config.smooth,config.tip)
    return ok and status==1
end
function V:ApplyCapePhysics(reset)
    if not self:CapePhysicsAvailable() then return false end
    if not reset and not self:CommitCapeAdvancedEditors() then return false end
    local config=reset and {enabled=false,walk=100,run=100,idle=100,air=100,forward=100,side=100,twist=100,lean=0,smooth=0,tip=100} or self.capePhysicsDraft
    if not config then return false end
    local previous=VanityStudioCharacter.capeMotion;VanityStudioCharacter.capeMotion=config
    if not self:SyncCapePhysics() then
        VanityStudioCharacter.capeMotion=previous
        self.capePhysicsStatus:SetText("Could not apply. Check the DLL and cape files.");self.capeAdvancedStatus:SetText("Could not apply. Check the DLL and cape files.");return false
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
    for _,row in ipairs(self.capeAdvancedRows or {}) do
        local editor=row.editor
        if reset or not available or not config.enabled or editor.draft~=config then editor.editing=nil;editor:ClearFocus() end
        if not editor.editing then editor:SetText(config[row.control.key]) end
        editor.closetEnabled=available and config.enabled;editor:EnableMouse(editor.closetEnabled);editor:SetAlpha(editor.closetEnabled and 1 or .4)
        self.physicsEnable(row.minus,editor.closetEnabled);self.physicsEnable(row.plus,editor.closetEnabled)
    end
    if self.capeAdvancedApply then
        self.physicsEnable(self.capeAdvancedApply,available);self.physicsEnable(self.capeAdvancedDefault,available);self.physicsEnable(self.capeSoften,available)
        if reset then self.capeAdvancedStatus:SetText("");self.capeAdvancedInvalid=nil end
    end
    self.capePhysicsRefreshing=nil
end
function V:CreateCapePhysicsWindow(window,section,label,settingsButton,enable,help,edit,redButton)
    self.capePhysicsWindow=window("SaureksClosetCapePhysics","Cape Physics")
    local f=self.capePhysicsWindow
    self.capePhysicsToggle=CreateFrame("CheckButton","SaureksClosetCapeMotionToggle",f,"UICheckButtonTemplate")
    self.capePhysicsToggle:ClearAllPoints();self.capePhysicsToggle:SetPoint("TOPLEFT",f,"TOPLEFT",33,-80);self.capePhysicsToggle:SetWidth(24);self.capePhysicsToggle:SetHeight(24)
    label(f,"Custom motion",63,85,154,20,true)
    redButton(f,"Advanced",232,80,96,function() V:RefreshCapePhysics();V.capePhysicsWindow:Hide();V.capeAdvancedWindow:Show() end)
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
    self:CreateCapeAdvancedWindow(window,section,label,settingsButton,help,edit,redButton)
    self:RefreshCapePhysics(true)
end

function V:CommitCapeAdvancedEditor(editor)
    if self.capePhysicsRefreshing or not editor.editing then return true end
    editor.editing=nil
    local value=tonumber(editor:GetText());local field=editor.control
    local ok=editor.closetEnabled and editor.draft==self.capePhysicsDraft and value and value==value and value>=field.min and value<=field.max
    if ok then self.capePhysicsDraft[field.key]=math.floor(value+.5) end
    self.capeAdvancedInvalid=not ok
    self.capeAdvancedStatus:SetText(ok and "" or "Enter a value from "..field.min.." to "..field.max..".")
    self:RefreshCapePhysics();return ok and true or false
end
function V:CommitCapeAdvancedEditors()
    for _,row in ipairs(self.capeAdvancedRows or {}) do if row.editor.editing then
        local ok=self:CommitCapeAdvancedEditor(row.editor);row.editor:ClearFocus()
        if not ok then self.capeAdvancedInvalid=nil;return false end
    end end
    if self.capeAdvancedInvalid then self.capeAdvancedInvalid=nil;return false end
    return true
end
function V:CapeAdvancedPreset(soft)
    if not self:CapePhysicsAvailable() then return end
    for _,row in ipairs(self.capeAdvancedRows) do row.editor.editing=nil;row.editor:ClearFocus() end
    local preset=soft and {forward=55,side=85,twist=60,lean=5,smooth=65,tip=60} or {forward=100,side=100,twist=100,lean=0,smooth=0,tip=100}
    for key,value in pairs(preset) do self.capePhysicsDraft[key]=value end
    if soft then self.capePhysicsDraft.enabled=true end
    self.capeAdvancedInvalid=nil;self.capeAdvancedStatus:SetText("");self:RefreshCapePhysics()
end
function V:CreateCapeAdvancedWindow(window,section,label,settingsButton,help,edit,redButton)
    local f=window("SaureksClosetCapeAdvanced","Cape Motion");self.capeAdvancedWindow=f
    redButton(f,"Back",34,80,82,function()
        if not V:CommitCapeAdvancedEditors() then return end
        f:Hide();V.capePhysicsWindow:Show();V:RefreshCapePhysics()
    end)
    self.capeAdvancedRows={}
    for i,control in ipairs(advancedControls) do
        local row=section(f,34,116+(i-1)*37,296,32,false);row.control=control;self.capeAdvancedRows[i]=row
        local title=label(row,control.title,9,7,142,20,true);title:SetFont("Fonts\\FRIZQT__.TTF",10)
        row.editor=edit(row,"SaureksClosetCapeAdvancedValue"..i,184,8,55,16);row.editor.control=control;row.editor:SetJustifyH("CENTER")
        label(row,control.key=="lean" and "deg" or "%",242,8,15,18,true)
        row.editor:SetScript("OnEditFocusGained",function() this.editing=true;this.draft=V.capePhysicsDraft end)
        row.editor:SetScript("OnEditFocusLost",function() V:CommitCapeAdvancedEditor(this) end)
        row.editor:SetScript("OnEnterPressed",function() V:CommitCapeAdvancedEditor(this);this:ClearFocus() end)
        row.editor:SetScript("OnEscapePressed",function() this.editing=nil;this:ClearFocus();V:RefreshCapePhysics() end)
        help(row.editor,control.title,control.tip.." Click Apply when ready.")
        local function nudge(x,text,direction)
            local b=section(row,x,5,22,22,true,"Button",.75);b.control=control;b.direction=direction
            local t=label(b,text,0,1,22,20,true);t:SetFont("Fonts\\FRIZQT__.TTF",14);t:SetJustifyH("CENTER");t:SetJustifyV("MIDDLE")
            b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
            b:SetScript("OnClick",function()
                if not this.closetEnabled or not V:CommitCapeAdvancedEditors() then return end
                local field=this.control;local step=IsShiftKeyDown and IsShiftKeyDown() and 10 or 1
                V.capePhysicsDraft[field.key]=math.max(field.min,math.min(field.max,V.capePhysicsDraft[field.key]+this.direction*step));V:RefreshCapePhysics()
            end)
            help(b,control.title,control.tip.." Click for 1; Shift-click for 10.");return b
        end
        row.minus=nudge(153,"-",-1);row.plus=nudge(263,"+",1)
    end
    self.capeAdvancedStatus=label(f,"",38,345,288,27,true)
    self.capeSoften=settingsButton(f,"Soften",30,378,96,function() V:CapeAdvancedPreset(true) end)
    self.capeAdvancedDefault=settingsButton(f,"Default",134,378,96,function() V:CapeAdvancedPreset(false) end)
    self.capeAdvancedApply=settingsButton(f,"Apply",238,378,96,function() V:ApplyCapePhysics() end)
    help(self.capeSoften,"Soft motion","Reduce forward/back flapping and hem motion, soften reversals, and add a slight outward lean. Your walking, running, idle and airborne amounts stay unchanged. Click Apply to use it.")
    help(self.capeAdvancedDefault,"Default directions","Restore native direction, resting lean, smoothing and hem settings. Your activity amounts stay unchanged. Click Apply to use it.")
end
