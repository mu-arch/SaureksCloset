-- Temporary read-only evidence for independent weapon attachments.
-- Dormant unless /closet weaponscan is explicitly run. No UI/model updates.
local V=VanityStudio
local function numbers(values)
    local strings={}
    for i=1,table.getn(values) do strings[i]=tostring(values[i]) end
    return table.concat(strings,",")
end
function V:WeaponrySnapshot()
    local summary={SaureksClosetWeaponryProbe(-1)}
    if summary[1]~=1 then return "unavailable:"..numbers(summary) end
    local lines={"world="..numbers(summary)}
    for slot=16,18 do
        local link=GetInventoryItemLink("player",slot)
        local _,_,id=string.find(link or "","item:(%d+)")
        local item=id and self.index[tonumber(id)]
        table.insert(lines,"equipped="..slot..","..(id or "0")..","..(item and item[3] or -1)..","..(item and item[5] or -1)..","..(item and item[6] or -1))
    end
    for ordinal=0,63 do
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
        "Saurek's Closet weapon attachment capture; schema 1",
        "addon="..tostring(GetAddOnMetadata("SaureksCloset","Version")),
        "world: status,model,loaded,raw-D40,display,native,virtual-displays[3],virtual-info[6]",
        "equipped: inventory-slot,item-ID,inventory-type,class,subclass",
        "child: ordinal,status,model,parent,attachment-ID,bone-index,loaded,reference-count,next-model"
    }}
    self:Message("Capturing for 30 seconds. Draw and sheathe melee weapons, then your ranged weapon. Run /closet weaponscan again to finish early.")
    self:UpdateWeaponryCapture()
    return true
end
function V:UpdateWeaponryCapture()
    local capture=self.weaponryCapture
    if not capture then return end
    local now=GetTime()
    if now-capture.started>=30 then self:FinishWeaponryCapture();return end
    if now<capture.nextSample then return end
    capture.nextSample=now+.1
    local ok,snapshot=pcall(self.WeaponrySnapshot,self)
    if not ok then self:FinishWeaponryCapture("read failed");return end
    if snapshot==capture.last then return end
    local entry=string.format("\nTIME %.2f\n",now-capture.started)..snapshot
    if capture.bytes+string.len(entry)>131072 then self:FinishWeaponryCapture("size limit");return end
    capture.bytes=capture.bytes+string.len(entry);capture.last=snapshot
    table.insert(capture.lines,entry)
end
