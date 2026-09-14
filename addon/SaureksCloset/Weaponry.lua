-- Physical weapon placements are separate from the real equipment-slot overrides.
local V=VanityStudio
V.weaponOrder={101,102,103,104,105,106,107}
V.weaponNames={"Left waist","Right waist","Back left","Back right","Shield","Ranged back","Quiver"}
V.weaponIcons={"SecondaryHand","MainHand","MainHand","MainHand","SecondaryHand","Ranged","Ammo"}
for i,slot in ipairs(V.weaponOrder) do V.slotNames[slot]=V.weaponNames[i] end
function V:IsWeaponPosition(slot) return type(slot)=="number" and slot>=101 and slot<=107 end
function V:WeaponCompatible(id,slot)
    if not self:IsWeaponPosition(slot) then return false end
    if id==0 then return true end
    local a=SaureksClosetWeaponAssets[id];if not a then return false end
    local i=slot-101
    if i<2 then return a[1]==1 end
    if i<4 then return a[1]==1 or a[1]==2 end
    return a[1]==i-1
end
function V:NormalizeWeapons(source)
    local result={}
    for _,slot in ipairs(self.weaponOrder) do
        local id=source and source[slot]
        if type(id)=="number" and id>0 and self:WeaponCompatible(id,slot) then result[slot]=id end
    end
    for _,name in ipairs({"quiverHorizontal","hideRangedWhenStored","hideMeleeWhenStored"}) do
        if source and source[name]==true then result[name]=true end
    end
    if source and source.backBag==1 then result.backBag=1 end
    return result
end
function V:BagRendererAvailable()
    if not self:WeaponRendererAvailable() or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30515
end
function V:SelectBackBag(id)
    if id~=nil and id~=1 then return false end
    if id and not self:BagRendererAvailable() then
        self:Message("Copy the updated SaureksCloset.dll and fully restart WoW to use visible bags.")
        return false
    end
    self:CancelDraft()
    local c=VanityStudioCharacter;c.weapons=c.weapons or {}
    -- Saved with physical placements so looks, reset, and the world toggle
    -- include bags without changing actual inventory or equipped bag slots.
    c.weapons.backBag=id
    self:TrackUnsaved();self:SyncWeapons();self:Refresh();return true
end
function V:InitializeWeapons()
    for _,item in ipairs(SaureksClosetQuivers) do
        self.index[item[1]]=item;item.search=string.lower(item[2])
    end
    for _,slot in ipairs(self.weaponOrder) do
        self.slots[slot]={}
        for id,item in pairs(self.index) do
            if self:WeaponCompatible(id,slot) then table.insert(self.slots[slot],item) end
        end
        table.sort(self.slots[slot],function(a,b) return a[2]<b[2] end)
    end
    VanityStudioCharacter.weapons=self:NormalizeWeapons(VanityStudioCharacter.weapons)
end
function V:WeaponRendererAvailable()
    return type(SaureksClosetSetWeapons)=="function"
end
function V:QuiverRotationAvailable()
    if not self:WeaponRendererAvailable() or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30446
end
function V:SetQuiverHorizontal(horizontal)
    if not self:QuiverRotationAvailable() then return false end
    local c=VanityStudioCharacter;c.weapons=c.weapons or {}
    c.weapons.quiverHorizontal=horizontal and true or nil
    self:TrackUnsaved();self:SyncWeapons();self:Refresh();return true
end
function V:WeaponStoredVisibilityAvailable()
    if not self:WeaponRendererAvailable() or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30449
end
function V:SetWeaponStoredHidden(kind,hidden)
    local field=kind=="ranged" and "hideRangedWhenStored" or kind=="melee" and "hideMeleeWhenStored"
    if not field or not self:WeaponStoredVisibilityAvailable() then return false end
    local c=VanityStudioCharacter;c.weapons=c.weapons or {}
    c.weapons[field]=hidden and true or nil
    self:TrackUnsaved();self:SyncWeapons();self:Refresh();return true
end
function V:SlotSelection(slot)
    local c=VanityStudioCharacter
    if self:IsWeaponPosition(slot) then return (c.weapons or {})[slot] end
    return c.selected[slot]
end
function V:SelectWeapon(slot,id)
    if id and not self:WeaponCompatible(id,slot) then return false end
    if not self:WeaponRendererAvailable() then
        self:Message("Copy the updated SaureksCloset.dll, then fully restart WoW through VanillaFixes to use weapon placements.")
        return false
    end
    self:CancelDraft()
    local c=VanityStudioCharacter;c.weapons=c.weapons or {}
    -- On first use, preserve compatible legacy weapon appearances in their new
    -- positions, then release the old inventory overrides before native routing.
    local used={}
    for _,role in ipairs({16,17,18}) do
        local old=c.selected[role]
        if old and old>0 then
            local choices=role==16 and {102,103,104,101} or (role==17 and {101,105,104,103} or {106})
            for _,position in ipairs(choices) do
                if not used[position] and not c.weapons[position] and self:WeaponCompatible(old,position) then
                    c.weapons[position]=old;used[position]=true;break
                end
            end
        end
        c.selected[role]=nil
    end
    c.weapons[slot]=id and id>0 and id or nil
    self:TrackUnsaved();self:Sync();self:Refresh();return true
end
function V:RealWeaponItems()
    local real={}
    for i=1,3 do
        local link=GetInventoryItemLink("player",i+15)
        if link then local _,_,id=string.find(link,"item:(%d+)");real[i]=tonumber(id) end
        real[i]=real[i] or 0
    end
    return real
end
function V:RealQuiverItem()
    for slot=20,23 do
        local link=GetInventoryItemLink("player",slot)
        if link then
            local _,_,number=string.find(link,"item:(%d+)")
            local id=tonumber(number);local asset=id and SaureksClosetWeaponAssets[id]
            if asset and asset[1]==5 then return id end
        end
    end
    return 0
end
-- Vanilla bows have no native sheath, and the two-handed sword home (26)
-- collides with quivers. Reserve separate homes for real equipped weapons too.
-- These fallbacks are renderer inputs only; saved choices remain untouched.
function V:EffectiveWeapons(weapons,overrides)
    local effective=self:Copy(weapons or {})
    local c=VanityStudioCharacter
    if not overrides then overrides=c.enabled and c.selected or {} end
    local real=self:RealWeaponItems();local routes=self:RawPreviewWeaponRoutes(effective)
    for role=1,3 do
        local asset=SaureksClosetWeaponAssets[real[role]]
        -- Preserve legacy inventory overrides, including explicitly hidden slots.
        if asset and not routes[role+15] and overrides[role+15]==nil then
            local choices
            if role==3 and asset[1]==4 then choices={106}
            elseif asset[1]==3 then choices={105}
            elseif asset[1]==2 then choices=role==1 and {103,104} or {104,103}
            elseif asset[1]==1 then choices=role==1 and {102,101,103,104} or {101,102,104,103} end
            for _,slot in ipairs(choices or {}) do
                if not effective[slot] then effective[slot]=real[role];break end
            end
        end
    end
    return effective
end
function V:ApplyWeaponRenderer(token,weapons)
    if not self:WeaponRendererAvailable() then return false end
    local w=weapons or {};local real=self:RealWeaponItems()
    if w.backBag and not self:BagRendererAvailable() then return false,-2 end
    local ok,status=pcall(SaureksClosetSetWeapons,token,w[101] or 0,w[102] or 0,w[103] or 0,w[104] or 0,w[105] or 0,w[106] or 0,w[107] or 0,real[1],real[2],real[3],w.quiverHorizontal and 1 or 0,w.hideRangedWhenStored and 1 or 0,w.hideMeleeWhenStored and 1 or 0,self:RealQuiverItem(),w.backBag or 0)
    return ok and status==1,status
end
function V:SyncWeapons()
    if self.SyncBagTuning then self:SyncBagTuning() end
    local c=VanityStudioCharacter
    local weapons=c.enabled and self:EffectiveWeapons(c.weapons,c.selected) or {}
    if not self:WeaponRendererAvailable() then return end
    -- Native synchronization is idempotent. This also restores children after
    -- model changes without cloning/reloading the character every update.
    local ok,status=self:ApplyWeaponRenderer(0,weapons)
    self.weaponError=not ok and status~=0 and "Weapon placements could not be applied." or nil
end
function V:PreviewWeapons()
    local c=VanityStudioCharacter;local weapons=self:Copy(c.enabled and c.weapons or {})
    if self.draft and self:IsWeaponPosition(self.draft.slot) then weapons[self.draft.slot]=self.draft.id end
    return self:EffectiveWeapons(weapons)
end
function V:WeaponDisplaySignature(weapons)
    local w=weapons or {}
    return ":q"..(w.quiverHorizontal and 1 or 0)..":r"..(w.hideRangedWhenStored and 1 or 0)..":m"..(w.hideMeleeWhenStored and 1 or 0)..":bag"..(w.backBag or 0)
end
function V:WeaponSignature(weapons,overrides,omitDisplayOptions)
    weapons=self:EffectiveWeapons(weapons,overrides)
    local text=""
    for _,slot in ipairs(self.weaponOrder) do text=text..":"..((weapons or {})[slot] or 0) end
    text=text..":a"..self:RealQuiverItem()
    if not omitDisplayOptions then text=text..self:WeaponDisplaySignature(weapons) end
    return text
end
function V:PreviewWeaponRoutes(weapons,overrides)
    return self:RawPreviewWeaponRoutes(self:EffectiveWeapons(weapons,overrides))
end
function V:RawPreviewWeaponRoutes(weapons)
    local real=self:RealWeaponItems();local routes,used={},{}
    local order={{102,101,103,104,105,106,107},{101,102,104,103,105,106,107},{106,101,102,103,104,105,107}}
    for role=1,3 do
        local actual=SaureksClosetWeaponAssets[real[role]]
        if actual then
            for pass=1,2 do
                if not routes[role+15] then
                    for _,slot in ipairs(order[role]) do
                        local a=SaureksClosetWeaponAssets[(weapons or {})[slot]]
                        if a and not used[slot] and a[1]==actual[1] and (pass==2 or a[2]==actual[2]) and (role~=3 or a[2]==actual[2]) then
                            routes[role+15]=slot;used[slot]=true;break
                        end
                    end
                end
            end
        end
    end
    return routes
end
function V:CopyWardrobeModel(target)
    target.weaponToken=nil
    if self:WeaponRendererAvailable() then
        local c=VanityStudioCharacter;local b=c.enabled and c.body or self:NativeBody()
        if not b then return false end
        local started,status=pcall(SaureksClosetBeginPreview,b.race,b.sex,b.skin,b.face,b.hairStyle,b.hairColor,b.facial)
        if started and (status==-1 or status==-4) then return false end
        if not started or status~=1 then error("Preview is not ready") end
        local copied=pcall(target.SetUnit,target,"player")
        local ended,token=pcall(SaureksClosetEndPreview)
        if copied and ended and token==-1 then return false end
        if not copied or not ended or not token or token<1 then error("Preview could not be created") end
        target.weaponToken=token
    else target:SetUnit("player") end
    return true
end
function V:DressWeaponPlacements(target,weapons,overrides)
    if not target.weaponToken or not self:WeaponRendererAvailable() then return true end
    local ok,status=self:ApplyWeaponRenderer(target.weaponToken,self:EffectiveWeapons(weapons,overrides))
    if not ok and status~=0 then error("Weapon preview unavailable") end
    return ok
end
