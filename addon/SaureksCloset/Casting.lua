local V=VanityStudio
function V:CastingAvailable()
    if type(SaureksClosetCastingVersion)~="function" or type(SaureksClosetSetCasting)~="function" then return false end
    local ok,version=pcall(SaureksClosetCastingVersion)
    return ok and type(version)=="number" and version>=1
end
function V:CastingConfig()
    local c=VanityStudioCharacter.casting
    if type(c)~="table" then c={} end
    return {enabled=c.enabled==true,effects=c.effects~=false,swing=c.swing~=false}
end
function V:CastingWeapon()
    local c=VanityStudioCharacter
    if c.selected and c.selected[16]==0 then return 0 end
    local real=self:RealWeaponItems()
    -- A cosmetic carried item is separate from the weapon actually drawn.
    -- An appearance cannot create a held weapon in an empty equipment slot.
    if real[1]==0 then return 0 end
    local weapons=self:EffectiveWeapons(c.weapons,c.selected)
    local id=weapons[108] or real[1]
    local asset=SaureksClosetWeaponAssets[id]
    if not asset or (asset[1]~=1 and asset[1]~=2) then return 0 end
    return id
end
function V:SyncCasting()
    if not self:CastingAvailable() or self.sharingWorldPaused then return false end
    local config=self:CastingConfig()
    local ok,status=pcall(SaureksClosetSetCasting,VanityStudioCharacter.enabled and config.enabled and 1 or 0,
        config.effects and 1 or 0,config.swing and 1 or 0,self:CastingWeapon())
    return ok and status==1
end
function V:SetCasting(key,value)
    if key~="enabled" and key~="effects" and key~="swing" then return false end
    local c=VanityStudioCharacter;local previous=c.casting
    local config=self:CastingConfig();local changed=config[key]~=(value and true or false)
    config[key]=value and true or false
    if changed then
        if not config.enabled and config.effects and config.swing then c.casting=nil
        else c.casting=config end
    end
    if not self:SyncCasting() then
        c.casting=previous
        self:Message("Casting could not apply. Update SaureksCloset.dll and fully restart WoW.")
        self:RefreshCastingPage();return false
    end
    if changed and self.LookSettingsChanged then self:LookSettingsChanged() end
    self:RefreshCastingPage();return true
end
function V:CreateCastingPage(p,label,button,enabled)
    self.castingEnableControl=enabled
    label(p,"Casting",38,84,284,24)
    label(p,"Channel spells through your main-hand weapon, then swing it to release the spell.",38,118,284,48,true)
    self.castingToggle=button(p,"Enable weapon casting",38,184,284,function()
        V:SetCasting("enabled",not V:CastingConfig().enabled)
    end)
    local function help(control,title,text)
        control:SetScript("OnEnter",function()
            GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText(title,1,1,1)
            GameTooltip:AddLine(text,1,.82,0,true);GameTooltip:Show()
        end)
        control:SetScript("OnLeave",function() GameTooltip:Hide() end)
    end
    help(self.castingToggle,"Weapon casting","Use your visible main-hand sword, staff or other melee weapon for spell casting. Cosmetic only: spell timing, targets and damage stay unchanged. Ranged attacks keep their normal animations. Use the main Save button to save these choices with your look.")
    self.castingChecks={}
    local choices={
        {"effects","Casting effects on weapon",230,"Move hand casting effects to the end of your weapon. Spells with a projectile also launch from the weapon. Spells without casting effects do not gain new effects."},
        {"swing","Swing weapon on release",274,"Hold your weapon ready while charging and start a weapon swing when the spell releases. Channeled spells keep a steady ready pose. Turning this off keeps the normal casting motion."}
    }
    for _,choice in ipairs(choices) do
        local key,title,y,tip=choice[1],choice[2],choice[3],choice[4]
        local check=CreateFrame("CheckButton","SaureksClosetCasting"..key,p,"UICheckButtonTemplate")
        check:ClearAllPoints();check:SetPoint("TOPLEFT",p,"TOPLEFT",38,-y)
        check:SetWidth(24);check:SetHeight(24)
        label(p,title,68,y+3,254,20,true)
        check:SetScript("OnClick",function() V:SetCasting(key,this:GetChecked()) end)
        help(check,title,tip);self.castingChecks[key]=check
    end
    self.castingStatus=label(p,"",38,322,284,56,true)
    self:RefreshCastingPage()
end
function V:RefreshCastingPage()
    if not self.castingToggle then return end
    local available=self:CastingAvailable();local config=self:CastingConfig()
    self.castingToggle:SetText(config.enabled and "Disable weapon casting" or "Enable weapon casting")
    self.castingToggle.closetToggledOff=not config.enabled
    self.castingEnableControl(self.castingToggle,available)
    for key,check in pairs(self.castingChecks) do
        check:SetChecked(config[key]);self.castingEnableControl(check,available and config.enabled)
    end
    local status="Normal casting."
    if not available then status="Update SaureksCloset.dll and fully restart WoW to use weapon casting."
    elseif config.enabled and not VanityStudioCharacter.enabled then status="Enable the addon to use weapon casting."
    elseif config.enabled and self:CastingWeapon()==0 then status="Equip a visible main-hand melee weapon. Normal casting is used until one is available."
    elseif config.enabled then status="Weapon casting enabled. Uses your equipped main-hand appearance, including transmog." end
    self.castingStatus:SetText(status)
end
