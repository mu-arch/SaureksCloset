-- Player-only cloth preferences belong to the character, not to saved looks.
local V=VanityStudio

function V:CapePhysicsAvailable()
    return type(SaureksClosetSetCapePhysics)=="function" and
        type(SaureksClosetResetCapePhysics)=="function" and
        type(SaureksClosetCapePhysicsStatus)=="function"
end

function V:InitializePhysics()
    local c=VanityStudioCharacter
    if type(c.physics)~="table" then c.physics={} end
    c.physics.cape=c.physics.cape==true
    self.appliedCapePhysics=nil;self.capePhysicsBridge=nil;self.capePhysicsError=nil
    self:SyncCapePhysics(true)
end

function V:CapePhysicsEnabled()
    local c=VanityStudioCharacter
    return c and type(c.physics)=="table" and c.physics.cape==true or false
end

function V:SyncCapePhysics(force)
    if not self:CapePhysicsAvailable() then
        self.appliedCapePhysics=nil;self.capePhysicsBridge=nil
        return false
    end
    local c=VanityStudioCharacter
    local value=c and c.enabled and self:CapePhysicsEnabled() and 1 or 0
    -- Ordinary appearance refreshes must not restart an active simulation.
    if not force and self.appliedCapePhysics==value and self.capePhysicsBridge==SaureksClosetSetCapePhysics then return true end
    local ok,result=pcall(SaureksClosetSetCapePhysics,value)
    if not ok or result==0 or result==false then
        self.appliedCapePhysics=nil
        self.capePhysicsError="Could not update cape physics. Try again."
        return false
    end
    self.appliedCapePhysics=value;self.capePhysicsBridge=SaureksClosetSetCapePhysics
    self.capePhysicsError=nil
    return true
end

function V:CapePhysicsStatus()
    if not self:CapePhysicsAvailable() then return nil end
    local ok,status=pcall(SaureksClosetCapePhysicsStatus)
    if ok and (status==0 or status==1 or status==2 or status==3 or status==4) then return status end
end

function V:SetCapePhysics(enabled)
    if not self:CapePhysicsAvailable() then return false end
    local c=VanityStudioCharacter
    if type(c.physics)~="table" then c.physics={} end
    c.physics.cape=enabled and true or false
    local ok=self:SyncCapePhysics(true)
    if self.RefreshPhysicsPage then self:RefreshPhysicsPage() end
    return ok
end

function V:ResetCapePhysics()
    if not VanityStudioCharacter.enabled or not self:CapePhysicsEnabled() or (self:CapePhysicsStatus()~=2 and self:CapePhysicsStatus()~=4) then return false end
    local ok,result=pcall(SaureksClosetResetCapePhysics)
    ok=ok and result~=0 and result~=false
    self.capePhysicsError=not ok and "Could not reset cape motion. Try again." or nil
    if self.RefreshPhysicsPage then self:RefreshPhysicsPage() end
    return ok
end
