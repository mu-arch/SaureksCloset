-- Client-side character renderer. No display-ID morph or player appearance field writes.
local V=VanityStudio
V.bodyKeys={"skin","face","hairStyle","hairColor","facial"}
V.bodyLabels={skin="Skin color",face="Face",hairStyle="Hair style",hairColor="Hair color",facial="Facial features"}
function V:BodyAvailable()
    if type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and (version==30001 or version==30002 or version==30003 or version==30004 or version==30005 or version==30006 or version==30400) and type(SaureksClosetSetAppearance)=="function" and type(SaureksClosetClearAppearance)=="function" and type(SaureksClosetRealBody)=="function"
end
function V:BodyValues(body,key)
    local d=VanityStudioBodyOptions[body.race] and VanityStudioBodyOptions[body.race][body.sex]
    if not d then return {0} end
    if key=="face" then return d.faces[body.skin] or {0}
    elseif key=="hairColor" then return d.hairColors[body.hairStyle] or {0}
    else return d[key] or {0} end
end
function V:NormalizeBody(body)
    local b=self:Copy(body or {})
    if not VanityStudioBodyOptions[b.race] then b.race=1 end
    if b.sex~=0 and b.sex~=1 then b.sex=0 end
    for _,key in ipairs(self.bodyKeys) do
        local choices=self:BodyValues(b,key);local valid=false
        for _,value in ipairs(choices) do if b[key]==value then valid=true end end
        if not valid then b[key]=choices[1] end
    end
    return b
end
function V:NativeBody()
    if not self:BodyAvailable() then return nil end
    local ok,race,sex,skin,face,hair,color,facial=pcall(SaureksClosetRealBody)
    if not ok or race==-1 then self.bodyError="Character is not ready yet.";return nil end
    return self:NormalizeBody({race=race,sex=sex,skin=skin,face=face,hairStyle=hair,hairColor=color,facial=facial})
end
function V:BodyDraft()
    return self.editingBody or VanityStudioCharacter.body or self:NativeBody() or self:NormalizeBody({})
end
function V:EditBody(key,value)
    if not self:BodyAvailable() then self:Message("Restart through VanillaFixes.exe to load SaureksCloset.dll for body customization.");return false end
    local b=self:Copy(self:BodyDraft())
    b[key]=value
    self.editingBody=self:NormalizeBody(b)
    self:Refresh();return true
end
function V:ApplyBody()
    if not self.editingBody then return end
    local c=VanityStudioCharacter;local previous=c.body
    c.body=self:Copy(self.editingBody)
    if self:SyncBody()==false then c.body=previous;self:Message(self.bodyError);self:Refresh();return false end
    self.editingBody=nil;self:TrackUnsaved()
    self:InvalidatePreviewModel(.25,true)
    self:Refresh();return true
end
function V:CycleBody(key,direction)
    local b=self:BodyDraft();local choices
    if key=="race" then choices={1,2,3,4,5,6,7,8}
    elseif key=="sex" then choices={0,1}
    else choices=self:BodyValues(b,key) end
    local index=1
    for i,value in ipairs(choices) do if b[key]==value then index=i end end
    index=index+direction
    if index>table.getn(choices) then index=1 elseif index<1 then index=table.getn(choices) end
    return self:SelectBodyValue(key,choices[index])
end
function V:SelectBodyValue(key,value)
    if not self:EditBody(key,value) then return false end
    local applied=self:ApplyBody()
    if not applied then
        -- Reject the draft as well so the row returns to the last applied appearance.
        self.editingBody=nil;self:Refresh()
    end
    return applied
end
function V:ClearBody()
    self.editingBody=nil;VanityStudioCharacter.body=nil;self:TrackUnsaved()
    self:InvalidatePreviewModel(.25,true)
    self:Sync();self:Refresh()
end
local bodyErrors={[-1]="Character is not ready yet.",[-2]="That appearance is unavailable.",[-3]="Restore your real body in other morph addons first.",[-4]="A model change is already in progress."}
function V:SyncBody()
    local c=VanityStudioCharacter;local b=c.enabled and c.body
    if not self:BodyAvailable() then
        self.bodyError=b and "Restart through VanillaFixes.exe to load the new race renderer." or nil
        if b then return false end
        return
    end
    local ok,status
    if b then
        ok,status=pcall(SaureksClosetSetAppearance,b.race,b.sex,b.skin,b.face,b.hairStyle,b.hairColor,b.facial)
    else ok,status=pcall(SaureksClosetClearAppearance) end
    if not ok or status~=1 then self.bodyError=bodyErrors[status] or tostring(status);return false end
    self.bodyError=nil;c.bodyManaged=b and true or nil
    return true
end
function V:Diagnose()
    local lines={"Saurek's Closet "..self.VERSION.." diagnostics", "Renderer available: "..tostring(self:BodyAvailable())}
    local version,build=GetBuildInfo()
    table.insert(lines,"Client: "..tostring(version).." build "..tostring(build))
    table.insert(lines,"Armor helper: "..tostring(self:Available()))
    table.insert(lines,"Old body helper loaded: "..tostring(type(SaureksClosetSetBody)=="function"))
    table.insert(lines,"World appearance enabled: "..tostring(VanityStudioCharacter.enabled))
    table.insert(lines,"Saved body present: "..tostring(VanityStudioCharacter.body~=nil))
    table.insert(lines,"Body error: "..tostring(self.bodyError))
    table.insert(lines,"Requested body: "..(VanityStudioCharacter.body and (VanityStudioCharacter.body.race..","..VanityStudioCharacter.body.sex..","..VanityStudioCharacter.body.skin..","..VanityStudioCharacter.body.face..","..VanityStudioCharacter.body.hairStyle..","..VanityStudioCharacter.body.hairColor..","..VanityStudioCharacter.body.facial) or "real"))
    table.insert(lines,"Preview error: "..tostring(self.previewError))
    local rendererOK,renderer=pcall(function() return SaureksClosetRendererVersion() end)
    table.insert(lines,"Loaded renderer version: "..tostring(rendererOK and renderer or "unavailable"))
    if type(UnitDisplayInfo)=="function" then
        local ok,display,native,mount=pcall(UnitDisplayInfo,"player")
        if ok then table.insert(lines,"Displays: "..tostring(display)..","..tostring(native)..","..tostring(mount)) end
    end
    if type(SaureksClosetInspect)=="function" then
        local result={pcall(SaureksClosetInspect)}
        for i=1,table.getn(result) do result[i]=tostring(result[i]) end
        table.insert(lines,"Renderer inspection: "..table.concat(result,","))
    end
    for _,slot in ipairs(self.slotOrder) do
        table.insert(lines,"Slot "..slot..": "..tostring(VanityStudioCharacter.selected[slot]).."; error: "..tostring(self.errors[slot]))
    end
    local report=table.concat(lines,"\n").."\n"
    VanityStudioDB.diagnostics=report
    if type(WriteFile)=="function" then
        local ok=pcall(WriteFile,"SaureksCloset-diagnostics.txt","w",report)
        if ok then self:Message("Saved VanillaHelpersData/SaureksCloset-diagnostics.txt in the game folder.");return end
    end
    self:Message("Diagnostic report saved in VanityStudioDB at logout.")
end
BINDING_NAME_VANITYSTUDIO_PREVIOUS="Previous saved look"
BINDING_NAME_VANITYSTUDIO_NEXT="Next saved look"
