local V=VanityStudio
function V:HaircraftAvailable()
    return type(SaureksClosetSetHaircraft)=="function"
end
function V:HatTuningAvailable()
    if not self:HaircraftAvailable() or not self:BagTuningAvailable() then return false end
    local ok,status=pcall(SaureksClosetGetBagFitDefaults,111,1,0)
    return ok and status==1
end
function V:SyncHaircraft()
    if not self:HaircraftAvailable() or self.sharingWorldPaused then return false end
    local c=VanityStudioCharacter
    local ok,status=pcall(SaureksClosetSetHaircraft,c.enabled and c.keepHairWithHat and 1 or 0)
    return ok and status==1
end
function V:SetHaircraft(keep)
    local c=VanityStudioCharacter;local previous=c.keepHairWithHat
    c.keepHairWithHat=keep and true or false
    if not self:SyncHaircraft() then
        c.keepHairWithHat=previous
        self:Message("Haircraft could not apply. Update SaureksCloset.dll and fully restart WoW.")
        self:RefreshHaircraftPage();return false
    end
    self:InvalidatePreviewModel(0,true)
    self:RefreshHaircraftPage();return true
end
function V:CreateHaircraftPage(p,label,button,enabled)
    p:SetFrameLevel(self.frame:GetFrameLevel()+12)
    label(p,"Haircraft",33,87,124,18)
    label(p,"Keep your hairstyle visible while wearing hats.",33,115,124,48,true)
    self.haircraftToggle=button(p,"Keep hair: Off",33,176,124,function()
        V:SetHaircraft(not VanityStudioCharacter.keepHairWithHat)
    end)
    self.haircraftToggle:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Keep hair with hats",1,1,1)
        GameTooltip:AddLine("Show your current hairstyle, including ponytails, with your equipped or cosmetic hat. This character setting also applies to the wardrobe preview. Some hairstyles may clip through closed helmets.",1,.82,0,true)
        GameTooltip:Show()
    end)
    self.haircraftToggle:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self.haircraftStatus=label(p,"",33,214,124,76,true)
    self.haircraftHat=button(p,"Adjust hat",33,300,124,function() V:OpenPlacementTuner(111) end)
    self.haircraftHat:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Adjust hat placement",1,1,1)
        GameTooltip:AddLine("Move, tilt or resize your hat to fit your hairstyle. Adjustments follow your head. Save Fit keeps the placement for this race and gender; Default restores the original fit.",1,.82,0,true);GameTooltip:Show()
    end)
    self.haircraftHat:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self.haircraftDefault=button(p,"Default hair",33,337,124,function() V:SetHaircraft(false) end)
    self.haircraftEnableControl=enabled
    self:RefreshHaircraftPage()
end
function V:RefreshHaircraftPage()
    if not self.haircraftToggle then return end
    local available=self:HaircraftAvailable();local c=VanityStudioCharacter
    self.haircraftToggle:SetText(c.keepHairWithHat and "Keep hair: On" or "Keep hair: Off")
    self.haircraftEnableControl(self.haircraftToggle,available)
    self.haircraftEnableControl(self.haircraftDefault,available)
    self.haircraftEnableControl(self.haircraftHat,self:HatTuningAvailable())
    self.haircraftStatus:SetText(not available and "Update the DLL and restart WoW to use Haircraft." or
        not c.enabled and "Enable the addon to see Haircraft." or
        c.keepHairWithHat and "Hair stays visible. Some hats may overlap your hair." or "Hats use their normal hair visibility.")
end
