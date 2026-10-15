-- Cosmetic mount replacement via VanillaHelpers. Only the local player's
-- mounted appearance is overridden. Mount spells, movement and speed stay native.
local V=VanityStudio
local function iconKey(path)
    return type(path)=="string" and string.lower(string.gsub(path,"%.[bB][lL][pP]$","")) or ""
end
function V:MountCatalogIndex()
    if not self.mountCatalogByID then
        self.mountCatalogByID={}
        for _,mount in ipairs(VanityStudioMountCatalog) do self.mountCatalogByID[mount.id]=mount end
    end
    return self.mountCatalogByID
end
function V:MountsAvailable()
    return type(SetUnitMountDisplayID)=="function" and type(UnitDisplayInfo)=="function"
        and type(GetPlayerBuff)=="function" and type(GetPlayerBuffTexture)=="function"
end
function V:MountAuraActive()
    if not self:MountsAvailable() then return false end
    for i=0,31 do
        local index=GetPlayerBuff(i,"HELPFUL")
        if not index or index<0 then break end
        local texture=GetPlayerBuffTexture(index)
        if VanityStudioMountAuraIcons[iconKey(texture)] then return true end
        -- Some event mounts use a shared icon. The exact aura name disambiguates
        -- those without mistaking movement buffs for being mounted.
        if not texture or VanityStudioMountCandidateIcons[iconKey(texture)] then
            if not self.mountAuraTip then
                self.mountAuraTip=CreateFrame("GameTooltip","SaureksClosetMountAuraTip",UIParent,"GameTooltipTemplate")
            end
            local tip=self.mountAuraTip
            tip:SetOwner(UIParent,"ANCHOR_NONE");tip:ClearLines();tip:SetPlayerBuff(index)
            local line=getglobal("SaureksClosetMountAuraTipTextLeft1")
            local name=line and line:GetText();tip:Hide()
            if name and VanityStudioMountAuras[name] then return true end
        end
    end
    return false
end
function V:ReleaseMount()
    if not (self.mountApplied or VanityStudioCharacter.mountManaged) then return true end
    if type(SetUnitMountDisplayID)~="function" then return false end
    local ok,value=pcall(SetUnitMountDisplayID,"player")
    if not ok or value==false then self.mountError="Could not restore your original mount. Retrying.";return false end
    self.mountApplied=nil;VanityStudioCharacter.mountManaged=nil;self.mountError=nil
    return true
end
function V:SyncMount()
    local c=VanityStudioCharacter
    if not c or self.mountSyncing then return end
    self.mountSyncing=true
    local entry=c.mount and self:MountCatalogIndex()[c.mount]
    if self.mountSuspended or not c.enabled or not entry or not self:MountsAvailable() or
        (UnitOnTaxi and UnitOnTaxi("player")) or (UnitIsDeadOrGhost and UnitIsDeadOrGhost("player")) or not self:MountAuraActive() then
        self:ReleaseMount()
    else
        local ok,_,_,display=pcall(UnitDisplayInfo,"player")
        if ok and type(display)=="number" and display>0 then
            if self.mountApplied~=entry.id or display~=entry.id then
                local applied,value=pcall(SetUnitMountDisplayID,"player",entry.id)
                if applied and value~=false then
                    self.mountApplied=entry.id;c.mountManaged=true;self.mountError=nil
                else self.mountError="Could not apply this mount appearance. Retrying." end
            end
        end
    end
    self.mountSyncing=nil
end
function V:SetMountAppearance(id)
    if id~=nil and (type(id)~="number" or not self:MountCatalogIndex()[id]) then return false end
    if id and not self:MountsAvailable() then self:Message("Update VanillaHelpers.dll and fully restart WoW to change mount appearances.");return false end
    if VanityStudioCharacter.mount==id then return true end
    VanityStudioCharacter.mount=id
    self:LookSettingsChanged();self:SyncMount();self:Refresh()
    return true
end
function V:FilterMounts(query)
    query=string.lower(string.gsub(query or "","^%s*(.-)%s*$","%1"))
    local result={}
    for _,mount in ipairs(VanityStudioMountCatalog) do
        if query=="" or string.find(string.lower(mount.name),query,1,true) then table.insert(result,mount) end
    end
    return result
end
function V:RefreshMountsPage()
    if not self.mountButtons then return end
    local available=self:MountsAvailable()
    local selected=self:MountCatalogIndex()[VanityStudioCharacter.mount]
    self.mountCurrent:SetText(selected and selected.name or "Original mount")
    self.mountEnable(self.mountOriginal,VanityStudioCharacter.mount~=nil)
    self.mountStatus:SetText(not available and "Update VanillaHelpers.dll to change mounts." or self.mountError or "")
    local matches=self:FilterMounts(self.mountSearch:GetText())
    local pageSize=table.getn(self.mountButtons)
    local pages=math.max(1,math.ceil(table.getn(matches)/pageSize))
    self.mountPage=math.max(1,math.min(self.mountPage or 1,pages))
    for i,b in ipairs(self.mountButtons) do
        local mount=matches[(self.mountPage-1)*pageSize+i]
        b.mount=mount
        if mount then
            b.icon:SetTexture(mount.icon);b:Show();b:SetAlpha(available and 1 or .4);self.mountEnable(b,available)
            if selected and selected.id==mount.id then b.selected:Show() else b.selected:Hide() end
        else b:Hide() end
    end
    self.mountPageLabel:SetText(self.mountPage.." / "..pages)
    self.mountEmpty:SetText(table.getn(matches)==0 and "No mounts match your search." or "")
    self.mountEnable(self.mountPrevious,self.mountPage>1);self.mountEnable(self.mountNext,self.mountPage<pages)
end
function V:CreateMountsPage(p,section,label,edit,button,enable,arrow)
    self.mountEnable=enable
    label(p,"Mounts",38,84,174,24)
    self.mountOriginal=button(p,"Original",240,80,90,function() V:SetMountAppearance(nil) end)
    self.mountCurrent=label(p,"",38,114,292,22)
    self.mountSearch=edit(p,"SaureksClosetMountSearch",43,144,282,70)
    local hint=label(p,"Search mounts",48,149,270,18,true)
    self.mountSearchHint=hint
    local function refreshHint()
        if V.mountSearch:GetText()=="" and not V.mountSearch.closetFocused then hint:Show() else hint:Hide() end
    end
    -- Vanilla EditBoxes have focus events, but no HasFocus() method.
    self.mountSearch:SetScript("OnEditFocusGained",function() this.closetFocused=true;refreshHint() end)
    self.mountSearch:SetScript("OnEditFocusLost",function() this.closetFocused=nil;refreshHint() end)
    self.mountSearch:SetScript("OnHide",function() this.closetFocused=nil;refreshHint() end)
    self.mountSearch:SetScript("OnTextChanged",function() refreshHint();V.mountPage=1;V:RefreshMountsPage() end)
    self.mountSearch:SetScript("OnEnterPressed",function() this:ClearFocus() end)
    refreshHint()
    self.mountButtons={}
    for i=1,24 do
        local column=math.mod(i-1,6);local row=math.floor((i-1)/6)
        local b=CreateFrame("Button","SaureksClosetMount"..i,p)
        b:SetWidth(42);b:SetHeight(42);b:SetPoint("TOPLEFT",p,"TOPLEFT",37+column*49,-(175+row*42))
        b.icon=b:CreateTexture(nil,"ARTWORK");b.icon:SetPoint("CENTER",b,"CENTER",0,0);b.icon:SetWidth(36);b.icon:SetHeight(36)
        local border=b:CreateTexture(nil,"OVERLAY");border:SetTexture("Interface\\Buttons\\UI-Quickslot2");border:SetAllPoints(b)
        b.selected=b:CreateTexture(nil,"OVERLAY");b.selected:SetTexture("Interface\\Buttons\\CheckButtonHilight");b.selected:SetAllPoints(b);b.selected:SetBlendMode("ADD")
        b:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square","ADD")
        b:SetScript("OnClick",function() if this.closetEnabled and this.mount then V:SetMountAppearance(this.mount.id) end end)
        b:SetScript("OnEnter",function()
            if not this.mount then return end
            GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText(this.mount.name,1,.82,0)
            GameTooltip:AddLine("Use this appearance when you mount up. Saved with your current look.",1,1,1,true)
            if V:MountCatalogIndex()[VanityStudioCharacter.mount]==this.mount then GameTooltip:AddLine("Selected",.2,1,.2) end
            GameTooltip:Show()
        end)
        b:SetScript("OnLeave",function() GameTooltip:Hide() end)
        self.mountButtons[i]=b
    end
    self.mountEmpty=label(p,"",38,230,292,40,true)
    self.mountStatus=label(p,"",38,344,292,14,true);self.mountStatus:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.mountPrevious=arrow(p,"Prev",84,358,function() V.mountPage=(V.mountPage or 1)-1;V:RefreshMountsPage() end)
    self.mountPageLabel=label(p,"",135,366,100,18,true);self.mountPageLabel:SetJustifyH("CENTER")
    self.mountNext=arrow(p,"Next",249,358,function() V.mountPage=(V.mountPage or 1)+1;V:RefreshMountsPage() end)
    p:EnableMouseWheel(true)
    p:SetScript("OnMouseWheel",function() V.mountPage=(V.mountPage or 1)+(arg1>0 and -1 or 1);V:RefreshMountsPage() end)
    self:RefreshMountsPage()
end
function V:InitializeMounts()
    self:MountCatalogIndex()
    if self.mountEvents then return end
    local f=CreateFrame("Frame");self.mountEvents=f
    for _,event in ipairs({"PLAYER_AURAS_CHANGED","UNIT_AURA","PLAYER_ENTERING_WORLD","PLAYER_LEAVING_WORLD","PLAYER_LOGOUT"}) do f:RegisterEvent(event) end
    f:SetScript("OnEvent",function()
        if not V.ready or (event=="UNIT_AURA" and arg1~="player") then return end
        if event=="PLAYER_LEAVING_WORLD" or event=="PLAYER_LOGOUT" then V.mountSuspended=true
        elseif event=="PLAYER_ENTERING_WORLD" then V.mountSuspended=nil;V.mountApplied=nil end
        V:SyncMount();V:RefreshMountsPage()
    end)
    local elapsed=0
    f:SetScript("OnUpdate",function()
        if not V.ready then return end
        elapsed=elapsed+(arg1 or 0);if elapsed<.25 then return end;elapsed=0
        local previous=V.mountError;V:SyncMount()
        if previous~=V.mountError then V:RefreshMountsPage() end
    end)
    self:SyncMount()
end
