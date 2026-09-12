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
    self.outfitModel:SetAlpha(0);self.outfitBuffer:SetAlpha(0)
    self.detailMissing=nil
    self.detailPending={look=self:Copy(look),phase="copy",deadline=GetTime()+8}
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
    local routes=self:PreviewWeaponRoutes(look.weapons)
    for slot,id in pairs(self:OutfitPreviewItems(look)) do
        if id>0 and not routes[slot] then
            target:TryOn(tostring(id))
            if not GetItemInfo(id) then self.detailMissing[id]=true;self:RequestItem(id) end
        end
    end
    self:DressWeaponPlacements(target,look.weapons)
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
        local ok,status=pcall(SaureksClosetPreviewStatus,pending.token)
        if not ok or status<0 then
            self.detailPending=nil;self.detailPreviewNote:SetText("Outfit preview unavailable. Close and reopen this outfit.");return
        end
        if status~=1 then return end
        local target=self.outfitBuffer;target.weaponToken=pending.token
        local dressed=pcall(self.DressOutfitPreview,self,target,pending.look)
        self.detailPending=nil
        if not dressed then self.detailPreviewNote:SetText("Some outfit items could not be previewed.");return end
        local previous=self.outfitModel;target.rotation=previous.rotation or .61
        target:SetAlpha(1);previous:SetAlpha(0);self.outfitModel=target;self.outfitBuffer=previous
        self.detailPreviewLook=pending.look;self.detailItemRetries=0;self.detailRetryAt=GetTime()+2
        self.detailPreviewNote:SetText(next(self.detailMissing) and "Some item data is still loading." or "")
        return
    end
    if self.detailMissing and next(self.detailMissing) and self.detailItemRetries<3 and GetTime()>=self.detailRetryAt then
        self.detailItemRetries=self.detailItemRetries+1;self.detailRetryAt=GetTime()+2
        local ok=pcall(self.DressOutfitPreview,self,self.outfitModel,self.detailPreviewLook)
        self.detailPreviewNote:SetText(ok and (next(self.detailMissing) and "Some item data is unavailable." or "") or "Some outfit items could not be previewed.")
    end
end
