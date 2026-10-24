local V=VanityStudio
V.slotNames=V.slotNames or {};V.slotNames[112]="Second hat"
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
    if (previous and true or false)~=c.keepHairWithHat and self.LookSettingsChanged then self:LookSettingsChanged() end
    self:InvalidatePreviewModel(0,true)
    self:RefreshHaircraftPage();return true
end
function V:SetHairTrimming(trim)
    local c=VanityStudioCharacter;local previous=c.trimHair
    c.trimHair=trim and true or false
    if not self:SyncHairMask() then
        local message=self.hairMaskStatus or "Could not change hair trimming."
        c.trimHair=previous;self:SyncHairMask()
        self:Message(message);self:RefreshHaircraftPage();return false
    end
    self:RefreshHaircraftPage();return true
end
function V:CreateHaircraftPage(p,label,button,enabled)
    p:SetFrameLevel(self.frame:GetFrameLevel()+12)
    label(p,"Haircraft",33,87,124,18)
    label(p,"Keep your hairstyle visible while wearing hats.",33,115,124,48,true)
    self.haircraftToggle=button(p,"Toggle Hair",33,176,124,function()
        V:SetHaircraft(not VanityStudioCharacter.keepHairWithHat)
    end)
    self.haircraftToggle:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Toggle Hair",1,1,1)
        GameTooltip:AddLine("Show your current hairstyle, including ponytails, with your equipped or cosmetic hat. This setting also applies to the wardrobe preview. Use the main Save button to keep it with this look. Enable Trim hair to automatically fit covered hair while preserving skin and exposed lower hair. Hats that sit inside the head may still need Adjust hat.",1,.82,0,true)
        GameTooltip:Show()
    end)
    self.haircraftToggle:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self.haircraftStatus=label(p,"It's normal for hats to clip when getting started. Don't give up, adjust the position and fit. Most hats can be made to look okay.",33,214,124,100,true)
    self.haircraftHat=button(p,"Adjust hat",33,324,124,function() V:OpenPlacementTuner(111) end)
    self.haircraftHat:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Adjust hat placement",1,1,1)
        GameTooltip:AddLine("Move, tilt or resize your hat to fit your hairstyle. Adjustments follow your head. Save keeps the placement in this look; Default restores the original fit for this look only.",1,.82,0,true);GameTooltip:Show()
    end)
    self.haircraftHat:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self.haircraftTrim=CreateFrame("CheckButton","SaureksClosetHaircraftTrim",p,"UICheckButtonTemplate")
    self.haircraftTrim:ClearAllPoints();self.haircraftTrim:SetPoint("TOPLEFT",p,"TOPLEFT",33,-356)
    self.haircraftTrim:SetWidth(24);self.haircraftTrim:SetHeight(24)
    label(p,"Trim hair",61,360,96,20,true)
    self.haircraftTrim:SetScript("OnClick",function() V:SetHairTrimming(this:GetChecked()) end)
    self.haircraftTrim:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Trim hair",1,1,1)
        if V.hairMaskStatus then GameTooltip:AddLine(V.hairMaskStatus,1,.82,0,true) end
        GameTooltip:AddLine("Automatically fit hair beneath your hat when Toggle Hair is on. Turn this off to show your full, untrimmed hairstyle. Saved for this character.",1,.82,0,true);GameTooltip:Show()
    end)
    self.haircraftTrim:SetScript("OnLeave",function() GameTooltip:Hide() end)
    self.haircraftEnableControl=enabled
    self:RefreshHaircraftPage()
end
function V:RefreshHaircraftPage()
    if not self.haircraftToggle then return end
    local available=self:HaircraftAvailable();local c=VanityStudioCharacter
    self.haircraftToggle:SetText("Toggle Hair")
    self.haircraftToggle.closetToggledOff=not c.keepHairWithHat
    self.haircraftEnableControl(self.haircraftToggle,available)
    self.haircraftEnableControl(self.haircraftHat,self:HatTuningAvailable())
    self.haircraftTrim:SetChecked(c.trimHair~=false)
    self.haircraftEnableControl(self.haircraftTrim,available and self:HairMaskAvailable())

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
    -- Preserve the previous automatic behavior until this character chooses
    -- otherwise. Old per-hat cutting-plane settings remain unused.
    local active=self:HairMaskKey() and c.enabled and c.keepHairWithHat and c.trimHair~=false
    local ok,status,count,generation=pcall(SaureksClosetSetHairMask,active and 1 or 0,3)
    self.hairMaskStatus=not ok and "Could not fit hair." or status==0 and "Waiting for the hat model..." or status==2 and "No visible hat to fit." or status==3 and "Loading the fitted hairstyle..." or status==4 and "Hat surface unavailable. Hair is unchanged." or status==1 and (active and ((count or 0)>0 and "Hair fitted beneath your hat." or "Hair preserved. No safe fit needed or available.") or (c.keepHairWithHat and (c.trimHair==false and "Hair stays visible. Trimming is off." or "No visible hat to fit.") or "Hats use their normal hair visibility.")) or status==-6 and "The fitted model did not load. Restart WoW with the updated DLL." or "Could not fit hair. Original hair preserved."
    if generation and self.hairMaskGeneration~=generation then
        self.hairMaskGeneration=generation
        if self.InvalidatePreviewModel then self:InvalidatePreviewModel(0,true) end
    end
    self:RefreshHaircraftPage()
    self.hairMaskLastOK=ok and status and status>=0
    return self.hairMaskLastOK
end

function V:SecondHatAvailable()
    return type(SaureksClosetSetSecondHat)=="function" and type(SaureksClosetSecondHatVersion)=="function"
end
function V:SecondHatSelection()
    if self.draft and self.draft.slot==112 then return self.draft.id end
    return VanityStudioCharacter.enabled and VanityStudioCharacter.secondHat or nil
end
function V:SyncSecondHat(token,id)
    if self.sharingWorldPaused then return false end
    if id==nil then id=self:SecondHatSelection() end
    if not self:SecondHatAvailable() then return not id or id==0,1 end
    local item=id and self.index[id]
    if id and id~=0 and (not item or item[3]~=1) then return false,-2 end
    local ok,status=pcall(SaureksClosetSetSecondHat,token or 0,item and item[8] or 0)
    return ok and type(status)=="number" and status>=0,status
end
function V:SetSecondHat(id)
    if id==0 then id=nil end
    if not self:SecondHatAvailable() or not self:Compatible(id,112) then return false end
    self:CancelDraft()
    local c=VanityStudioCharacter;local previous=c.secondHat;c.secondHat=id
    if not self:SyncSecondHat() then
        c.secondHat=previous;self:SyncSecondHat()
        self:Message("This second hat could not load. Update the DLL and fully restart WoW if needed.");return false
    end
    if previous~=id then self:LookSettingsChanged() end
    self:Refresh();return true
end
