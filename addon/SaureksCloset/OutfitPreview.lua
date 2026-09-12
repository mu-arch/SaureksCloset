-- An independent UI model: selecting an outfit never applies it to the world.
local V=VanityStudio
function V:StartOutfitPreview()
    if not self.outfitDetails or not self.outfitDetails:IsShown() then return end
    local look=self:GetOutfit(self.detailKey)
    if not look then return end
    if next(look.weapons or {}) and not self:WeaponRendererAvailable() then
        self.outfitModel:SetAlpha(0);self.outfitBuffer:SetAlpha(0);self.detailPending=nil
        self.detailPreviewNote:SetText("Update SaureksCloset.dll and restart WoW to preview weapon placements.");return
    end
    local b=look.body and self:NormalizeBody(look.body) or self:NativeBody()
    local bodyKey=b and (b.race..":"..b.sex..":"..b.skin..":"..b.face..":"..b.hairStyle..":"..b.hairColor..":"..b.facial) or "pending"
    local signature=bodyKey..self:WeaponSignature(look.weapons or {},look.slots)
    local items=self:OutfitPreviewItems(look)
    for _,slot in ipairs(self.slotOrder) do signature=signature..":"..items[slot] end
    if self.detailPending and self.detailPending.signature==signature then return end
    if self.detailPreviewSignature==signature then return end
    local reuse=self.detailPreviewBodyKey==bodyKey and self.outfitModel.weaponToken
    self.outfitBuffer:SetAlpha(0)
    if not reuse then self.outfitModel:SetAlpha(0) end
    self.detailMissing=nil
    self.detailPending={look=self:Copy(look),phase=reuse and "dress" or "copy",target=reuse and self.outfitModel or self.outfitBuffer,signature=signature,bodyKey=bodyKey,deadline=GetTime()+8}
    self.detailPreviewNote:SetText("Loading outfit...")
end
function V:OutfitPreviewItems(look)
    local items={}
    for _,slot in ipairs(self.slotOrder) do
        local id=look.slots[slot]
        if id==nil then
            local link=GetInventoryItemLink("player",slot)
            if link then local _,_,number=string.find(link,"item:(%d+)");id=tonumber(number) end
        end
        items[slot]=id or 0
    end
    return items
end
function V:DressOutfitPreview(target,look)
    target:Undress();self.detailMissing={}
    local routes=self:PreviewWeaponRoutes(look.weapons,look.slots)
    for slot,id in pairs(self:OutfitPreviewItems(look)) do
        if id>0 and not routes[slot] then
            target:TryOn(tostring(id))
            if not GetItemInfo(id) then self.detailMissing[id]=true;self:RequestItem(id) end
        end
    end
    self:DressWeaponPlacements(target,look.weapons,look.slots)
    target:SetRotation(self.outfitModel.rotation or .61)
end
function V:UpdateOutfitPreview()
    if not self.outfitDetails or not self.outfitDetails:IsVisible() then return end
    local pending=self.detailPending
    if pending then
        if GetTime()>pending.deadline then
            self.detailPending=nil;self.detailPreviewNote:SetText("Preview could not finish loading. Close and reopen this outfit.");return
        end
        if pending.phase=="copy" then
            if type(SaureksClosetBeginPreview)~="function" or type(SaureksClosetEndPreview)~="function" or type(SaureksClosetPreviewStatus)~="function" then
                self.detailPending=nil;self.detailPreviewNote:SetText("Restart WoW with the updated SaureksCloset.dll to preview saved bodies.");return
            end
            local b=pending.look.body or self:NativeBody()
            if not b then return end
            local ok,status=pcall(SaureksClosetBeginPreview,b.race,b.sex,b.skin,b.face,b.hairStyle,b.hairColor,b.facial)
            if not ok or status~=1 then return end
            local copied=pcall(self.outfitBuffer.SetUnit,self.outfitBuffer,"player")
            -- Always end the scope, including Lua API errors. It cannot affect later SetUnit calls.
            local ended,token=pcall(SaureksClosetEndPreview)
            if not copied or not ended or not token or token<1 then
                self.detailPending=nil;self.detailPreviewNote:SetText("Outfit preview unavailable. Close and reopen this outfit.");return
            end
            pending.phase="compose";pending.token=token;return
        end
        if pending.phase=="reveal" then
            if not self:PreviewModelReady(pending.target,pending.dressedAt) then return end
            local ok,ready=pcall(self.DressWeaponPlacements,self,pending.target,pending.look.weapons,pending.look.slots)
            if not ok or not ready then return end
            local target=pending.target;local previous=self.outfitModel
            target.rotation=previous.rotation or .61;target:SetRotation(target.rotation)
            if target~=previous then previous:SetAlpha(0);self.outfitModel=target;self.outfitBuffer=previous end
            target:SetAlpha(1);self.detailPending=nil
            self.detailPreviewSignature=pending.signature;self.detailPreviewBodyKey=pending.bodyKey
            self.detailPreviewLook=pending.look;self.detailItemRetries=0;self.detailRetryAt=GetTime()+2
            self.detailPreviewNote:SetText(next(self.detailMissing) and "Some item data is still loading." or "")
            return
        end
        local ok,status=true,1
        if pending.phase=="compose" then ok,status=pcall(SaureksClosetPreviewStatus,pending.token) end
        if not ok or status<0 then
            self.detailPending=nil;self.detailPreviewNote:SetText("Outfit preview unavailable. Close and reopen this outfit.");return
        end
        if status~=1 then return end
        local target=pending.target
        if pending.token then target.weaponToken=pending.token end
        local dressed=pcall(self.DressOutfitPreview,self,target,pending.look)
        if not dressed then self.detailPending=nil;self.detailPreviewNote:SetText("Some outfit items could not be previewed.");return end
        pending.phase="reveal";pending.dressedAt=GetTime()
        return
    end
    if self.detailMissing and next(self.detailMissing) then
        -- Retry only missing items. Undressing the completed model on each cache
        -- response makes the entire outfit visibly snap through intermediate sets.
        local retry=GetTime()>=self.detailRetryAt and self.detailItemRetries<3
        if retry then self.detailItemRetries=self.detailItemRetries+1;self.detailRetryAt=GetTime()+2 end
        for id,_ in pairs(self.detailMissing) do
            if GetItemInfo(id) or retry then
                pcall(self.outfitModel.TryOn,self.outfitModel,tostring(id))
                if GetItemInfo(id) then self.detailMissing[id]=nil else self:RequestItem(id) end
            end
        end
        self.detailPreviewNote:SetText(next(self.detailMissing) and "Some item data is unavailable." or "")
    end
end
