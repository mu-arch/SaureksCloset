-- Physical weapon placements are separate from the real equipment-slot overrides.
local V=VanityStudio
V.weaponOrder={101,102,103,104,105,106,107,108,109,110}
V.weaponNames={"Left waist","Right waist","Back left","Back right","Shield","Back ranged","Quiver","Main hand","Off hand","Ranged"}
V.weaponIcons={"SecondaryHand","MainHand","MainHand","MainHand","SecondaryHand","Ranged","Ammo","MainHand","SecondaryHand","Ranged"}
for i,slot in ipairs(V.weaponOrder) do V.slotNames[slot]=V.weaponNames[i] end
function V:IsWeaponPosition(slot) return type(slot)=="number" and slot>=101 and slot<=110 end
function V:IsCarriedWeapon(slot) return self:IsWeaponPosition(slot) and slot<=107 end
function V:CarriedWeaponsEnabled(weapons)
    local w=weapons or {}
    if type(w.carriedEnabled)=="boolean" then return w.carriedEnabled end
    -- Looks saved before this switch used carried placements as soon as a
    -- selection existed. Preserve that intent once, including explicit off.
    for slot=101,107 do
        if type(w[slot])=="number" and w[slot]>0 and self:WeaponCompatible(w[slot],slot) then return true end
    end
    return false
end
-- Advanced mode adds independently carried models. Keep the existing saved
-- field so changing the presentation never changes the intent of older looks.
function V:WeaponAdvancedMode(weapons)
    return self:CarriedWeaponsEnabled(weapons)
end
function V:WeaponCompatible(id,slot)
    if not self:IsWeaponPosition(slot) then return false end
    if id==0 then return true end
    local a=SaureksClosetWeaponAssets[id];if not a then return false end
    if slot==108 then return a[1]==1 or a[1]==2 end
    if slot==109 then return a[1]==1 or a[1]==3 end
    if slot==110 then return a[1]==4 end
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
    -- Upgrade old looks once. Keep carried models and explicitly preserve their
    -- former hand appearances; future carried edits never change attacking gear.
    if source and not source.independent then
        local routes=self:RawPreviewWeaponRoutes(result)
        result[108]=result[108] or result[routes[16] or 102] or result[103]
        result[109]=result[109] or result[routes[17] or 101] or result[105]
        result[110]=result[110] or result[106]
    end
    result.independent=true
    result.carriedEnabled=self:CarriedWeaponsEnabled(source)
    return result
end
function V:IndependentWeaponsAvailable()
    if not self:WeaponRendererAvailable() or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30700
end
function V:CarriedWeaponsAvailable()
    if not self:WeaponRendererAvailable() or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30703
end
function V:SetCarriedWeaponsEnabled(enabled)
    if not self:CarriedWeaponsAvailable() then
        self:Message("Copy the updated SaureksCloset.dll, then fully restart WoW through VanillaFixes to use Carried on your body.")
        return false
    end
    enabled=enabled and true or false
    local c=VanityStudioCharacter;c.weapons=c.weapons or {}
    local changed=self:CarriedWeaponsEnabled(c.weapons)~=enabled
    local hadDraft=self.draft~=nil
    if not enabled and self:IsCarriedWeapon(self.slot) and self.browser and self.browser:IsShown() and self.CloseBrowser then
        self:CloseBrowser()
    else self:CancelDraft() end
    if not enabled and self:IsCarriedWeapon(self.placementTunerSlot) and self.bagTunerWindow and self.bagTunerWindow:IsShown() then
        self.bagTunerWindow:Hide()
    end
    c.weapons.carriedEnabled=enabled
    if changed then self:TrackUnsaved();self:SyncWeapons() end
    if changed or hadDraft then self:Refresh() end
    return true
end
function V:SetWeaponAdvancedMode(enabled)
    return self:SetCarriedWeaponsEnabled(enabled)
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
    local c=VanityStudioCharacter
    c.weapons=self:NormalizeWeapons(c.weapons)
    self:MigrateEquippedWeaponOverrides(c.selected,c.weapons)
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
    -- Inventory can change while the appearance browser is open. Recheck the
    -- current equipment before committing, while always allowing Passthrough.
    if id and id>0 and slot>=108 and not self:WeaponChoiceCompatible(id,slot) then
        self:Message("Choose an appearance that matches the item currently equipped in this slot.")
        return false
    end
    if not self:IndependentWeaponsAvailable() then
        self:Message("Copy the updated SaureksCloset.dll, then fully restart WoW through VanillaFixes to use weapon placements.")
        return false
    end
    self:CancelDraft()
    local c=VanityStudioCharacter;c.weapons=c.weapons or {}
    -- Choosing another item must not turn a disabled carried section back on.
    c.weapons.carriedEnabled=self:CarriedWeaponsEnabled(c.weapons)
    for _,role in ipairs({16,17,18}) do
        local target=role+92
        local old=c.selected[role]
        if old and old>0 and not c.weapons[target] and self:WeaponCompatible(old,target) then c.weapons[target]=old end
        c.selected[role]=nil
    end
    c.weapons.independent=true
    c.weapons[slot]=id and id>0 and id or nil
    self:TrackUnsaved();self:Sync();self:Refresh();return true
end
function V:MigrateEquippedWeaponOverrides(slots,weapons)
    for _,role in ipairs({16,17,18}) do
        local id=slots[role];local target=role+92
        if id and id>0 and self:WeaponCompatible(id,target) then
            if not weapons[target] then weapons[target]=id end
            slots[role]=nil
        end
    end
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
function V:WeaponSlotState(slot,real)
    if slot~=108 and slot~=109 and slot~=110 then return false,"No equipment slot" end
    real=real or self:RealWeaponItems()
    local main=SaureksClosetWeaponAssets[real[1]]
    if slot==109 and main and main[1]==2 then return false,"Two-handed main hand" end
    if not real[slot-107] or real[slot-107]<=0 then return false,"No item equipped" end
    -- Unknown server items still occupy their real slot. Their absence from
    -- the local appearance catalog must not disable the corresponding editor.
    return true
end
function V:WeaponChoiceCompatible(id,slot,real)
    if not self:WeaponCompatible(id,slot) then return false end
    if id==0 or self:IsCarriedWeapon(slot) then return true end
    real=real or self:RealWeaponItems()
    if not self:WeaponSlotState(slot,real) then return false end
    if slot==110 then return true end
    local actual=SaureksClosetWeaponAssets[real[slot-107]]
    local selected=SaureksClosetWeaponAssets[id]
    -- The native renderer preserves the equipped melee role: one hand, two
    -- hands, or shield. Ranged families can freely change appearance.
    return not actual or selected[1]==actual[1]
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
-- Saved-look previews also reach this path before loading/migrating the look.
-- Normalize copies so legacy placements preserve their old attacking choices,
-- while already independent looks never refill an explicitly cleared role.
function V:EffectiveWeapons(weapons,overrides)
    if not next(weapons or {}) and not (overrides and (overrides[16] or overrides[17] or overrides[18])) then return {} end
    local result=self:NormalizeWeapons(weapons)
    if overrides then self:MigrateEquippedWeaponOverrides(self:Copy(overrides),result) end
    return result
end
function V:WeaponPreviewMode()
    if self.draft and self.draft.slot==110 then return 2 end
    if self.draft and (self.draft.slot==108 or self.draft.slot==109) then return 1 end
    return self.weaponPreviewMode or 0
end
function V:ApplyWeaponRenderer(token,weapons)
    if not self:WeaponRendererAvailable() then return false end
    local w=self:EffectiveWeapons(weapons);local real=self:RealWeaponItems()
    local carried=self:CarriedWeaponsEnabled(w)
    if w.independent and not self:IndependentWeaponsAvailable() then return false,-2 end
    -- Older renderers also differ when carried mode is off: their independent
    -- attack appearances can stay hidden while sheathed. Only an empty clear
    -- can safely cross that version boundary.
    if next(w) and not self:CarriedWeaponsAvailable() then return false,-2 end
    if w.backBag and not self:BagRendererAvailable() then return false,-2 end
    local previewMode=0
    if token>0 and ((self.model and self.model.weaponToken==token) or (self.previewBuffer and self.previewBuffer.weaponToken==token)) then previewMode=self:WeaponPreviewMode() end
    local ok,status=pcall(SaureksClosetSetWeapons,token,carried and w[101] or 0,carried and w[102] or 0,carried and w[103] or 0,carried and w[104] or 0,carried and w[105] or 0,carried and w[106] or 0,carried and w[107] or 0,real[1],real[2],real[3],w.quiverHorizontal and 1 or 0,0,0,self:RealQuiverItem(),w.backBag or 0,w[108] or 0,w[109] or 0,w[110] or 0,w.independent and 1 or 0,previewMode,carried and 1 or 0)
    return ok and status==1,status
end
function V:SyncWeapons()
    if self.SyncBagTuning then self:SyncBagTuning() end
    local c=VanityStudioCharacter
    local weapons=c.enabled and self:EffectiveWeapons(c.weapons,c.selected) or {}
    if self.draft and self:IsWeaponPosition(self.draft.slot) then weapons=self:PreviewWeapons() end
    if not self:WeaponRendererAvailable() then return end
    -- Native synchronization is idempotent. This also restores children after
    -- model changes without cloning/reloading the character every update.
    local ok,status=self:ApplyWeaponRenderer(0,weapons)
    self.weaponError=not ok and status==-2 and "Update SaureksCloset.dll and fully restart WoW through VanillaFixes to use these weapon settings." or not ok and status~=0 and "Weapon placements could not be applied." or nil
end
function V:PreviewWeapons()
    local c=VanityStudioCharacter;local weapons=self:Copy(c.enabled and c.weapons or {})
    if self.draft and self:IsWeaponPosition(self.draft.slot) then
        weapons[self.draft.slot]=self.draft.id;weapons.independent=true
        if self:IsCarriedWeapon(self.draft.slot) then weapons.carriedEnabled=true end
    end
    return self:EffectiveWeapons(weapons)
end
function V:WeaponDisplaySignature(weapons)
    local w=weapons or {}
    return ":q"..(w.quiverHorizontal and 1 or 0)..":carried"..(self:CarriedWeaponsEnabled(w) and 1 or 0)..":bag"..(w.backBag or 0)..":ind"..(w.independent and 1 or 0)
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
    if weapons and weapons.independent then
        for role=1,3 do
            local selected=SaureksClosetWeaponAssets[weapons[107+role]]
            local actual=SaureksClosetWeaponAssets[real[role]]
            if real[role]>0 and selected and (role==3 or not actual or selected[1]==actual[1]) then routes[role+15]=107+role end
        end
        return routes
    end
    local order={{102,101,103,104,105,106,107},{101,102,104,103,105,106,107},{106,101,102,103,104,105,107}}
    for role=1,3 do
        local actual=SaureksClosetWeaponAssets[real[role]]
        if role==3 and real[role]>0 and weapons and weapons[106] then
            routes[18]=106;used[106]=true
        elseif actual then
            for pass=1,2 do
                if not routes[role+15] then
                    for _,slot in ipairs(order[role]) do
                        local a=SaureksClosetWeaponAssets[(weapons or {})[slot]]
                        if a and not used[slot] and a[1]==actual[1] and (pass==2 or a[2]==actual[2]) then
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
    target.weaponToken=nil;target.weaponPose=nil
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
    if ok and (target==self.model or target==self.previewBuffer) then
        local pose=0
        if self:WeaponPreviewMode()==2 then
            local real=self:RealWeaponItems()
            local asset=SaureksClosetWeaponAssets[(weapons or {})[110] or real[3]]
            if asset then pose=asset[2]==2 and 29 or ((asset[2]==3 or asset[2]==18) and 48 or 108) end
        end
        if target.weaponPose~=pose then target:SetSequence(pose);target.weaponPose=pose end
    end
    return ok
end
