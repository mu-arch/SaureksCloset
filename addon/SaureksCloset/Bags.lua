-- Cosmetic bag instances belong to a look, never to inventory bag slots.
local V=VanityStudio
V.MAX_BAGS=5
-- Keep the renderer's eight identity slots for older looks and wire calls.
-- The visible bag limit is independent of those stable attachment IDs.
V.BAG_INSTANCE_SLOTS=8
V.bagMounts={back=0,leftHip=1,rightHip=2}
-- A character visibility preference, not a change to the saved look's bags.
function V:BagsShown()
    return not (VanityStudioCharacter and VanityStudioCharacter.bagsHidden)
end
function V:SetBagsShown(shown)
    if not self:MultiBagRendererAvailable() then return false end
    VanityStudioCharacter.bagsHidden=not shown and true or nil
    self:SyncWeapons()
    self:SyncLiveBagFits()
    self.detailPreviewSignature=nil
    self:Refresh()
    return true
end
function V:BagBodyType(model)
    return type(model)=="number" and model>=12 and model<=16 and "Soft body" or "Rigid body"
end
function V:BagPhysicsAmount(bag)
    local n=bag and bag.amplitude
    return type(n)=="number" and n==n and math.max(0,math.min(200,math.floor(n+.5))) or 100
end
function V:SetBagPhysics(id,on,amount)
    local bag=self:BagInstance(id)
    if not bag or not self:AnimationControlsAvailable() or type(amount)~="number" or not (amount>=0 and amount<=200) then return false end
    bag.physics=on and true or false;bag.amplitude=math.floor(amount+.5)
    self:BagChanged();return true
end
local fitFields={"left","inset","up","pitch","roll","yaw","scale"}
local function integer(n,low,high)
    return type(n)=="number" and n>=low and n<=high and n==math.floor(n)
end
local function assignBagSlots(bags,limit)
    local occupied={}
    -- Reserve explicit positions before filling legacy gaps. An earlier bag
    -- without a slot must not take a later bag's saved position.
    for i,bag in ipairs(bags) do
        if i>limit then break end
        if type(bag)=="table" then
            if integer(bag.slot,1,limit) and not occupied[bag.slot] then occupied[bag.slot]=bag
            else bag.slot=nil end
        end
    end
    local slot=1
    for i,bag in ipairs(bags) do
        if i>limit then break end
        if type(bag)=="table" and not bag.slot then
            while occupied[slot] do slot=slot+1 end
            bag.slot=slot;occupied[slot]=bag
        end
    end
    return occupied
end
function V:BagModelChoice(modelID)
    return self.bagModelChoicesByID and self.bagModelChoicesByID[modelID]
end
function V:BagModelColor(modelID)
    return self.bagModelColorsByID and self.bagModelColorsByID[modelID]
end
function V:ValidBagFit(values)
    if type(values)~="table" then return false end
    for i,key in ipairs(fitFields) do
        local n=values[key];local low=key=="up" and -3 or (i<=3 and -1 or (i<=6 and -180 or 25))
        local high=i<=3 and 1 or (i<=6 and 180 or 200)
        if type(n)~="number" or not (n>=low and n<=high) then return false end
    end
    return true
end
function V:MultiBagRendererAvailable()
    if type(SaureksClosetSetBags)~="function" or type(SaureksClosetSetBagInstanceFit)~="function" or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30800
end
function V:NormalizeBags(weapons)
    local result,used={},{}
    weapons=type(weapons)=="table" and weapons or {}
    local source=weapons.bags
    if type(source)~="table" then
        if weapons.backBag~=1 then return result end
        local old={id=1,model=1,mount="back",fits={}}
        -- Import the old single-bag fit once, including its built-in offset.
        for race=1,8 do for sex=0,1 do
            local saved=self.BagTunerSaved and self:BagTunerSaved(1,race,sex)
            local values=saved and saved.values or (self.BagTunerDefaults and self:BagTunerDefaults(1,race,sex))
            if self:ValidBagFit(values) then old.fits[race..":"..sex]=self:Copy(values) end
        end end
        source={old}
    end
    for _,bag in ipairs(source) do
        if table.getn(result)>=self.MAX_BAGS then break end
        local model=type(bag)=="table" and bag.model
        -- Retire selections in a copy, leaving stored looks intact until saved.
        -- The old olive pouch uses its low model with the same placement.
        model=(self.retiredBagModels and self.retiredBagModels[model]) or model
        if type(bag)=="table" and integer(bag.id,1,self.BAG_INSTANCE_SLOTS) and not used[bag.id] and self.bagCatalogByID[model] then
            local clean={id=bag.id,slot=bag.slot,model=model,mount=self.bagMounts[bag.mount] and bag.mount or "back",fits={}}
            for race=1,8 do for sex=0,1 do
                local key=race..":"..sex;local values=type(bag.fits)=="table" and bag.fits[key]
                if self:ValidBagFit(values) then clean.fits[key]=self:Copy(values) end
            end end
            clean.physics=bag.physics~=false;clean.amplitude=self:BagPhysicsAmount(bag)
            table.insert(result,clean);used[bag.id]=true
        end
    end
    assignBagSlots(result,self.MAX_BAGS)
    return result
end
function V:GetBags()
    local c=VanityStudioCharacter or {};c.weapons=c.weapons or {}
    if type(c.weapons.bags)~="table" then c.weapons.bags=self:NormalizeBags(c.weapons);c.weapons.backBag=nil end
    -- In-memory legacy callers may bypass startup normalization. Add only the
    -- layout metadata here so active bag objects, fits and tuner refs survive.
    assignBagSlots(c.weapons.bags,self.MAX_BAGS)
    return c.weapons.bags
end
function V:BagInSlot(slot)
    if not integer(slot,1,self.MAX_BAGS) then return nil end
    for i,bag in ipairs(self:GetBags()) do
        if i>self.MAX_BAGS then break end
        if type(bag)=="table" and bag.slot==slot then return bag end
    end
end
function V:BagInstance(id)
    for _,bag in ipairs(self:GetBags()) do if bag.id==id then return bag end end
end
function V:BagDraftKey(bag,race,sex)
    return "instance:"..bag.id..":"..bag.model..":"..bag.mount..":"..race..":"..sex
end
function V:VisitChangedBagFits(save)
    local changed=false
    for _,bag in ipairs(self:GetBags()) do for race=1,8 do for sex=0,1 do
        local key=race..":"..sex
        local draft=self.bagTunerDrafts and self.bagTunerDrafts[self:BagDraftKey(bag,race,sex)]
        if self:ValidBagFit(draft) then
            local baseline=bag.fits[key] or self:BagTunerDefaults(200+bag.id,race,sex)
            local different=not baseline
            if baseline then for _,field in ipairs(fitFields) do if math.abs(draft[field]-baseline[field])>.000001 then different=true end end end
            if different then
                changed=true
                if save then bag.fits[key]=self:Copy(draft) end
            end
        end
    end end end
    return changed
end
function V:HasUnsavedBagFits() return self:VisitChangedBagFits(false) end
function V:SaveBagDraftFits() return self:VisitChangedBagFits(true) end
function V:DiscardBagDrafts(id,keepOpen)
    for key in pairs(self.bagTunerDrafts or {}) do
        local prefix=id and ("instance:"..id..":") or "instance:"
        if string.sub(key,1,string.len(prefix))==prefix then self.bagTunerDrafts[key]=nil end
    end
    if not keepOpen and (not id or self.placementTunerBag==id) then
        self.placementTunerBag=nil;self.bagTunerTargetKey=nil;self.bagTunerPaused=false
        if self.bagTunerWindow and self.bagTunerWindow:IsShown() then self.bagTunerWindow:Hide() end
    end
end
function V:BagChanged()
    self:CancelDraft();self:TrackUnsaved();self:SyncWeapons();self:Refresh()
end
function V:AddBag(model,mount,slot)
    if not self:MultiBagRendererAvailable() then self:Message("Fully restart WoW with the updated SaureksCloset.dll to add bags.");return false end
    mount=mount or "back"
    if not self.bagCatalogByID[model] or self.bagMounts[mount]==nil or (slot~=nil and not integer(slot,1,self.MAX_BAGS)) then return false end
    local bags=self:GetBags();if table.getn(bags)>=self.MAX_BAGS then self:Message("You can add up to "..self.MAX_BAGS.." bags to a look.");return false end
    local used,positions={},{};for _,bag in ipairs(bags) do used[bag.id]=true;positions[bag.slot]=true end
    if slot and positions[slot] then return false end
    if not slot then slot=1;while positions[slot] do slot=slot+1 end end
    local id=1;while used[id] do id=id+1 end
    self:DiscardBagDrafts(id)
    local bag={id=id,slot=slot,model=model,mount=mount,fits={}};table.insert(bags,bag)
    self:BagChanged();return true,bag
end
function V:DeleteBag(id)
    local bags=self:GetBags()
    for i,bag in ipairs(bags) do if bag.id==id then
        self:DiscardBagDrafts(id);table.remove(bags,i);self:BagChanged();return true
    end end
    return false
end
function V:SetBagModel(id,model,keepOpen)
    local bag=self:BagInstance(id);if not bag or not self.bagCatalogByID[model] then return false end
    if bag.model==model then return true end
    -- A model swap changes the appearance, not the fit being edited. Move each
    -- race/gender draft to the new identity without overwriting saved values.
    local drafts={}
    for race=1,8 do for sex=0,1 do
        local key=self:BagDraftKey(bag,race,sex)
        local values=self.bagTunerDrafts and self.bagTunerDrafts[key]
        if self:ValidBagFit(values) then drafts[race..":"..sex]=self:Copy(values) end
    end end
    self:DiscardBagDrafts(id,keepOpen);bag.model=model
    self.bagTunerDrafts=self.bagTunerDrafts or {}
    for race=1,8 do for sex=0,1 do
        local values=drafts[race..":"..sex]
        if values then self.bagTunerDrafts[self:BagDraftKey(bag,race,sex)]=values end
    end end
    if keepOpen and self.placementTunerBag==id then
        local target,race,sex=self:BagTunerIdentity()
        self.bagTunerTargetKey=race and self:BagDraftKey(bag,race,sex) or nil
    end
    self:BagChanged();return true
end
function V:SetBagMount(id,mount,keepOpen)
    local bag=self:BagInstance(id);if not bag or self.bagMounts[mount]==nil then return false end
    if bag.mount==mount then return true end
    self:DiscardBagDrafts(id,keepOpen);bag.mount=mount;bag.fits={}
    if keepOpen and self.placementTunerBag==id then
        local target,race,sex=self:BagTunerIdentity()
        self.bagTunerTargetKey=race and self:BagDraftKey(bag,race,sex) or nil
    end
    self:BagChanged();return true
end
function V:BagSignature(weapons)
    local text=""
    for _,bag in ipairs(self:NormalizeBags(weapons)) do
        text=text..":bag"..bag.id..","..bag.model..","..bag.mount..",slot"..bag.slot..",physics"..(bag.physics==false and "off" or "on")..",amplitude"..self:BagPhysicsAmount(bag)
        for race=1,8 do for sex=0,1 do
            local key=race..":"..sex;local fit=bag.fits[key]
            if fit then
                text=text..","..key
                for _,field in ipairs(fitFields) do text=text..","..string.format("%.6f",fit[field]) end
            end
        end end
    end
    return text
end
function V:ApplyBagRenderer(token,weapons,useDrafts)
    local bags=self:BagsShown() and self:NormalizeBags(weapons) or {}
    if not self:MultiBagRendererAvailable() then return table.getn(bags)==0,-2 end
    local byID,args={}, {token}
    for _,bag in ipairs(bags) do byID[bag.id]=bag end
    for id=1,self.BAG_INSTANCE_SLOTS do
        local bag=byID[id];table.insert(args,bag and bag.model or 0);table.insert(args,bag and self.bagMounts[bag.mount] or 0)
    end
    local ok,status,generation=pcall(SaureksClosetSetBags,unpack(args))
    if not ok or status~=1 then return false,status end
    -- A native generation changes whenever children or their context are
    -- recreated. This avoids resending 128 unchanged fits on every UI tick
    -- without losing fits after a model rebuild or preview-token replacement.
    self.bagFitCaches=self.bagFitCaches or {}
    local cache=self.bagFitCaches[token]
    if not generation or not cache or cache.generation~=generation then
        local count=0;for _ in pairs(self.bagFitCaches) do count=count+1 end
        if count>=12 then self.bagFitCaches={} end
        cache={generation=generation,values={}};self.bagFitCaches[token]=cache
    end
    local store=self:BagTunerStore();local enabled=store and store.enabled~=false
    for _,bag in ipairs(bags) do for race=1,8 do for sex=0,1 do
        local key=self:BagDraftKey(bag,race,sex)
        local draft=useDrafts and self.bagTunerDrafts and self.bagTunerDrafts[key]
        local values=enabled and (draft or bag.fits[race..":"..sex]) or nil
        local motion=bag.physics~=false and not (useDrafts and self.bagTunerPaused and self.bagTunerTargetKey==key)
        local amplitude=self:BagPhysicsAmount(bag)
        -- Animation controls remain effective with default fits and live tuning off.
        if not values and (not motion or amplitude~=100) then
            local mount=self.bagMounts[bag.mount]
            local okDefaults,statusDefaults,left,inset,up,pitch,roll,yaw,scale=pcall(SaureksClosetGetBagFitDefaults,200+bag.id,race,sex,mount)
            if okDefaults and statusDefaults==1 then values={left=left,inset=inset,up=up,pitch=pitch,roll=roll,yaw=yaw,scale=scale} else return false,-2 end
        end
        local signature=values and ((motion and "on" or "paused")..":"..amplitude) or "off"
        if values then for _,field in ipairs(fitFields) do signature=signature..":"..string.format("%.6f",values[field]) end end
        if cache.values[key]~=signature then
            if values then
                ok,status=pcall(SaureksClosetSetBagInstanceFit,token,bag.id,race,sex,1,values.left,values.inset,values.up,values.pitch,values.roll,values.yaw,values.scale,motion and 1 or 0,amplitude)
            else ok,status=pcall(SaureksClosetSetBagInstanceFit,token,bag.id,race,sex,0) end
            if not ok or status~=1 then return false,status end
            cache.values[key]=signature
        end
    end end end
    return true,1
end
function V:SyncLiveBagFits()
    if not self:MultiBagRendererAvailable() then return true end
    local c=VanityStudioCharacter;local weapons=c.enabled and c.weapons or {}
    local ok,status=self:ApplyBagRenderer(0,weapons,true)
    for _,target in ipairs({self.model or false,self.previewBuffer or false}) do
        if target and target.weaponToken then
            local checked,ready=true,1
            if type(SaureksClosetPreviewStatus)=="function" then checked,ready=pcall(SaureksClosetPreviewStatus,target.weaponToken) end
            -- Expired or composing preview tokens belong to the normal preview
            -- rebuild loop; they must not report a failed world fit.
            if checked and ready==1 then
                local applied,code=self:ApplyBagRenderer(target.weaponToken,self.tab=="body" and {} or weapons,true)
                if not applied and code~=0 then ok=false end
            end
        end
    end
    return ok or status==0
end
