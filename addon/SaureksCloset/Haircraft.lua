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
function V:CreateHaircraftPage(p,label,button,enabled,sheet,edit)
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
    self.haircraftMask=button(p,"Trim hair",33,337,124,function() V:OpenHairMask() end)
    self.haircraftMask:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Trim hair around your hat",1,1,1)
        GameTooltip:AddLine("Bake a reversible hair mask around the active transmogged hat. Adjust the crown bounds and preserve hanging hair below the cutoff. Fits are saved separately for each hat, race, gender and hairstyle.",1,.82,0,true);GameTooltip:Show()
    end)
    self.haircraftMask:SetScript("OnLeave",function() GameTooltip:Hide() end)
    if sheet then self:CreateHairMaskWindow(sheet,label,button,edit) end
    self.haircraftEnableControl=enabled
    self:RefreshHaircraftPage()
end
function V:RefreshHaircraftPage()
    if not self.haircraftToggle then return end
    local available=self:HaircraftAvailable();local c=VanityStudioCharacter
    self.haircraftToggle:SetText(c.keepHairWithHat and "Keep hair: On" or "Keep hair: Off")
    self.haircraftEnableControl(self.haircraftToggle,available)
    self.haircraftEnableControl(self.haircraftMask,available and type(SaureksClosetSetHairMask)=="function")
    self.haircraftEnableControl(self.haircraftHat,self:HatTuningAvailable())
    self.haircraftStatus:SetText(not available and "Update the DLL and restart WoW to use Haircraft." or
        not c.enabled and "Enable the addon to see Haircraft." or
        c.keepHairWithHat and "Hair stays visible. Use Trim hair to hide clipping around your hat." or "Hats use their normal hair visibility.")
end

local maskFields={
    {key="width",title="Crown width",min=30,max=150,default=90,tip="Side-to-side size of the cutting envelope, as a percentage of the estimated crown. Reduce it to hide more hair."},
    {key="depth",title="Crown depth",min=30,max=150,default=90,tip="Front-to-back size of the cutting envelope. Reduce it to hide more hair."},
    {key="top",title="Top height",min=10,max=150,default=95,tip="Top cutting plane, measured from the bottom of the hat as a percentage of its height. Hair above this plane is hidden."},
    {key="cutoff",title="Lower cutoff",min=0,max=90,default=35,tip="Hair below this height is preserved, including hanging hair and ponytails. Raise it to protect more hair. Must be below Top height."}
}
function V:HairMaskKey()
    local c=VanityStudioCharacter;local b=c.enabled and c.body or self:NativeBody()
    if not b then return nil end
    local id=c.enabled and c.selected and c.selected[1] or nil
    if self.draft and self.draft.slot==1 then id=self.draft.id end
    if id==nil then local link=GetInventoryItemLink("player",1);if link then local _,_,n=string.find(link,"item:(%d+)");id=tonumber(n) end end
    if not id or id<=0 then return nil end
    return b.race..":"..b.sex..":"..(b.hairStyle or 0)..":"..id
end
function V:HairMaskConfig(key)
    local saved=(VanityStudioCharacter.hairMasks or {})[key or ""] or {};local out={enabled=saved.enabled and true or false}
    for _,field in ipairs(maskFields) do
        local n=tonumber(saved[field.key]);if not n or n~=n then n=field.default end
        out[field.key]=math.floor(math.max(field.min,math.min(field.max,n))+.5)
    end
    if out.top<=out.cutoff then out.top=95 end
    return out
end
function V:SyncHairMask()
    if type(SaureksClosetSetHairMask)~="function" or self.sharingWorldPaused then return false end
    local key=self:HairMaskKey();local config=self:HairMaskConfig(key);local c=VanityStudioCharacter
    local active=key and c.enabled and c.keepHairWithHat and config.enabled
    local ok,status,count,generation=pcall(SaureksClosetSetHairMask,active and 1 or 0,config.width,config.depth,config.top,config.cutoff)
    self.hairMaskStatus=not ok and "Could not apply hair mask." or status==0 and "Waiting for the hat model..." or status==2 and "No visible hat to fit." or status==1 and (active and ((count or 0)>0 and "Hair mask applied." or "Mask applied; no hair crosses these bounds.") or "Hair mask is off.") or status==-3 and "Cannot estimate this crown. Try a different hat." or "Could not bake the mask. Check the DLL and model files."
    if generation and self.hairMaskGeneration~=generation then
        self.hairMaskGeneration=generation
        if self.InvalidatePreviewModel then self:InvalidatePreviewModel(0,true) end
    end
    if self.hairMaskWindow and self.hairMaskWindow:IsShown() then
        if self.hairMaskKey~=key then self:OpenHairMask() else self.hairMaskMessage:SetText(self.hairMaskStatus) end
    end
    self.hairMaskLastOK=ok and status and status>=0
    return self.hairMaskLastOK
end
function V:OpenHairMask()
    if not self.hairMaskWindow then return end
    self.hairMaskKey=self:HairMaskKey();self.hairMaskDraft=self:HairMaskConfig(self.hairMaskKey)
    self.hairMaskToggle:SetChecked(self.hairMaskDraft.enabled)
    for _,row in ipairs(self.hairMaskRows) do row.editor:SetText(self.hairMaskDraft[row.field.key]);row.editor:ClearFocus() end
    local available=self.hairMaskKey and type(SaureksClosetSetHairMask)=="function"
    self.haircraftEnableControl(self.hairMaskApply,available)
    self.haircraftEnableControl(self.hairMaskToggle,available)
    self.hairMaskMessage:SetText(not self.hairMaskKey and "Select a visible hat first." or self.hairMaskStatus or "Adjust the crown, then Apply. Saved for this hat and hairstyle.")
    self.hairMaskWindow:Show()
end
function V:ApplyHairMask()
    if self.hairMaskKey~=self:HairMaskKey() then self:OpenHairMask();return false end
    if not self.hairMaskKey or type(SaureksClosetSetHairMask)~="function" then return false end
    local config={enabled=self.hairMaskToggle:GetChecked() and true or false}
    for _,row in ipairs(self.hairMaskRows) do
        local n=tonumber(row.editor:GetText());local f=row.field
        if not n or n~=n or n<f.min or n>f.max then self.hairMaskMessage:SetText("Enter "..f.min.." to "..f.max.." for "..f.title..".");return false end
        config[f.key]=math.floor(n+.5)
    end
    if config.top<=config.cutoff then self.hairMaskMessage:SetText("Top height must be above Lower cutoff.");return false end
    local c=VanityStudioCharacter;c.hairMasks=c.hairMasks or {};local old=c.hairMasks[self.hairMaskKey];local oldKeep=c.keepHairWithHat
    c.hairMasks[self.hairMaskKey]=config
    if config.enabled then c.keepHairWithHat=true end
    if not self:SyncHaircraft() or self.hairMaskLastOK==false then
        local message=self.hairMaskStatus or "Could not apply hair mask."
        c.hairMasks[self.hairMaskKey]=old;c.keepHairWithHat=oldKeep;self:SyncHaircraft()
        self.hairMaskMessage:SetText(message);return false
    end
    self:RefreshHaircraftPage();return true
end
function V:CreateHairMaskWindow(sheet,label,button,edit)
    local f=sheet("SaureksClosetHairMask",UIParent,"Hair Mask");self.hairMaskWindow=f;f:Hide();f:SetFrameStrata("DIALOG");f:SetClampedToScreen(true)
    f:SetPoint("TOPLEFT",self.frame,"TOPRIGHT",-30,0);f:SetMovable(true);f:RegisterForDrag("LeftButton")
    f:SetScript("OnDragStart",function() this:StartMoving() end);f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
    f.close:SetScript("OnClick",function() f:Hide() end);table.insert(UISpecialFrames,f:GetName())
    self.hairMaskToggle=CreateFrame("CheckButton","SaureksClosetHairMaskToggle",f,"UICheckButtonTemplate")
    self.hairMaskToggle:ClearAllPoints();self.hairMaskToggle:SetPoint("TOPLEFT",f,"TOPLEFT",34,-81);self.hairMaskToggle:SetWidth(24);self.hairMaskToggle:SetHeight(24)
    label(f,"Trim hair around this hat",64,86,248,20,true)
    label(f,"Preserve hanging hair below the cutoff. Apply bakes and saves this fit.",38,118,286,38,true)
    self.hairMaskRows={}
    for i,field in ipairs(maskFields) do
        local y=171+(i-1)*40;local row={field=field};self.hairMaskRows[i]=row
        label(f,field.title,38,y+3,130,20,true)
        row.editor=edit(f,"SaureksClosetHairMaskValue"..i,207,y,62,4)
        -- Lua 5.0 exhausts generic-for variables. Deferred callbacks use the
        -- stable per-row field, never the outer loop's `field` upvalue.
        row.minus=button(f,"-",176,y,24,function() local n=tonumber(row.editor:GetText()) or row.field.default;row.editor:SetText(math.max(row.field.min,n-1)) end)
        row.plus=button(f,"+",282,y,24,function() local n=tonumber(row.editor:GetText()) or row.field.default;row.editor:SetText(math.min(row.field.max,n+1)) end)
        row.editor:SetScript("OnEnterPressed",function() V:ApplyHairMask();row.editor:ClearFocus() end)
        row.editor:SetScript("OnEscapePressed",function() V:OpenHairMask() end)
        for _,control in ipairs({row.editor,row.minus,row.plus}) do
            control:SetScript("OnEnter",function() GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText(row.field.title.." (%)");GameTooltip:AddLine(row.field.tip,1,.82,0,true);GameTooltip:Show() end)
            control:SetScript("OnLeave",function() GameTooltip:Hide() end)
        end
    end
    self.hairMaskMessage=label(f,"",38,334,286,40,true)
    self.hairMaskApply=button(f,"Apply",206,384,112,function() V:ApplyHairMask() end)
end
