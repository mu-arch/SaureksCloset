-- Temporary read-only evidence for independent weapon attachments.
-- Dormant unless /closet weaponscan is explicitly run. No UI/model updates.
local V=VanityStudio
local function numbers(values)
    local strings={}
    for i=1,table.getn(values) do strings[i]=tostring(values[i]) end
    return table.concat(strings,",")
end
local function inspection(fn,arg)
    if type(fn)~="function" then return "unavailable" end
    return numbers({pcall(fn,arg)})
end
local function slots(values,first,last)
    local result={}
    for slot=first,last do table.insert(result,tostring((values or {})[slot] or 0)) end
    return table.concat(result,",")
end
local function body(value)
    if not value then return "real" end
    local values={}
    for _,key in ipairs({"race","sex","skin","face","hairStyle","hairColor","facial"}) do
        table.insert(values,tostring(value[key]))
    end
    return table.concat(values,",")
end
local function field(value)
    local text=string.gsub(string.gsub(string.gsub(tostring(value),"\n","\\n"),"\r","\\r"),",","\\,")
    return text
end
local function entry(lines,label,...)
    local values={}
    for i=1,arg.n do table.insert(values,field(arg[i])) end
    table.insert(lines,label.."="..table.concat(values,","))
end
-- Read query results only: never call body/UI refreshes, TryOn, item requests,
-- or renderer setters while gathering evidence.
local function query(lines,label,fn,self,value,other)
    if type(fn)~="function" then entry(lines,label,"unavailable");return {} end
    local ok,result=pcall(fn,self,value,other)
    if not ok or type(result)~="table" then entry(lines,label,"read-failed",result);return {} end
    return result
end
local function itemSet(lines,label,values)
    local ids={}
    for id,_ in pairs(values or {}) do if type(id)=="number" then table.insert(ids,id) end end
    table.sort(ids)
    table.insert(lines,label.."="..numbers(ids))
end
function V:WeaponrySnapshot()
    local summary={SaureksClosetWeaponryProbe(-1)}
    local lines={"world="..numbers(summary)}
    local c=VanityStudioCharacter or {}
    table.insert(lines,"enabled="..tostring(c.enabled))
    table.insert(lines,"saved-weapons="..slots(c.weapons,101,107))
    table.insert(lines,"legacy-weapons="..slots(c.selected,16,18))
    table.insert(lines,"draft="..tostring(self.draft and self.draft.slot)..","..tostring(self.draft and self.draft.id))
    table.insert(lines,"body="..body(c.body))
    table.insert(lines,"body-editing="..(self.editingBody and body(self.editingBody) or "none"))
    table.insert(lines,"body-controls="..(self.bodyControlValues and body(self.bodyControlValues) or "unavailable"))
    table.insert(lines,"body-true-cache="..(self.trueBody and body(self.trueBody) or "unavailable"))
    entry(lines,"true-model",not self.editingBody and not c.body,self.trueBodyPending)
    table.insert(lines,"native-body="..inspection(SaureksClosetRealBody))
    table.insert(lines,"unit-displays="..inspection(UnitDisplayInfo,"player"))
    table.insert(lines,"show-helm="..inspection(ShowingHelm))
    table.insert(lines,"show-cloak="..inspection(ShowingCloak))
    table.insert(lines,"renderer="..inspection(SaureksClosetInspect))
    table.insert(lines,"errors="..tostring(self.weaponError)..","..tostring(self.bodyError)..","..tostring(self.previewError))
    table.insert(lines,"preview-state="..tostring(self.tab)..","..tostring(self.previewBaseDirty)..","..tostring(self.previewDressAt~=nil)..","..tostring(self.previewReveal~=nil)..","..tostring(self.detailPending and self.detailPending.phase))
    for _,name in ipairs({"model","previewBuffer","outfitModel","outfitBuffer"}) do
        local model=self[name]
        if model then
            table.insert(lines,"preview="..name..","..tostring(model.weaponToken)..","..inspection(model.IsVisible,model)..","..inspection(model.GetAlpha,model))
            if model.weaponToken then
                table.insert(lines,"preview-body="..name..","..inspection(SaureksClosetInspectPreview,model.weaponToken))
            end
        end
    end
    -- Include ammo and bags: an equipped quiver can affect attachments even
    -- though bag appearance overrides are not implemented by the wardrobe.
    local equipped={}
    for slot=0,23 do
        local link=GetInventoryItemLink("player",slot)
        local _,_,id=string.find(link or "","item:(%d+)")
        local item=id and self.index[tonumber(id)]
        equipped[slot]=tonumber(id) or 0
        table.insert(lines,"equipped="..slot..","..(id or "0")..","..(item and item[3] or -1)..","..(item and item[5] or -1)..","..(item and item[6] or -1))
        local asset=id and SaureksClosetWeaponAssets and SaureksClosetWeaponAssets[tonumber(id)]
        if asset then table.insert(lines,"equipped-asset="..slot..","..numbers(asset)) end
    end
    local previewItems=query(lines,"preview-items-query",self.PreviewItems,self)
    local previewWeapons=query(lines,"preview-weapons-query",self.PreviewWeapons,self)
    local previewRoutes=query(lines,"preview-routes-query",self.PreviewWeaponRoutes,self,previewWeapons)
    local worldWeapons=c.enabled and c.weapons or {}
    if c.enabled and type(self.EffectiveWeapons)=="function" then
        worldWeapons=query(lines,"world-weapons-query",self.EffectiveWeapons,self,c.weapons,c.selected)
    end
    local look=self.detailPending and self.detailPending.look or self.detailPreviewLook
    local outfitItems=look and query(lines,"outfit-items-query",self.OutfitPreviewItems,self,look) or {}
    local outfitWeapons=look and look.weapons or {}
    if look and type(self.EffectiveWeapons)=="function" then
        outfitWeapons=query(lines,"outfit-weapons-query",self.EffectiveWeapons,self,outfitWeapons,look.slots)
    end
    local outfitRoutes=look and query(lines,"outfit-routes-query",self.PreviewWeaponRoutes,self,outfitWeapons,look.slots) or {}
    table.insert(lines,"outfit-body="..(look and body(look.body) or "no-preview"))
    local itemIDs={}
    for _,slot in ipairs(self.slotOrder or {1,3,15,4,5,19,9,10,6,7,8,16,17,18}) do
        local selected=(c.selected or {})[slot]
        local requested=c.enabled and selected
        if requested==nil or requested==false then requested=equipped[slot] end
        local pending=(self.pending or {})[slot]
        entry(lines,"morph-slot",slot,(self.slotNames or {})[slot],equipped[slot],selected,requested,
            (self.applied or {})[slot],(c.managed or {})[slot],pending and pending.id,pending and pending.attempts,
            (self.errors or {})[slot],previewItems[slot],previewRoutes[slot],outfitItems[slot],outfitRoutes[slot])
        for _,id in pairs({selected=selected,applied=(self.applied or {})[slot],preview=previewItems[slot],outfit=outfitItems[slot]}) do
            if type(id)=="number" and id>0 then itemIDs[id]=true end
        end
    end
    for slot=101,107 do
        entry(lines,"weapon-slot",slot,(self.slotNames or {})[slot],(c.weapons or {})[slot],
            (worldWeapons or {})[slot],previewWeapons[slot],outfitWeapons[slot])
        for _,id in pairs({saved=(c.weapons or {})[slot],preview=previewWeapons[slot],outfit=outfitWeapons[slot]}) do
            if type(id)=="number" and id>0 then itemIDs[id]=true end
        end
    end
    local sortedIDs={}
    for id,_ in pairs(itemIDs) do table.insert(sortedIDs,id) end
    table.sort(sortedIDs)
    for _,id in ipairs(sortedIDs) do
        local item=self.index[id] or {}
        local asset=SaureksClosetWeaponAssets and SaureksClosetWeaponAssets[id] or {}
        entry(lines,"selected-item",id,item[2],item[3],item[5],item[6],asset[1],asset[2])
    end
    itemSet(lines,"wardrobe-missing-items",self.previewWaiting)
    itemSet(lines,"outfit-missing-items",self.detailMissing)
    for ordinal=0,63 do
        if summary[1]~=1 then break end
        local values={SaureksClosetWeaponryProbe(ordinal)}
        if values[1]==0 then break end
        table.insert(lines,"child="..ordinal..","..numbers(values))
        if values[1]~=1 then break end
        if ordinal==63 then table.insert(lines,"child-list-limit=64") end
    end
    return table.concat(lines,"\n")
end
function V:FinishWeaponryCapture(reason)
    local capture=self.weaponryCapture
    if not capture then return end
    self.weaponryCapture=nil
    table.insert(capture.lines,"END "..(reason or "complete"))
    local report=table.concat(capture.lines,"\n").."\n"
    VanityStudioDB.weaponryDiagnostics=report
    local written=false
    if type(WriteFile)=="function" then
        local ok,value=pcall(WriteFile,"SaureksCloset-weaponry.txt","w",report)
        written=ok and value~=false
    end
    self:Message(written and "Weapon capture saved: VanillaHelpersData/SaureksCloset-weaponry.txt in the game folder."
        or "Weapon capture saved in VanityStudioDB.weaponryDiagnostics; log out to write SavedVariables.")
end
function V:StartWeaponryCapture()
    if self.weaponryCapture then self:FinishWeaponryCapture("stopped");return end
    if type(SaureksClosetWeaponryProbe)~="function" then
        self:Message("Copy the updated SaureksCloset.dll and fully restart WoW before running the weapon capture.")
        return false
    end
    self.weaponryCapture={started=GetTime(),nextSample=0,bytes=0,lines={
        "Saurek's Closet weapon and morph capture; schema 3 (read-only)",
        "addon="..tostring(GetAddOnMetadata("SaureksCloset","Version")),
        "addon-code-version="..tostring(self.VERSION),
        "renderer-version="..inspection(SaureksClosetRendererVersion),
        "client="..inspection(GetBuildInfo),
        "world: status,model,loaded,raw-D40,display,native,virtual-displays[3],virtual-info[6]",
        "saved-weapons: left-waist,right-waist,back-left,back-right,shield,ranged-back,quiver (item IDs; 0=none)",
        "legacy-weapons: main-hand,off-hand,ranged (item IDs)",
        "draft: position,item-ID; nil item means clear selection",
        "body: race,sex,skin,face,hair-style,hair-color,facial; real means no saved override",
        "body-editing/body-controls/body-true-cache/outfit-body: same body fields; values are zero-based client values",
        "true-model: no-saved-or-editing-body,true-body-pending",
        "unit-displays: call-ok,display,native,mount; show-helm/show-cloak: call-ok,visible",
        "native-body/renderer/preview-body: first value is protected-call success",
        "renderer: schema,enabled,applies,revision,composed,reloads,detail-updates,display,native,scale,body[7],ratio",
        "preview-state: tab,base-dirty,dress-pending,reveal-pending,outfit-phase",
        "preview: name,token,visible-call-ok,visible,alpha-call-ok,alpha",
        "preview-body: name,call-ok,status,copied-appearance,race,sex,skin,face,hair-style,hair-color,facial,dirty-textures",
        "Preview attachment lists are not exposed by this DLL; child rows describe the world model only.",
        "equipped: inventory-slot,item-ID,inventory-type,class,subclass",
        "Inventory slots: 0 ammo; 1 head; 2 neck; 3 shoulders; 4 shirt; 5 chest; 6 waist; 7 legs; 8 feet; 9 wrists; 10 hands; 11-12 fingers; 13-14 trinkets; 15 back; 16 main hand; 17 off hand; 18 ranged; 19 tabard; 20-23 bags",
        "morph-slot: slot,name,equipped,saved-override,world-requested-item,last-applied-override,managed,pending-item,pending-attempts,error,wardrobe-requested-item,wardrobe-weapon-route,outfit-requested-item,outfit-weapon-route",
        "weapon-slot: position,name,saved-item,world-requested-item,wardrobe-requested-item,outfit-requested-item",
        "Weapon positions: 101 left waist; 102 right waist; 103 back left; 104 back right; 105 shield; 106 ranged back; 107 quiver",
        "nil override means use real equipment; 0 armor override means hidden; nil weapon position means no forced item. Requested/last-applied values are addon state, not confirmation of rendered geometry. Weapon routes supersede inventory preview items.",
        "selected-item: item-ID,name,inventory-type,class,subclass,weapon-kind,weapon-subclass; missing metadata is nil",
        "equipped-asset: inventory-slot,weapon-kind,weapon-subclass",
        "child: ordinal,status,model,parent,attachment-ID,attachment-index,loaded,reference-count,next-model"
    }}
    self:Message("Capturing all weapons, equipment and body morph settings for 90 seconds. Change the affected slots and compare world/preview results. Run /closet weaponscan again to save early.")
    self:UpdateWeaponryCapture()
    return true
end
function V:UpdateWeaponryCapture()
    local capture=self.weaponryCapture
    if not capture then return end
    local now=GetTime()
    if now-capture.started>=90 then self:FinishWeaponryCapture();return end
    if now<capture.nextSample then return end
    capture.nextSample=now+.1
    local ok,snapshot=pcall(self.WeaponrySnapshot,self)
    if not ok then self:FinishWeaponryCapture("read failed");return end
    if snapshot==capture.last then return end
    local entry=string.format("\nTIME %.2f\n",now-capture.started)..snapshot
    if capture.bytes+string.len(entry)>524288 then self:FinishWeaponryCapture("size limit");return end
    capture.bytes=capture.bytes+string.len(entry);capture.last=snapshot
    table.insert(capture.lines,entry)
end
