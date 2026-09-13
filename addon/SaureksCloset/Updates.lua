local V=VanityStudio
V.REQUIRED_RENDERER=30500
V.websiteURLs={"https://github.com/mu-arch/SaureksCloset","https://github.com/mu-arch/SaureksCloset/releases","https://discord.gg/6mfxCdNbM6"}
function V:VersionParts(version)
    local _,_,major,minor,patch=string.find(version or "","^(%d+)%.(%d+)%.(%d+)$")
    major=tonumber(major);minor=tonumber(minor);patch=tonumber(patch)
    if not major or major<1 or major>65535 or minor>65535 or patch>65535 then return nil end
    return {major,minor,patch}
end
function V:VersionIsNewer(candidate,current)
    local a=self:VersionParts(candidate);local b=self:VersionParts(current)
    if not a or not b then return false end
    for i=1,3 do if a[i]~=b[i] then return a[i]>b[i] end end
    return false
end
function V:RendererVersionText(version)
    if type(version)~="number" or version<1 or version>999999 or version~=math.floor(version) then return "Not loaded" end
    return string.format("%d.%d.%d",math.floor(version/10000),math.mod(math.floor(version/100),100),math.mod(version,100))
end
function V:RefreshUpdateUI()
    local alert=self.updateMismatch or self.remoteUpdateAvailable
    if self.frame then
        self.frame.title:SetText(alert and "Saurek's Closet (Update Available)" or "Saurek's Closet")
        self.frame.title:SetFont("Fonts\\FRIZQT__.TTF",alert and 11 or 12)
    end
    if self.autoUpdatesCheckbox then self.autoUpdatesCheckbox:SetChecked(VanityStudioDB.autoCheckUpdates and 1 or nil) end
    if self.updatesSummary then
        local text="Addon: "..self.VERSION.."\nLoaded DLL: "..self:RendererVersionText(self.loadedRenderer).."\nRequired DLL: "..self:RendererVersionText(self.REQUIRED_RENDERER)
        if self.updateMismatch then text=text.."\n\nThe addon and loaded DLL do not match. Install both from the same release, then fully restart WoW through VanillaFixes." end
        local remote=VanityStudioDB.autoCheckUpdates and self.remoteRelease
        text=text.."\n\nGitHub current version addon: "..(remote and remote.addon or "Unknown")
            .."\nGitHub current version DLL: "..(remote and self:RendererVersionText(remote.dll) or "Unknown")
        self.updatesSummary:SetText(text)
        self.updatesStatus:SetText(self.updateStatus or "Not checked yet.")
    end
    if self.checkUpdatesButton then
        local on=VanityStudioDB.autoCheckUpdates and not self.updatePolling
        if on then self.checkUpdatesButton:Enable() else self.checkUpdatesButton:Disable() end
        self.checkUpdatesButton:SetAlpha(on and 1 or .45)
        self.checkUpdatesButton.closetEnabled=on and true or false
    end
end
function V:CheckLocalRenderer()
    local ok,value=false,nil
    if type(SaureksClosetRendererVersion)=="function" then ok,value=pcall(SaureksClosetRendererVersion) end
    self.loadedRenderer=ok and value or nil
    self.updateMismatch=self.loadedRenderer~=self.REQUIRED_RENDERER
    local key=tostring(self.loadedRenderer)..":"..self.REQUIRED_RENDERER
    if self.updateMismatch and self.lastDllAlert~=key then
        self.lastDllAlert=key
        self:Message("Update Available: addon "..self.VERSION.." requires DLL "..self:RendererVersionText(self.REQUIRED_RENDERER).."; loaded: "..self:RendererVersionText(self.loadedRenderer)..". Install matching files and fully restart WoW through VanillaFixes.")
    end
    self:RefreshUpdateUI()
end
function V:InitializeUpdates()
    if type(VanityStudioDB.autoCheckUpdates)~="boolean" then VanityStudioDB.autoCheckUpdates=true end
    self:CheckLocalRenderer()
    self:SetAutoUpdates(VanityStudioDB.autoCheckUpdates)
end
function V:SetAutoUpdates(on)
    VanityStudioDB.autoCheckUpdates=on and true or false
    if type(SaureksClosetSetUpdateChecks)=="function" then pcall(SaureksClosetSetUpdateChecks,on and 1 or 0) end
    self.updatePolling=nil;self.updateWaitingStart=nil
    self.updateDue=on and GetTime()+3 or nil
    self.updateStatus=on and "Automatic checks enabled." or "Update checks disabled."
    self:RefreshUpdateUI()
end
function V:CheckForUpdates(manual)
    self:CheckLocalRenderer()
    if not VanityStudioDB.autoCheckUpdates then return false end
    if self.updatePolling then return false end
    self.updateDue=nil
    if type(SaureksClosetStartUpdateCheck)~="function" or type(SaureksClosetPollUpdateCheck)~="function" then
        self.updateStatus="Update checker unavailable. Install the matching DLL and fully restart WoW."
        if manual then self:Message(self.updateStatus) end
        self:RefreshUpdateUI();return false
    end
    local ok,status=pcall(SaureksClosetStartUpdateCheck)
    if not ok or (status~=0 and status~=1 and status~=2) then
        self.updateStatus=status==-3 and "Please wait one minute before checking again." or "Could not check for updates. Try again later."
        if status==-3 and not manual then self.updateDue=GetTime()+60 end
        if manual then self:Message(self.updateStatus) end
        self:RefreshUpdateUI();return false
    end
    self.updateWaitingStart=status==0
    self.updatePolling=true;self.updateDeadline=GetTime()+30;self.updatePollAt=0
    self.updateStatus="Checking GitHub...";self:RefreshUpdateUI();return true
end
function V:UpdateUpdates()
    if not VanityStudioDB.autoCheckUpdates then return end
    local now=GetTime()
    if self.updateDue and now>=self.updateDue then self:CheckForUpdates(false) end
    if not self.updatePolling or now<(self.updatePollAt or 0) then return end
    self.updatePollAt=now+.25
    if now>self.updateDeadline then
        -- Discard the in-flight generation; the DLL worker closes its own handles.
        if type(SaureksClosetSetUpdateChecks)=="function" then pcall(SaureksClosetSetUpdateChecks,0);pcall(SaureksClosetSetUpdateChecks,1) end
        self.updatePolling=nil;self.updateStatus="The update check timed out. Try again later.";self:RefreshUpdateUI();return
    end
    if self.updateWaitingStart then
        local ok,status=pcall(SaureksClosetStartUpdateCheck)
        if ok and status==0 then return end
        self.updateWaitingStart=nil
        if not ok or (status~=1 and status~=2) then
            self.updatePolling=nil;self.updateStatus="The previous check was cancelled. Try again in a minute.";self:RefreshUpdateUI();return
        end
    end
    local values={pcall(SaureksClosetPollUpdateCheck)}
    local status=values[1] and values[2]
    if status==1 then return end
    self.updatePolling=nil
    local valid=status==2
    for i=3,6 do
        local value=values[i]
        if type(value)~="number" or value~=math.floor(value) or value<0 or value>(i==6 and 999999 or 65535) then valid=false end
    end
    if valid and values[3]>0 and values[6]>0 then
        local addon=string.format("%d.%d.%d",values[3],values[4],values[5])
        self.remoteRelease={addon=addon,dll=values[6]}
        self.remoteUpdateAvailable=self:VersionIsNewer(addon,self.VERSION) or values[6]>self.REQUIRED_RENDERER
        self.updateStatus=self.remoteUpdateAvailable and "Update available on GitHub." or "You have the latest published version or newer."
        local key=addon..":"..values[6]
        if self.remoteUpdateAvailable and self.lastUpdateAlert~=key then
            self.lastUpdateAlert=key;self:Message("Update Available: Saurek's Closet "..addon..". Open Settings > Version Details for the download page.")
        end
    else self.updateStatus="Could not check for updates. Try again later." end
    self:RefreshUpdateUI()
end
function V:OpenWebsite(page)
    if not self.websiteURLs[page] then return false end
    if self.websiteAddress then self.websiteAddress:SetText(self.websiteURLs[page]) end
    if type(SaureksClosetOpenWebsite)=="function" then
        local ok,status=pcall(SaureksClosetOpenWebsite,page)
        if ok and status==1 then return true end
    end
    self:Message("Open this address in your browser: "..self.websiteURLs[page]);return false
end
