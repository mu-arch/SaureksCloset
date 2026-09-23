-- Saurek's Closet. Lua 5.0 / original WoW 1.12.1. GPL-3.0-or-later.
VanityStudio = { index = {}, slots = {}, applied = {}, pending = {}, errors = {} }
local V = VanityStudio
-- Read from reloaded code; client addon metadata can retain the startup version.
V.VERSION = "3.7.11"
V.UNSAVED = {} -- Runtime key; the single draft itself lives in character saved variables.
V.slotOrder = {1,3,15,4,5,19,9,10,6,7,8,16,17,18}
V.slotNames = {[1]="Head",[3]="Shoulders",[4]="Shirt",[5]="Chest",[6]="Waist",[7]="Legs",[8]="Feet",[9]="Wrists",[10]="Hands",[15]="Back",[16]="Main hand",[17]="Off hand",[18]="Ranged",[19]="Tabard"}
V.inventorySlots = {[1]={1},[3]={3},[4]={4},[5]={5},[6]={6},[7]={7},[8]={8},[9]={9},[10]={10},[13]={16,17},[14]={17},[15]={18},[16]={15},[17]={16},[19]={19},[20]={5},[21]={16},[22]={17},[23]={17},[25]={18},[26]={18}}
V.qualityNames = {[0]="Poor",[1]="Common",[2]="Uncommon",[3]="Rare",[4]="Epic",[5]="Legendary",[6]="Artifact"}
V.colors = {[0]={.62,.62,.62},[1]={.93,.93,.93},[2]={.12,1,.12},[3]={.25,.55,1},[4]={.72,.36,1},[5]={1,.5,0},[6]={.9,.8,.5}}
V.materialNames = {[0]="Other",[1]="Cloth",[2]="Leather",[3]="Mail",[4]="Plate",[6]="Shield"}

-- ItemSubClass.dbc labels, shortened to fit the native filter button.
V.weaponTypeNames = {[0]="1H Axes",[1]="2H Axes",[2]="Bows",[3]="Guns",[4]="1H Maces",[5]="2H Maces",[6]="Polearms",[7]="1H Swords",[8]="2H Swords",[9]="Obsolete",[10]="Staves",[11]="1H Exotics",[12]="2H Exotics",[13]="Fist weapons",[14]="Other",[15]="Daggers",[16]="Thrown",[17]="Spears",[18]="Crossbows",[19]="Wands",[20]="Fishing poles"}
function V:ItemTypeKey(item,slot)
    -- A few old quest/test props occupy weapon slots but carry armor-class metadata.
    if (slot==16 or slot==18) and item[5]~=2 then return 400 end
    return item[5]*100+item[6]
end
function V:ItemTypeName(key,slot)
    if not key then return "All" end
    local class=math.floor(key/100);local sub=math.mod(key,100)
    if class==11 then return sub==2 and "Quivers" or "Ammo pouches" end
    if class==2 then return self.weaponTypeNames[sub] or "Other" end
    if class==4 and sub==0 and slot==17 then return "Held items" end
    return self.materialNames[sub] or "Other"
end
function V:MatchesItemQuality(item,quality)
    if quality=="unobtainable" then return item.unobtainable end
    return not item.unobtainable and (quality==nil or item[4]==quality)
end
function V:ItemBrowseLevel(item)
    if (item[11] or 0)>0 then return item[11] end
    return math.max(1,item[12] or 1)
end
function V:ItemTypeOptions(slot,quality,maxLevel)
    local seen,options={},{}
    local weapon=self.IsWeaponPosition and self:IsWeaponPosition(slot)
    local real=weapon and slot>=108 and self:RealWeaponItems() or nil
    for _,item in ipairs(self.slots[slot] or {}) do
        local key=self:ItemTypeKey(item,slot)
        if (not weapon or self:WeaponChoiceCompatible(item[1],slot,real)) and
            self:MatchesItemQuality(item,quality) and (not maxLevel or self:ItemBrowseLevel(item)<=maxLevel) and not seen[key] then seen[key]=true;table.insert(options,key) end
    end
    table.sort(options);return options
end

function V:Copy(source)
    local result = {}
    for k,v in pairs(source or {}) do result[k] = type(v) == "table" and self:Copy(v) or v end
    return result
end

function V:Message(message)
    if DEFAULT_CHAT_FRAME then DEFAULT_CHAT_FRAME:AddMessage("|cffffcf79Saurek's Closet:|r " .. message) end
end

function V:Available()
    return type(SetUnitVisibleItemID) == "function"
end

function V:Link(id)
    return "item:" .. id .. ":0:0:0"
end

function V:CatalogIcon(id)
    local item=self.index[id]
    local icon=item and (item[10] or VanityStudioItemIcons[item[8]])
    if icon then return "Interface\\Icons\\"..icon end
end

function V:Compatible(id, slot)
    if self:IsWeaponPosition(slot) then return self:WeaponCompatible(id,slot) end
    if id == 0 then return self.slotNames[slot] ~= nil end
    local item = self.index[id]
    if not item then return false end
    for _,s in ipairs(self.inventorySlots[item[3]] or {}) do
        if s == slot then return true end
    end
    return false
end

function V:Initialize()
    VanityStudioDB = VanityStudioDB or {}
    VanityStudioDB.portraitDiagnostics=nil -- Retire the generated-thumbnail diagnostics.
    self.armorHistory=VanityStudioDB.armorHistory or {}
    VanityStudioDB.favorites = VanityStudioDB.favorites or {}
    VanityStudioDB.outfits = VanityStudioDB.outfits or {}
    if VanityStudioDB.hideHigherLevelItems==nil then VanityStudioDB.hideHigherLevelItems=true end
    VanityStudioCharacter = VanityStudioCharacter or {}
    local c = VanityStudioCharacter
    c.selected = c.selected or {}
    c.managed = c.managed or {}
    if c.enabled == nil then c.enabled = true end
    c.useRaceVoice = nil -- Voice now follows the active body automatically.
    for _,slot in ipairs(self.slotOrder) do self.slots[slot] = {} end
    for _,item in ipairs(VanityStudioCatalog) do
        self.index[item[1]] = item
        item.search = string.lower(item[2])
        for _,slot in ipairs(self.inventorySlots[item[3]] or {}) do table.insert(self.slots[slot], item) end
    end
    self:InitializeWeapons()
    for slot,id in pairs(c.selected) do
        if self:IsWeaponPosition(slot) or not self:Compatible(id, slot) then c.selected[slot] = nil end
    end
    for name,outfit in pairs(VanityStudioDB.outfits) do
        if not outfit.slots then VanityStudioDB.outfits[name] = {version=2,slots=self:Copy(outfit)} end
    end
    if c.body then c.body = self:NormalizeBody(c.body) end
    -- First run has no body override. Populate real appearance values as soon
    -- as the player is available, without activating body customization.
    self.trueBody=nil;self.bodyControlValues=nil
    self:RefreshTrueBody()
    self:InitializeBagTuning()
    self.armorLifeState = self:ArmorLifeState()
    self:CheckArmorRepairs()
    if c.outfitDirty then self:TrackUnsaved() end
    self.ready = true
    self:CreateLauncher()
    self:InitializeUpdates()
end

-- Three states: nil removes the override; 0 explicitly hides; positive ID replaces it.
function V:Release(slot)
    if not VanityStudioCharacter.managed[slot] and not self.applied[slot] then return true end
    if not self:Available() then return false end
    local ok,err = pcall(SetUnitVisibleItemID, "player", slot)
    if ok then
        VanityStudioCharacter.managed[slot] = nil
        self.applied[slot] = nil
        self.errors[slot] = nil
        self:RecordArmorEvent("released slot "..slot.."; selected="..tostring(VanityStudioCharacter.selected[slot]).."; enabled="..tostring(VanityStudioCharacter.enabled))
    else
        self.errors[slot] = tostring(err)
    end
    return ok
end

function V:RequestItem(id)
    if not self.queryTip then
        self.queryTip = CreateFrame("GameTooltip", "VanityStudioQueryTooltip", UIParent, "GameTooltipTemplate")
    end
    self.queryTip:SetOwner(UIParent, "ANCHOR_NONE")
    pcall(self.queryTip.SetHyperlink, self.queryTip, self:Link(id))
    self.queryTip:Hide()
end

function V:Sync()
    if not self.ready then return end
    -- Appearance setters can synchronously emit model/inventory events.
    -- Those notifications must not schedule another recovery of our own work.
    self.syncingAppearance = true
    -- The original client can defer UNIT_MODEL_CHANGED until after the helper
    -- returns. Ignore only that short echo of our own writes, not later game
    -- rebuilds that genuinely need repair.
    self.ignoreArmorModelEventsUntil=GetTime()+1
    self:UpdateArmorEquipment()
    local c = VanityStudioCharacter
    self:SyncBody()
    local refreshArmorDisplay=self.refreshArmorDisplay
    if refreshArmorDisplay then
        -- Consume before calling the helper: its model events are synchronous
        -- on some clients, deferred on others. Neither may trigger another pulse.
        self.refreshArmorDisplay = nil
        self:RefreshArmorDisplay()
    end
    for _,slot in ipairs(self.slotOrder) do
        local draft = self.draft and self.draft.slot == slot
        local id = c.selected[slot]
        -- The browser owns this slot until Apply or Cancel, including hidden
        -- and passthrough choices. Background retries must use that same choice.
        if draft then id = self.draft.id end
        -- An already-applied item remains valid if the client temporarily
        -- cannot supply its tooltip data. Do not strip it during combat.
        local cached = id == 0 or (id and (self.applied[slot] == id or GetItemInfo(id)))
        if id and not cached then
            -- Until the new choice loads, release the previous choice and show real gear.
            self:Release(slot)
            local pending = self.pending[slot]
            if not pending or pending.id ~= id then
                pending = {id=id, started=GetTime(), attempts=0, last=-100}
                self.pending[slot] = pending
            end
            if GetTime() - pending.started < 10 then
                if pending.attempts < 3 and GetTime() - pending.last >= 2 then
                    self:RequestItem(id)
                    pending.attempts = pending.attempts + 1
                    pending.last = GetTime()
                end
            else
                self.errors[slot] = "Item data unavailable. Use Retry after the server loads this item."
            end
        else
            self.pending[slot] = nil
            if not id then self.errors[slot] = nil end
        end
        if not id or (not c.enabled and not draft) then
            self:Release(slot)
        elseif cached and self:Available() and (refreshArmorDisplay or self.applied[slot] ~= id) then
            local ok,err = pcall(SetUnitVisibleItemID, "player", slot, id)
            if ok then
                self.applied[slot] = id
                c.managed[slot] = true
                self.pending[slot] = nil
                self.errors[slot] = nil
            else
                self.errors[slot] = tostring(err)
            end
        elseif cached and not self:Available() then
            self.errors[slot] = nil
        end
    end
    self:SyncWeapons()
    self.syncingAppearance = nil
end

function V:Select(slot, id)
    if self:IsWeaponPosition(slot) then return self:SelectWeapon(slot,id) end
    if not self:Compatible(id, slot) then return false end
    self:CancelDraft()
    VanityStudioCharacter.selected[slot] = id
    self:TrackUnsaved()
    self.errors[slot] = nil
    self.pending[slot] = nil
    self:Sync()
    self:Refresh()
    return true
end

function V:ClearSlot(slot)
    if self:IsWeaponPosition(slot) then return self:SelectWeapon(slot,nil) end
    self:CancelDraft()
    VanityStudioCharacter.selected[slot] = nil
    self:TrackUnsaved()
    self.pending[slot] = nil
    self.errors[slot] = nil
    self:Sync()
    self:Refresh()
end

function V:ClearAll()
    self:CancelDraft()
    VanityStudioCharacter.selected = {}
    VanityStudioCharacter.weapons = {}
    self.editingBody=nil
    VanityStudioCharacter.body = nil
    VanityStudioCharacter.activeOutfit = nil
    VanityStudioCharacter.activeUnsaved = nil
    VanityStudioCharacter.outfitDirty = nil
    self.pending = {}
    self.errors = {}
    self:Sync()
    self:Refresh()
end

function V:SetEnabled(enabled)
    self:CancelDraft()
    VanityStudioCharacter.enabled = enabled and true or false
    self:InvalidatePreviewModel(.25,true)
    self.pending = {}
    self.errors = {}
    self:Sync()
    self:Refresh()
end

function V:Retry()
    self.previewRequests={};self.previewSignature=nil
    self.pending = {}
    self.errors = {}
    self:Sync()
    self:Refresh()
end

function V:OutfitNames()
    local names = {}
    for name,_ in pairs(VanityStudioDB.outfits) do table.insert(names,name) end
    table.sort(names)
    -- Older saved looks predate timestamp tracking. Give them stable migration
    -- times once so future sorting does not jump around between refreshes.
    local now=time and time() or 0
    for i,name in ipairs(names) do
        local outfit=VanityStudioDB.outfits[name]
        if not outfit.updatedAt then outfit.updatedAt=now-i end
    end
    table.sort(names,function(a,b)
        local left=VanityStudioDB.outfits[a].updatedAt or 0
        local right=VanityStudioDB.outfits[b].updatedAt or 0
        if left==right then return string.lower(a)<string.lower(b) end
        return left>right
    end)
    return names
end

function V:CurrentLook()
    local c=VanityStudioCharacter
    return {version=3,slots=self:Copy(c.selected),weapons=self:Copy(c.weapons),body=c.body and self:Copy(c.body)}
end
function V:TrackUnsaved()
    local c=VanityStudioCharacter
    local base=c.activeUnsaved and c.unsaved and c.unsaved.baseName or c.activeOutfit
    c.unsaved=self:CurrentLook();c.unsaved.baseName=base
    c.activeOutfit=nil;c.activeUnsaved=true;c.outfitDirty=true;self.outfitOffset=0;self.unsavedLookMessage=nil
end
function V:OutfitKeys()
    local keys=self:OutfitNames()
    if VanityStudioCharacter.unsaved then table.insert(keys,1,self.UNSAVED) end
    return keys
end
function V:GetOutfit(key)
    if key==self.UNSAVED then return VanityStudioCharacter.unsaved end
    return VanityStudioDB.outfits[key]
end
function V:OutfitLabel(key)
    return key==self.UNSAVED and "(Unsaved Look)" or key
end
function V:IsOutfitActive(key)
    local c=VanityStudioCharacter
    if key==self.UNSAVED then return c.activeUnsaved end
    return not c.activeUnsaved and key==c.activeOutfit
end
function V:ValidateOutfitName(name,existing)
    name=string.gsub(string.gsub(name or "","^%s+",""),"%s+$","")
    if name=="" then return nil,"Enter a name for this outfit." end
    if string.len(name)>40 then return nil,"Use 40 characters or fewer." end
    if string.lower(name)=="(unsaved)" or string.lower(name)=="(unsaved look)" then return nil,"Choose a name for your saved outfit." end
    if VanityStudioDB.outfits[name] and name~=existing then return nil,"That name is already used." end
    return name
end
function V:SaveOutfit(name,replace,key)
    local clean,err=self:ValidateOutfitName(name,replace and name or nil)
    if not clean then return false,err end
    local look=key and self:GetOutfit(key) or self:CurrentLook()
    if not look then return false,"This outfit no longer exists." end
    local c=VanityStudioCharacter
    local active=not key or self:IsOutfitActive(key)
    VanityStudioDB.outfits[clean]={version=3,slots=self:Copy(look.slots),weapons=self:Copy(look.weapons),body=look.body and self:Copy(look.body),updatedAt=time and time() or 0}
    if key==self.UNSAVED or (not key and c.activeUnsaved) then c.unsaved=nil end
    if active then c.activeOutfit=clean;c.activeUnsaved=nil;c.outfitDirty=nil end
    self:Refresh();return true,clean
end
function V:RenameOutfit(old,name)
    if old==self.UNSAVED or not self:GetOutfit(old) then return false,"Select a saved outfit." end
    local clean,err=self:ValidateOutfitName(name,old)
    if not clean then return false,err end
    if clean~=old then
        VanityStudioDB.outfits[clean]=VanityStudioDB.outfits[old];VanityStudioDB.outfits[old]=nil
        local c=VanityStudioCharacter
        if c.activeOutfit==old then c.activeOutfit=clean end
        if c.unsaved and c.unsaved.baseName==old then c.unsaved.baseName=clean end
    end
    VanityStudioDB.outfits[clean].updatedAt=time and time() or 0
    self:Refresh();return true,clean
end
function V:DeleteOutfit(key)
    if not self:GetOutfit(key) then return false end
    local c=VanityStudioCharacter
    if key==self.UNSAVED then
        c.unsaved=nil
        if c.activeUnsaved then self:ClearAll() end
    else
        if self:IsOutfitActive(key) then self:TrackUnsaved() end
        VanityStudioDB.outfits[key]=nil
    end
    self:Refresh();return true
end
function V:LoadOutfit(name)
    local outfit=self:GetOutfit(name)
    if not outfit then return false end
    if next(outfit.weapons or {}) and not self:WeaponRendererAvailable() then self:Message("Update SaureksCloset.dll and restart WoW before loading this look.");return false end
    if outfit.weapons and outfit.weapons.backBag and not self:BagRendererAvailable() then self:Message("Update SaureksCloset.dll and restart WoW before loading a look with visible bags.");return false end
    self:CancelDraft()
    local selected={}
    for slot,id in pairs(outfit.slots or outfit) do
        if not self:IsWeaponPosition(slot) and self:Compatible(id,slot) then selected[slot]=id end
    end
    local c=VanityStudioCharacter;local previousBody=c.body
    c.body=outfit.body and self:NormalizeBody(outfit.body)
    if c.body and not self:BodyAvailable() then c.body=previousBody;self:Message("The race renderer is missing. Restart the game before loading this combo.");return false end
    if self:SyncBody()==false then c.body=previousBody;self:Message(self.bodyError);return false end
    self.editingBody=nil;self:InvalidatePreviewModel(.25,true)
    c.selected=selected;c.weapons=self:NormalizeWeapons(outfit.weapons)
    self:MigrateEquippedWeaponOverrides(c.selected,c.weapons)
    c.activeUnsaved=name==self.UNSAVED and true or nil
    c.activeOutfit=name~=self.UNSAVED and name or nil;c.outfitDirty=c.activeUnsaved
    self.pending={};self.errors={};self:Sync();self:Refresh();return true
end

function V:CycleOutfit(direction)
    local names=self:OutfitNames()
    if table.getn(names)==0 then self:Message("Save a look in the Outfits tab first.");return end
    local index=direction>0 and 0 or 1
    for i,name in ipairs(names) do if name==VanityStudioCharacter.activeOutfit then index=i end end
    index=math.mod(index-1+direction,table.getn(names))+1
    if index<1 then index=table.getn(names) end
    self:LoadOutfit(names[index])
end

function V:HideArmor()
    self:CancelDraft()
    for _,slot in ipairs(self.slotOrder) do
        if slot<16 or slot==19 then VanityStudioCharacter.selected[slot]=0 end
    end
    self:TrackUnsaved()
    self:Sync(); self:Refresh()
end

function V:Filter(slot, query, quality, material, favorites, maxLevel)
    local result = {}
    local weapon=self.IsWeaponPosition and self:IsWeaponPosition(slot)
    local real=weapon and slot>=108 and self:RealWeaponItems() or nil
    query = string.lower(query or "")
    for _,item in ipairs(self.slots[slot] or {}) do
        local match = (not weapon or self:WeaponChoiceCompatible(item[1],slot,real)) and self:MatchesItemQuality(item,quality) and
            (not maxLevel or self:ItemBrowseLevel(item)<=maxLevel) and
            (material == nil or self:ItemTypeKey(item,slot) == material) and
            (not favorites or VanityStudioDB.favorites[item[1]])
        if match then
            for word in string.gfind(query, "%S+") do
                if not string.find(item.search, word, 1, true) and word ~= tostring(item[1]) and word ~= "#"..item[1] then match = false end
            end
        end
        if match then table.insert(result, item) end
    end
    return result
end

BINDING_HEADER_VANITYSTUDIO = "Saurek's Closet"
BINDING_NAME_VANITYSTUDIO_TOGGLE = "Open wardrobe"
SLASH_VANITYSTUDIO1 = "/closet"
SLASH_VANITYSTUDIO2 = "/vc"
SLASH_VANITYSTUDIO3 = "/vanity"
SLASH_VANITYSTUDIO4 = "/vs"
SlashCmdList["VANITYSTUDIO"] = function(msg)
    msg = string.lower(msg or "")
    if not V.ready then V:Message("Please wait until your character has loaded."); return end
    if msg == "reset" then V:ClearAll(); V:Message("Appearance overrides cleared.")
    elseif msg == "next" then V:CycleOutfit(1)
    elseif msg == "prev" then V:CycleOutfit(-1)
    elseif msg == "body" then V:Toggle(true); V:SetTab("body")
    elseif msg == "outfits" then V:Toggle(true); V:SetTab("outfits")
    elseif msg == "off" then V:SetEnabled(false); V:Message("Local appearances disabled.")
    elseif msg == "on" then V:SetEnabled(true)
    elseif msg == "retry" then V:Retry()
    elseif msg == "weaponscan" then V:StartWeaponryCapture()
    elseif msg == "bagtune" or msg == "bagtuner" or msg == "bagdebug" then V:OpenBagTuner()
    elseif msg == "diagnose" then V:Diagnose()
    elseif msg == "armorlog" then
        for _,line in ipairs(V.armorHistory or {}) do V:Message(line) end
    elseif msg == "updates" then V:Toggle(true);V:OpenInfoPage("updates")
    elseif msg == "home" then V:Toggle(true)
    else V:Toggle() end
end

function V:UpdateArmorEquipment()
    -- Actual equipped items are independent of appearance-only notifications.
    local previous = self.armorEquipment
    local equipment = {}
    local changed = false
    for _,slot in ipairs(self.slotOrder) do
        local link = GetInventoryItemLink("player", slot)
        equipment[slot] = link or false
        if previous and previous[slot] ~= equipment[slot] then
            self.applied[slot] = nil
            changed = true
        end
    end
    self.armorEquipment = equipment
    return changed
end

function V:RecordArmorEvent(message)
    self.armorHistory = self.armorHistory or {}
    table.insert(self.armorHistory,string.format("%.1f %s",GetTime(),message))
    if table.getn(self.armorHistory)>30 then table.remove(self.armorHistory,1) end
    if VanityStudioDB then VanityStudioDB.armorHistory=self.armorHistory end
end

function V:CheckArmorRepairs()
    if self.syncingAppearance or (UnitExists and not UnitExists("player")) then return end
    -- Original 1.12 has no GetInventoryItemDurability. Its inventory tooltip
    -- returns the real item's repair cost, independently of the visible ID.
    if not self.repairTip then
        self.repairTip=CreateFrame("GameTooltip","SaureksClosetRepairTooltip",UIParent,"GameTooltipTemplate")
    end
    local previous=self.armorRepairCosts or {}
    local current={}
    local repaired=false
    for _,slot in ipairs(self.slotOrder) do
        local link=GetInventoryItemLink("player",slot)
        if link then
            self.repairTip:SetOwner(UIParent,"ANCHOR_NONE")
            self.repairTip:ClearLines()
            local hasItem,hasCooldown,cost=self.repairTip:SetInventoryItem("player",slot)
            self.repairTip:Hide()
            local before=previous[slot]
            if hasItem and type(cost)=="number" then
                current[slot]={link=link,cost=cost}
                -- Only a completed repair qualifies. Damage, vendor discounts,
                -- missing item data and swapping equipment must not rebuild us.
                if before and before.link==link and before.cost>0 and cost==0 then repaired=true end
            elseif before and before.link==link then current[slot]=before end
        end
    end
    self.armorRepairCosts=current
    if repaired and VanityStudioCharacter.enabled then
        self:QueueArmorCheck("equipment repaired")
        -- A confirmed repair is not an echo of our own model setter. It must
        -- recover even immediately after choosing an appearance or drawing.
        self.armorCheckRefreshDisplay=true
        self.armorCheckReason="equipment repaired"
    end
end

function V:QueueArmorCheck(reason,refreshDisplay)
    if self.syncingAppearance or not VanityStudioCharacter.enabled then return end
    if refreshDisplay and self.ignoreArmorModelEventsUntil and
        GetTime()<self.ignoreArmorModelEventsUntil then return end
    if refreshDisplay then
        self.armorCheckRefreshDisplay=true
        self.armorCheckReason=reason
    end
    -- Keep the first deadline so a stream of combat notifications cannot
    -- postpone the check indefinitely. No timer runs when there are no events.
    if not self.armorCheckAt then
        self.armorCheckAt=GetTime()+.25
        self.armorCheckReason=self.armorCheckReason or reason
        local inCombat=UnitAffectingCombat and UnitAffectingCombat("player")
        local renderer=""
        if type(SaureksClosetInspect)=="function" then
            local inspection={pcall(SaureksClosetInspect)}
            if inspection[1] and inspection[2]==3 then
                -- revision/composed/reloads/detail-updates identify whether
                -- our native body renderer initiated work around this event.
                renderer="; renderer="..tostring(inspection[5]).."/"..tostring(inspection[6])..
                    "/"..tostring(inspection[7]).."/"..tostring(inspection[8])
            end
        end
        self:RecordArmorEvent("check queued: "..reason.."; refresh="..tostring(refreshDisplay and true or false).."; combat="..tostring(inCombat and true or false)..renderer)
    end
end

function V:UpdateArmorCheck()
    if not self.armorCheckAt or GetTime()<self.armorCheckAt or self.respawnRecovery then return end
    if UnitExists and not UnitExists("player") then return end
    self.armorCheckAt=nil
    local reason=self.armorCheckReason or "unknown"
    local refreshDisplay=self.armorCheckRefreshDisplay
    self.armorCheckReason=nil;self.armorCheckRefreshDisplay=nil
    local c=VanityStudioCharacter
    if not c.enabled or not self:Available() then return end
    if refreshDisplay then
        -- A client model rebuild can reset rendered armor while leaving the
        -- virtual item fields unchanged. Reasserting equal IDs is therefore a
        -- no-op in the helper. Sync performs one temporary field change and
        -- restores the complete outfit, which rebuilds the stale visuals once.
        self.refreshArmorDisplay=true
        self:RecordArmorEvent("display refresh requested after "..reason)
        -- Run the nudge and complete restore in this one Lua update. There is
        -- no intervening rendered frame and no periodic follow-up worker.
        self:Sync()
        return
    end
    -- Inventory/model updates may overwrite visible item fields without an
    -- equipment-link change. The Lua applied cache cannot detect that. Let the
    -- helper compare the real fields: equal IDs do not rebuild the model.
    self.syncingAppearance=true
    self.ignoreArmorModelEventsUntil=GetTime()+1
    local checked=0
    local requested={}
    for _,slot in ipairs(self.slotOrder) do
        local id=c.selected[slot]
        if id~=nil and not (self.draft and self.draft.slot==slot) and
            (id==0 or self.applied[slot]==id or GetItemInfo(id)) then
            local ok,err=pcall(SetUnitVisibleItemID,"player",slot,id)
            if ok then
                self.applied[slot]=id;c.managed[slot]=true;self.errors[slot]=nil
                checked=checked+1
                table.insert(requested,slot.."="..id)
            else
                self.applied[slot]=nil;self.errors[slot]=tostring(err)
                self:RecordArmorEvent("slot "..slot.." failed: "..tostring(err))
            end
        end
    end
    self.syncingAppearance=nil
    self:RecordArmorEvent("checked "..checked.." armor overrides: "..table.concat(requested,", "))
end

function V:ArmorLifeState()
    if UnitExists and not UnitExists("player") then return nil end
    if UnitIsGhost("player") then return "ghost" end
    if UnitIsDeadOrGhost("player") then return "dead" end
    return "alive"
end

function V:RefreshArmorDisplay()
    local c = VanityStudioCharacter
    if not c.enabled or not self:Available() then return end
    -- The helper compares item FIELDS, not the rendered equipment. After a
    -- ghost/resurrection rebuild the fields can still match while the model
    -- wears real armor, so reasserting identical IDs does nothing. Change one
    -- managed field, then let Sync restore it in this same frame. This forces
    -- one refresh pair for the whole outfit, not an off/on pass over every slot.
    for _,slot in ipairs(self.slotOrder) do
        local id = c.selected[slot]
        if self.draft and self.draft.slot==slot then id=self.draft.id end
        if id and (id == 0 or self.applied[slot] == id or GetItemInfo(id)) then
            local temporary = 0
            local usable = id > 0
            if id == 0 then
                -- An all-hidden outfit needs an occupied slot to release.
                usable = GetInventoryItemLink("player",slot) ~= nil
                temporary = nil
            end
            if usable then
                local ok,err = pcall(SetUnitVisibleItemID,"player",slot,temporary)
                if not ok then self.errors[slot] = tostring(err) end
                return
            end
        end
    end
end

function V:QueueRespawnRecovery(reason)
    local previous = self.respawnRecovery
    local state = self:ArmorLifeState()
    local changed = state and self.armorLifeState and state ~= self.armorLifeState
    self.armorLifeState = state or self.armorLifeState
    self.respawnRecovery = {
        next = GetTime() + .25,
        refreshDisplay = changed or (reason ~= nil and reason ~= "PLAYER_ENTERING_WORLD") or
            (previous and previous.refreshDisplay),
        waitFor = reason == "PLAYER_DEAD" and "dead" or
            reason == "PLAYER_UNGHOST" and "alive" or
            reason == "PLAYER_ALIVE" and "released" or (previous and previous.waitFor)
    }
    if reason == "state" then self.respawnRecovery.waitFor = nil end
end

function V:UpdateRespawnRecovery()
    local state = self:ArmorLifeState()
    if not state then return end
    if self.armorLifeState and self.armorLifeState ~= state then
        -- Release can report PLAYER_ALIVE before the ghost flag changes.
        -- Watch the actual state, without writing to the model on each tick.
        self:QueueRespawnRecovery("state")
    end
    self.armorLifeState = state
    local recovery = self.respawnRecovery
    if not recovery then return end
    if recovery.waitFor == "released" and state == "dead" then return end
    if recovery.waitFor == "dead" and state ~= "dead" then return end
    if recovery.waitFor == "alive" and state ~= "alive" then return end
    if GetTime() < recovery.next then return end
    -- Consume before applying: notifications from the helper cannot re-arm it.
    -- World entry reasserts IDs; lifecycle transitions also refresh stale textures.
    self.respawnRecovery = nil
    self.armorCheckAt = nil -- This application already includes all armor fields.
    self.armorCheckReason=nil;self.armorCheckRefreshDisplay=nil
    self.refreshArmorDisplay = recovery.refreshDisplay
    self.applied = {}
    self.needsSync = true
end

V.events = CreateFrame("Frame")
V.events:RegisterEvent("ADDON_LOADED")
V.events:RegisterEvent("PLAYER_ENTERING_WORLD")
V.events:RegisterEvent("UNIT_INVENTORY_CHANGED")
V.events:RegisterEvent("BAG_UPDATE")
V.events:RegisterEvent("UNIT_MODEL_CHANGED")
V.events:RegisterEvent("UNIT_PORTRAIT_UPDATE")
V.events:RegisterEvent("UNIT_LEVEL")
V.events:RegisterEvent("PLAYER_DEAD")
V.events:RegisterEvent("PLAYER_ALIVE")
V.events:RegisterEvent("PLAYER_UNGHOST")
V.events:RegisterEvent("UPDATE_INVENTORY_ALERTS")
V.events:RegisterEvent("MERCHANT_SHOW")
V.events:RegisterEvent("MERCHANT_UPDATE")
V.events:RegisterEvent("MERCHANT_CLOSED")
V.events:SetScript("OnEvent", function()
    if event == "ADDON_LOADED" and arg1 == "SaureksCloset" then V:Initialize()
    elseif V.ready and (event == "PLAYER_DEAD" or event == "PLAYER_ALIVE" or event == "PLAYER_UNGHOST") then
        V:QueueRespawnRecovery(event)
    elseif V.ready and event == "MERCHANT_SHOW" then
        V:CheckArmorRepairs()
    elseif V.ready and (event == "UPDATE_INVENTORY_ALERTS" or event == "MERCHANT_UPDATE" or event == "MERCHANT_CLOSED") then
        V.armorRepairCheckPending=true
    elseif V.ready and event == "BAG_UPDATE" then
        local quiver=V:RealQuiverItem()
        if V.lastEquippedQuiver~=quiver then
            V.lastEquippedQuiver=quiver;V.needsSync=true
            if V.outfitDetails and V.outfitDetails:IsShown() then V:StartOutfitPreview() end
        end
    elseif V.ready and event == "UNIT_LEVEL" and arg1 == "player" then
        if V.browser and V.browser:IsShown() then V:RefreshList() end
    elseif V.ready and event == "UNIT_PORTRAIT_UPDATE" and arg1 == "player" then
        V:RefreshPortraits()
    elseif V.ready and event == "UNIT_MODEL_CHANGED" and arg1 == "player" then
        -- Weapon attachment updates also emit this event. Treat it as a field
        -- check, not evidence that the whole character needs rebuilding. The
        -- explicit death/release/resurrection recovery still repairs textures.
        V:QueueArmorCheck(event)
        local recovery = V.respawnRecovery
        if recovery and not V.syncingAppearance then
            recovery.next = GetTime() + .25
        end
        V:RefreshTrueBody();V:RefreshBody()
        V:RefreshPortraits();V:RefreshPreviewForModelEvent()
    elseif V.ready and (event == "PLAYER_ENTERING_WORLD" or (event == "UNIT_INVENTORY_CHANGED" and arg1 == "player")) then
        if event == "PLAYER_ENTERING_WORLD" then
            V:CheckArmorRepairs()
            V.appliedRace = nil;V.bagTunerSynced={};V:RefreshTrueBody();V:RefreshBody()
            V:RefreshPortraits();V:InvalidatePreviewModel(.25,true)
            V:QueueRespawnRecovery(event)
        elseif not V.syncingAppearance then
            V.armorRepairCheckPending=true
            if V.respawnRecovery then V.respawnRecovery.next = GetTime() + .25 end
            if V:UpdateArmorEquipment() then V.needsSync = true
            else V:QueueArmorCheck(event) end
        end
        if V.outfitDetails and V.outfitDetails:IsShown() then V:StartOutfitPreview() end
    end
end)
local elapsed = 0
V.events:SetScript("OnUpdate", function()
    if not V.ready then return end
    V:UpdateUpdates()
    if V.weaponryCapture then V:UpdateWeaponryCapture() end
    V:UpdatePreviewLoading()
    elapsed = elapsed + arg1
    if elapsed < .5 then return end
    elapsed = 0
    if V.armorRepairCheckPending then
        V.armorRepairCheckPending=nil
        V:CheckArmorRepairs()
    end
    V:UpdateRespawnRecovery()
    if V.trueBodyPending and V:BodyAvailable() and V:RefreshTrueBody() then
        V:RefreshBody()
    end
    local pending = false
    for slot,p in pairs(V.pending) do
        if GetTime() - p.started < 11 or GetItemInfo(p.id) then pending = true end
    end
    if (V.needsSync or pending) and not V.respawnRecovery then
        V.needsSync = false
        V:Sync()
        V:Refresh()
    end
    V:UpdateArmorCheck()
    if VanityStudioCharacter.enabled or next(VanityStudioCharacter.weapons or {}) then V:SyncWeapons() end
end)
