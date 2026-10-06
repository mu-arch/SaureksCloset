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
    if ok and status==1 then self:SyncHairMask() end
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
        GameTooltip:AddLine("Show your current hairstyle, including ponytails, with your equipped or cosmetic hat. This character setting also applies to the wardrobe preview. Covered hair is fitted automatically while skin and exposed lower hair are preserved. Hats that sit inside the head may still need Adjust hat.",1,.82,0,true)
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
    self.haircraftEnableControl=enabled
    self:RefreshHaircraftPage()
end
function V:RefreshHaircraftPage()
    if not self.haircraftToggle then return end
    local available=self:HaircraftAvailable();local c=VanityStudioCharacter
    self.haircraftToggle:SetText(c.keepHairWithHat and "Keep hair: On" or "Keep hair: Off")
    self.haircraftEnableControl(self.haircraftToggle,available)
    self.haircraftEnableControl(self.haircraftHat,self:HatTuningAvailable())
    self.haircraftStatus:SetText(not available and "Update the DLL and restart WoW to use Haircraft." or
        not c.enabled and "Enable the addon to see Haircraft." or
        c.keepHairWithHat and (self.hairMaskStatus or "Fitting hair around your hat...") or "Hats use their normal hair visibility.")
end

function V:HairMaskAvailable()
    if type(SaureksClosetSetHairMask)~="function" or type(SaureksClosetHairMaskVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetHairMaskVersion);return ok and type(version)=="number" and version>=3
end
function V:HairMaskKey()
    local c=VanityStudioCharacter;local b=c.enabled and c.body or self:NativeBody()
    if not b then return nil end
    local id=c.enabled and c.selected and c.selected[1] or nil
    if self.draft and self.draft.slot==1 then id=self.draft.id end
    if id==nil then local link=GetInventoryItemLink("player",1);if link then local _,_,n=string.find(link,"item:(%d+)");id=tonumber(n) end end
    if not id or id<=0 then return nil end
    return b.race..":"..b.sex..":"..(b.hairStyle or 0)..":"..id
end
function V:SyncHairMask()
    if self.sharingWorldPaused then return false end
    if not self:HairMaskAvailable() then
        -- Stop an already-loaded destructive bake with an older DLL too.
        if type(SaureksClosetSetHairMask)=="function" then pcall(SaureksClosetSetHairMask,0,90,90,95,35) end
        self.hairMaskStatus="Update the DLL and restart WoW for automatic hair fitting."
        return false
    end
    local c=VanityStudioCharacter
    -- Old per-hat plane settings intentionally have no effect. Keep hair is
    -- the only switch; fitting follows the current hat and saved hat placement.
    local active=self:HairMaskKey() and c.enabled and c.keepHairWithHat
    local ok,status,count,generation=pcall(SaureksClosetSetHairMask,active and 1 or 0,3)
    self.hairMaskStatus=not ok and "Could not fit hair." or status==0 and "Waiting for the hat model..." or status==2 and "No visible hat to fit." or status==3 and "Loading the fitted hairstyle..." or status==4 and "Hat surface unavailable. Hair is unchanged." or status==1 and (active and ((count or 0)>0 and "Hair fitted beneath your hat." or "Hair preserved. No safe fit needed or available.") or "Hats use their normal hair visibility.") or status==-6 and "The fitted model did not load. Restart WoW with the updated DLL." or "Could not fit hair. Original hair preserved."
    if generation and self.hairMaskGeneration~=generation then
        self.hairMaskGeneration=generation
        if self.InvalidatePreviewModel then self:InvalidatePreviewModel(0,true) end
    end
    self:RefreshHaircraftPage()
    self.hairMaskLastOK=ok and status and status>=0
    return self.hairMaskLastOK
end
