-- Nearby appearance sharing. No game chat, addon channels, or position uploads.
local V=VanityStudio
V.sharingStatusText={
    [0]="Sharing is off.",[1]="Connecting to the sharing service...",[2]="Connected to the sharing service.",
    [3]="Connection lost. Reconnecting...",[-1]="Set up your sharing connection to continue.",
    [-2]="Sharing requires the updated DLL and WebSocket support.",[-3]="Check your sharing connection settings.",
}
function V:SharingAvailable()
    return type(SaureksClosetConfigureSharing)=="function" and type(SaureksClosetUpdateSharing)=="function"
end
function V:InitializeSharing()
    -- Explicit opt-in, independent of automatic update checks.
    if VanityStudioDB.broadcastTransmog==nil then VanityStudioDB.broadcastTransmog=false end
    if VanityStudioDB.receiveTransmog==nil then VanityStudioDB.receiveTransmog=false end
    self.sharingConfigured=nil;self.sharingStatus=0;self:ConfigureSharing()
end
function V:StopSharingSession()
    self.sharingWorldPaused=true
    if self:SharingAvailable() then pcall(SaureksClosetConfigureSharing,0,0,"","","","","","") end
    self.sharingConfigured=nil;self.sharingStatus=0;self:RefreshSharingUI()
end
function V:SharingMetadata()
    local version,build
    if GetBuildInfo then version,build=GetBuildInfo() end
    local server=GetCVar and GetCVar("realmList") or ""
    local realm=GetRealmName and GetRealmName() or ""
    local name=UnitName and UnitName("player") or ""
    return server or "",realm or "",name or "",(version or "1.12.1").." / "..(build or "5875")
end
function V:ConfigureSharing()
    local db,c=VanityStudioDB,VanityStudioCharacter
    if not db or not c then return false end
    local broadcast=db.broadcastTransmog and c.enabled and 1 or 0
    local receive=db.receiveTransmog and 1 or 0
    if self.sharingWorldPaused then broadcast=0;receive=0 end
    if not self:SharingAvailable() then self.sharingStatus=(broadcast+receive>0) and -2 or 0;self:RefreshSharingUI();return false end
    local server,realm,name,game=self:SharingMetadata()
    local endpoint=db.sharingEndpoint or V.SHARING_ENDPOINT or ""
    local token=c.sharingAccessKey or ""
    -- Length-prefix metadata to avoid ambiguous concatenations in the cache key.
    local signature=broadcast..":"..receive
    for _,value in ipairs({endpoint,token,server,realm,name,game}) do signature=signature..":"..string.len(value)..":"..value end
    if signature~=self.sharingConfigured then
        local ok,status=pcall(SaureksClosetConfigureSharing,broadcast,receive,endpoint,token,server,realm,name,game)
        if ok and status==1 then self.sharingConfigured=signature
        else
            -- Invalid replacement settings must not leave the old service active.
            pcall(SaureksClosetConfigureSharing,0,0,"","","","","","")
            self.sharingConfigured=nil;self.sharingStatus=ok and status or -3;self:RefreshSharingUI();return false
        end
    end
    return true
end
function V:SetSharingOption(key,enabled)
    if key~="broadcastTransmog" and key~="receiveTransmog" then return false end
    VanityStudioDB[key]=enabled and true or false
    self.sharingConfigured=nil;self:ConfigureSharing();self:UpdateSharing(true);self:RefreshSharingUI();return true
end
function V:SharingSnapshot()
    local c=VanityStudioCharacter;local body=c.enabled and c.body
    local values={body and 1 or 0,body and body.race or 0,body and body.sex or 0,body and body.skin or 0,
        body and body.face or 0,body and body.hairStyle or 0,body and body.hairColor or 0,body and body.facial or 0}
    local weapons=self:EffectiveWeapons(c.weapons)
    for slot=1,19 do
        local value=c.enabled and c.selected[slot]
        if slot>=16 and slot<=18 then
            value=c.enabled and weapons[slot+92]
            if value==0 then value=nil end -- No hand override means inherit real equipment.
        end
        table.insert(values,type(value)=="number" and value or -1)
    end
    table.insert(values,self:WeaponStowMask(weapons))
    return values
end
function V:UpdateSharing(force)
    if self.sharingWorldPaused then return end
    if not VanityStudioDB or not VanityStudioCharacter then return end
    if not force and not VanityStudioDB.broadcastTransmog and not VanityStudioDB.receiveTransmog and self.sharingStatus==0 then return end
    if not self:ConfigureSharing() then return end
    local values=self:SharingSnapshot()
    local ok,status=pcall(SaureksClosetUpdateSharing,unpack(values))
    self.sharingStatus=ok and status or -3
    self:RefreshSharingUI()
end
function V:RefreshSharingUI()
    local db=VanityStudioDB or {}
    if self.broadcastTransmogCheckbox then self.broadcastTransmogCheckbox:SetChecked(db.broadcastTransmog) end
    if self.receiveTransmogCheckbox then self.receiveTransmogCheckbox:SetChecked(db.receiveTransmog) end
    if self.sharingStatusLabel then self.sharingStatusLabel:SetText(self.sharingStatusText[self.sharingStatus or 0] or "Sharing is unavailable.") end
end
function V:CreateSharingSettings(privacy,label,edit,settingsButton)
    label(privacy,"Transmog sharing",38,219,284,24)
    local options={
        {"broadcastTransmog","Broadcast my transmog","Share your appearance with nearby players who have sharing enabled. Edits are sent after five seconds without another change."},
        {"receiveTransmog","Show other players' transmogs","Display shared appearances for players loaded nearby. Turning this off immediately restores their normal appearances."},
    }
    for i,option in ipairs(options) do
        local box=CreateFrame("CheckButton","SaureksClosetSharing"..i,privacy,"UICheckButtonTemplate")
        box:SetPoint("TOPLEFT",privacy,"TOPLEFT",36,-(246+(i-1)*32));box:SetWidth(24);box:SetHeight(24)
        label(privacy,option[2],67,251+(i-1)*32,254,28,true)
        box.sharingKey=option[1];box.sharingHelp=option[3]
        box:SetScript("OnClick",function() V:SetSharingOption(this.sharingKey,this:GetChecked()) end)
        box:SetScript("OnEnter",function() GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:SetText("Transmog sharing");GameTooltip:AddLine(this.sharingHelp,1,1,1,true);GameTooltip:Show() end)
        box:SetScript("OnLeave",function() GameTooltip:Hide() end)
        self[option[1].."Checkbox"]=box
    end
    local disclosure=label(privacy,"When enabled, the sharing service records your server, realm, character name, game version and connection time. Nearby players receive appearance data and a character identifier, never this service's user directory.",38,312,284,62,true)
    disclosure:SetFont("Fonts\\FRIZQT__.TTF",10);disclosure:SetTextColor(.8,.8,.8)
    self.sharingStatusLabel=label(privacy,"",38,377,284,24,true);self.sharingStatusLabel:SetFont("Fonts\\FRIZQT__.TTF",10)
    self.sharingConnectionButton=settingsButton(privacy,"Sharing connection",38,404,284,function() V:OpenSharingConnection() end)
    -- Setup is separate so the privacy page remains readable without scrolling.
    local panel=CreateFrame("Frame","SaureksClosetSharingConnection",UIParent)
    self.sharingConnectionWindow=panel;panel:SetWidth(350);panel:SetHeight(290);panel:SetPoint("CENTER",UIParent,"CENTER",0,0);panel:SetFrameStrata("DIALOG")
    panel:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border",tile=true,tileSize=32,edgeSize=32,insets={left=11,right=12,top=12,bottom=11}})
    panel:EnableMouse(true);panel:Hide();table.insert(UISpecialFrames,panel:GetName())
    label(panel,"Sharing connection",25,22,300,22)
    label(panel,"Service address (WSS)",25,62,300,20)
    self.sharingEndpointEdit=edit(panel,"SaureksClosetSharingEndpoint",30,87,288,512)
    label(panel,"Access key for this character",25,126,300,20)
    self.sharingKeyEdit=edit(panel,"SaureksClosetSharingKey",30,151,288,64)
    if self.sharingKeyEdit.SetPassword then self.sharingKeyEdit:SetPassword(true) end
    label(panel,"Use the address and character access key supplied by the sharing service.",25,188,300,40,true)
    settingsButton(panel,"Save",25,239,140,function()
        VanityStudioDB.sharingEndpoint=V.sharingEndpointEdit:GetText();VanityStudioCharacter.sharingAccessKey=V.sharingKeyEdit:GetText()
        V.sharingConfigured=nil;V:ConfigureSharing();V:UpdateSharing(true);V.sharingConnectionWindow:Hide()
    end)
    settingsButton(panel,"Cancel",185,239,140,function() V.sharingConnectionWindow:Hide() end)
    self:RefreshSharingUI()
end
function V:OpenSharingConnection()
    self.sharingEndpointEdit:SetText(VanityStudioDB.sharingEndpoint or self.SHARING_ENDPOINT or "")
    self.sharingKeyEdit:SetText(VanityStudioCharacter.sharingAccessKey or "")
    self.sharingConnectionWindow:Show()
end
