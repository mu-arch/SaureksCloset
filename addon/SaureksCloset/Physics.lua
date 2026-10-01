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
    c.physics.capeBags=c.physics.capeBags==true
    c.physics.capeWeapons=c.physics.capeWeapons~=false
    local function number(value,low,high,default)
        if type(value)~="number" or value~=value then return default end
        return math.max(low,math.min(high,value))
    end
    c.physics.capeWeight=number(c.physics.capeWeight,.25,3,1)
    c.physics.capeStiffness=number(c.physics.capeStiffness,0,1,.2)
    c.physics.capeAir=number(c.physics.capeAir,0,1,.25)
    self.appliedCapeOptions=nil;self.capeOptionsBridge=nil
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
    local optionsSynced=self:SyncCapeOptions(force)
    local value=c and c.enabled and self:CapePhysicsEnabled() and 1 or 0
    -- A settings failure must never block the master switch from stopping cloth.
    if not optionsSynced and value==1 then return false end
    -- Ordinary appearance refreshes must not restart an active simulation.
    if not force and self.appliedCapePhysics==value and self.capePhysicsBridge==SaureksClosetSetCapePhysics then return true end
    local ok,result=pcall(SaureksClosetSetCapePhysics,value)
    if not ok or result==0 or result==false then
        self.appliedCapePhysics=nil
        self.capePhysicsError="Could not update cape physics. Try again."
        return false
    end
    self.appliedCapePhysics=value;self.capePhysicsBridge=SaureksClosetSetCapePhysics
    if optionsSynced then self.capePhysicsError=nil end
    return optionsSynced
end

function V:CapePhysicsStatus()
    if not self:CapePhysicsAvailable() then return nil end
    local ok,status=pcall(SaureksClosetCapePhysicsStatus)
    if ok and (status==0 or status==1 or status==2 or status==3 or status==4 or status==5) then return status end
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
    if not VanityStudioCharacter.enabled or not self:CapePhysicsEnabled() or (self:CapePhysicsStatus()~=2 and self:CapePhysicsStatus()~=4 and self:CapePhysicsStatus()~=5) then return false end
    local ok,result=pcall(SaureksClosetResetCapePhysics)
    ok=ok and result~=0 and result~=false
    self.capePhysicsError=not ok and "Could not reset cape motion. Try again." or nil
    if self.RefreshPhysicsPage then self:RefreshPhysicsPage() end
    return ok
end

function V:CapeOptionsAvailable()
    return self:CapePhysicsAvailable() and type(SaureksClosetConfigureCapePhysics)=="function"
end
function V:SyncCapeOptions(force)
    if not self:CapeOptionsAvailable() then
        self.appliedCapeOptions=nil;self.capeOptionsBridge=nil
        return true -- Older renderers retain their existing enable/reset controls.
    end
    local p=VanityStudioCharacter.physics
    local key=(p.capeBags and "1" or "0")..":"..(p.capeWeapons and "1" or "0")..":"..p.capeWeight..":"..p.capeStiffness..":"..p.capeAir
    if not force and self.appliedCapeOptions==key and self.capeOptionsBridge==SaureksClosetConfigureCapePhysics then return true end
    local ok,result=pcall(SaureksClosetConfigureCapePhysics,p.capeBags and 1 or 0,p.capeWeapons and 1 or 0,p.capeWeight,p.capeStiffness,p.capeAir)
    if not ok or result==0 or result==false then
        self.capePhysicsError="Could not update cape settings. Try again."
        self.appliedCapeOptions=nil;return false
    end
    self.appliedCapeOptions=key;self.capeOptionsBridge=SaureksClosetConfigureCapePhysics
    return true
end
function V:SetCapeOption(key,value)
    if not self:CapeOptionsAvailable() then return false end
    if key=="capeBags" or key=="capeWeapons" then value=value and true or false
    else
        local bounds={capeWeight={.25,3},capeStiffness={0,1},capeAir={0,1}}
        local range=bounds[key]
        if not range or type(value)~="number" or value~=value then return false end
        value=math.max(range[1],math.min(range[2],value))
    end
    local p=VanityStudioCharacter.physics;local prior=p[key];p[key]=value
    local ok=self:SyncCapeOptions()
    if not ok then p[key]=prior else self.capePhysicsError=nil end
    if self.RefreshPhysicsPage then self:RefreshPhysicsPage() end
    return ok
end
function V:RestoreCapeDefaults()
    if not self:CapeOptionsAvailable() then return false end
    local p=VanityStudioCharacter.physics
    local prior={p.capeWeight,p.capeStiffness,p.capeAir}
    p.capeWeight=1;p.capeStiffness=.2;p.capeAir=.25
    local ok=self:SyncCapeOptions()
    if not ok then p.capeWeight=prior[1];p.capeStiffness=prior[2];p.capeAir=prior[3] else self.capePhysicsError=nil end
    self:RefreshPhysicsPage();return ok
end
