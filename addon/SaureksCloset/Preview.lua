-- Preview transactions never call the world-appearance helper.
local V=VanityStudio
function V:DraftSlot(slot,id)
    if not self.slotNames[slot] or (id~=nil and not self:Compatible(id,slot)) then return false end
    self.draft={slot=slot,id=id}
    self:Refresh()
    return true
end
function V:CancelDraft()
    self.draft=nil;self.previewWaiting=nil
end
function V:CommitDraft()
    local draft=self.draft
    if not draft then return false end
    self:CancelDraft()
    if draft.id==nil then self:ClearSlot(draft.slot) else self:Select(draft.slot,draft.id) end
    return true
end
function V:PreviewItems()
    local items={}
    for _,slot in ipairs(self.slotOrder) do
        local id
        if VanityStudioCharacter.enabled then id=VanityStudioCharacter.selected[slot] end
        if self.draft and self.draft.slot==slot then id=self.draft.id end
        if id==nil then
            local link=GetInventoryItemLink("player",slot)
            if link then local _,_,number=string.find(link,"item:(%d+)");id=tonumber(number) end
        end
        items[slot]=id or 0
    end
    return items
end
-- A live player rebuild and a dressing-room clone can finish on different frames.
-- Independent previews use native readiness; older bridges retain timed recovery.
function V:InvalidatePreviewModel(delay,recover)
    local now=GetTime()
    local tracked=self:WeaponRendererAvailable()
    self.previewBaseDirty=true;self.previewCaptureAt=now+(tracked and 0 or (delay or 0))
    self.previewReveal=nil;self.previewDeadline=now+8
    self.previewDressAt=nil;self.previewDressingModel=nil;self.previewSignature=nil
    -- The new bridge tracks composition explicitly. A timed second copy replaces
    -- an already-correct character and restarts its animation for no reason.
    tracked=tracked or type(SaureksClosetInspectPreview)=="function"
    self.previewRecoveries=recover and not tracked and {now+1.5} or nil
end
function V:PreviewBodyKey()
    local c=VanityStudioCharacter;local key=c.enabled and "enabled" or "disabled"
    local b=c.enabled and c.body or self:NativeBody()
    if b then
        key=key..":"..b.race..":"..b.sex
        for _,name in ipairs(self.bodyKeys) do key=key..":"..b[name] end
    end
    return key
end
-- Check current composition after dressing as well as after SetUnit.
function V:PreviewModelReady(target,after)
    if after and GetTime()<=after then return false end
    if not target.weaponToken then return true end
    local ok,status=pcall(SaureksClosetPreviewStatus,target.weaponToken)
    if not ok or status~=1 then return false end
    if type(SaureksClosetInspectPreview)=="function" then
        local details={pcall(SaureksClosetInspectPreview,target.weaponToken)}
        if not details[1] or details[2]~=1 or (type(details[11])=="number" and details[11]~=0) then return false end
    end
    return true
end
function V:RefreshPreviewForModelEvent()
    -- Independent previews already contain their requested body. World armor
    -- rebuild notifications must not restart their model/animation.
    if self:WeaponRendererAvailable() then self:RefreshPreview()
    else self:InvalidatePreviewModel(.25,true) end
end
function V:HidePreviewUntilReady()
    if not self.uiReady then return end
    self.model:SetAlpha(0);self.previewBuffer:SetAlpha(0)
    self.previewNote:SetText("Loading preview...")
end
function V:RefreshPreview()
    if not self.model or not self.model:IsVisible() then return end
    local key=self:PreviewBodyKey()
    if key~=self.previewRequestedBodyKey then
        self.previewRequestedBodyKey=key;self:InvalidatePreviewModel(.25,true)
    end
    local now=GetTime()
    if self.previewBaseDirty then
        if now<(self.previewCaptureAt or 0) then return end
        local ok=pcall(function()
            V.previewBuffer:SetAlpha(0)
            V:CopyWardrobeModel(V.previewBuffer)
            V.previewBuffer:SetRotation(V.model.rotation or .61)
            V.previewDressingModel=V.previewBuffer
        end)
        self.previewBaseDirty=nil
        if not ok then
            self.previewError="Preview unavailable. Close and reopen the wardrobe."
            self.previewNote:SetText(self.previewError);self.model:SetAlpha(1);return
        end
        self.previewDressAt=now+(self.previewBuffer.weaponToken and 0 or .1);self.previewSignature=nil
        return
    end
    if self.previewDressAt and now<self.previewDressAt then return end
    local items=self:PreviewItems();local weapons=self:PreviewWeapons()
    local routes=self:PreviewWeaponRoutes(weapons)
    local signature=key..self:WeaponSignature(weapons)
    for _,slot in ipairs(self.slotOrder) do signature=signature..":"..items[slot] end
    if self.previewReveal and self.previewReveal.signature==signature then
        local reveal=self.previewReveal
        if now>(self.previewDeadline or now+8) then
            self.previewReveal=nil;self.previewDressAt=nil;self.previewDressingModel=nil
            self.previewNote:SetText("Preview could not finish loading. Close and reopen the wardrobe.");return
        end
        if not self:PreviewModelReady(reveal.target,reveal.at) then return end
        local ok,ready=pcall(self.DressWeaponPlacements,self,reveal.target,weapons)
        if not ok or not ready then return end
        local previous=self.model;local target=reveal.target
        target.rotation=previous.rotation or .61;target:SetRotation(target.rotation)
        if target~=previous then previous:SetAlpha(0);self.model=target;self.previewBuffer=previous end
        target:SetAlpha(1)
        self.previewSignature=signature;self.previewError=nil;self.previewReveal=nil;self.previewDressAt=nil;self.previewDressingModel=nil
        self.previewNote:SetText(next(self.previewWaiting or {}) and "Some item data is unavailable." or "")
        return
    end
    if signature==self.previewSignature and not self.previewDressingModel and not self.previewReveal then return end
    local target=self.previewDressingModel or self.model
    if target.weaponToken and not self:PreviewModelReady(target) then
        if now>(self.previewDeadline or now+8) then self.previewDressAt=nil;self.previewDressingModel=nil;self.previewNote:SetText("Preview could not finish loading. Close and reopen the wardrobe.") end
        return
    end
    self.previewReveal=nil
    local ok=pcall(function()
        -- Reuse the finished clone for item previews; do not recreate it per item.
        target:Undress()
        V.previewWaiting={}
        for _,slot in ipairs(V.slotOrder) do
            local id=items[slot]
            if id>0 and not routes[slot] then
                target:TryOn(tostring(id))
                if not GetItemInfo(id) then
                    V.previewWaiting[id]=true
                    local request=V.previewRequests[id]
                    if not request or (request.attempts<3 and GetTime()-request.last>=2) then
                        V:RequestItem(id)
                        V.previewRequests[id]={last=GetTime(),attempts=request and request.attempts+1 or 1}
                    end
                end
            end
        end
        V:DressWeaponPlacements(target,weapons)
        target:SetRotation(V.model.rotation or .61)
    end)
    if ok and target.weaponToken then
        self.previewReveal={target=target,signature=signature,at=now}
        self.previewDressAt=now;self.previewDeadline=now+8
        return
    end
    self.previewDressAt=nil;self.previewDressingModel=nil
    if ok and target~=self.model then
        local previous=self.model;target.rotation=previous.rotation or .61
        target:SetAlpha(1);previous:SetAlpha(0)
        self.model=target;self.previewBuffer=previous
    end
    if ok then self.previewSignature=signature;self.previewError=nil;self:RefreshPortraits()
    else self.previewError="Preview unavailable. Close and reopen the wardrobe." end
    if self.previewError then self.previewNote:SetText(self.previewError)
    elseif next(self.previewWaiting or {}) then self.previewNote:SetText("Some item data is unavailable.\nThe preview may be incomplete.")
    else self.previewNote:SetText("") end
end
function V:UpdatePreviewLoading()
    self:UpdateOutfitPreview()
    if not self.model or not self.model:IsVisible() then return end
    if self.previewBaseDirty or self.previewDressAt then self:RefreshPreview();return end
    if self.previewRecoveries and self.previewRecoveries[1] and GetTime()>=self.previewRecoveries[1] then
        table.remove(self.previewRecoveries,1)
        self.previewBaseDirty=true;self.previewCaptureAt=GetTime();self.previewSignature=nil
        self:RefreshPreview();return
    end
    for id,_ in pairs(self.previewWaiting or {}) do
        local request=self.previewRequests[id]
        if GetItemInfo(id) or (request and request.attempts<3 and GetTime()-request.last>=2) then
            -- A late item response must not undress and rebuild the whole outfit.
            pcall(self.model.TryOn,self.model,tostring(id))
            if GetItemInfo(id) then self.previewWaiting[id]=nil
            else
                self:RequestItem(id)
                self.previewRequests[id]={last=GetTime(),attempts=request.attempts+1}
            end
        end
    end
    if self.previewWaiting and not next(self.previewWaiting) and not self.previewError then self.previewNote:SetText("") end
end
