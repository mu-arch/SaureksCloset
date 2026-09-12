-- Client-side character renderer. No display-ID morph or player appearance field writes.
local V=VanityStudio
V.bodyKeys={"skin","face","hairStyle","hairColor","facial"}
V.bodyLabels={skin="Skin color",face="Face",hairStyle="Hair style",hairColor="Hair color",facial="Facial features"}
function V:BodyAvailable()
    if type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and (version==30001 or version==30002 or version==30003 or version==30004 or version==30005 or version==30006 or version==30400 or version==30422 or version==30424 or version==30426 or version==30428 or version==30429 or version==30431 or version==30432 or version==30433 or version==30436) and type(SaureksClosetSetAppearance)=="function" and type(SaureksClosetClearAppearance)=="function" and type(SaureksClosetRealBody)=="function"
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
    if not ok or not VanityStudioRaces[race] or (sex~=0 and sex~=1) or
        type(skin)~="number" or type(face)~="number" or type(hair)~="number" or type(color)~="number" or type(facial)~="number" then
        self.bodyError="Character is not ready yet.";return nil
    end
    -- These are the player's actual settings, not a draft to replace with the
    -- catalog's first skin/hair choices. Normalize only deliberate edits.
    return {race=race,sex=sex,skin=skin,face=face,hairStyle=hair,hairColor=color,facial=facial}
end
function V:RefreshTrueBody()
    -- Runtime control values are separate from c.body, which enables an override.
    -- Always replace the entire set so no skin/hair values survive a reset.
    self.trueBody=self:NativeBody()
    self.trueBodyPending=not self.trueBody and true or nil
    if not VanityStudioCharacter.body and not self.editingBody then
        self.bodyControlValues=self.trueBody and self:Copy(self.trueBody) or nil
    end
    return self.trueBody
end
function V:BodyDraft()
    local body=self.editingBody or VanityStudioCharacter.body or self:RefreshTrueBody()
    self.bodyControlValues=body and self:Copy(body) or nil
    return self.bodyControlValues
end
function V:UsingTrueModel()
    return not self.editingBody and not VanityStudioCharacter.body
end
function V:EditBody(key,value)
    if not self:BodyAvailable() then self:Message("Restart through VanillaFixes.exe to load SaureksCloset.dll for body customization.");return false end
    local current=self:BodyDraft()
    if not current then self:Message("Character is not ready yet.");return false end
    local b=self:Copy(current)
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
    if not b then return false end
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
    local current=self:BodyDraft()
    if current and current[key]==value then return true end
    if not self:EditBody(key,value) then return false end
    local applied=self:ApplyBody()
    if not applied then
        -- Reject the draft as well so the row returns to the last applied appearance.
        self.editingBody=nil;self:Refresh()
    end
    return applied
end
function V:ClearBody()
    if self:UsingTrueModel() then self:RefreshTrueBody();self:Refresh();return true end
    local c=VanityStudioCharacter;local previous=c.body;local draft=self.editingBody
    self.editingBody=nil;c.body=nil
    if self:SyncBody()==false then
        c.body=previous;self.editingBody=draft;self:Message(self.bodyError);self:Refresh();return false
    end
    self:RefreshTrueBody()
    self:TrackUnsaved()
    self:InvalidatePreviewModel(.25,true)
    self:Sync();self:Refresh()
    return true
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
    if not b then self:RefreshTrueBody() end
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
    if type(SaureksClosetInspectPreview)=="function" then
        table.insert(lines,"Preview fields: ok,status,copied appearance,race,sex,skin,face,hair style,hair color,facial,dirty textures")
        for _,name in ipairs({"model","previewBuffer","outfitModel","outfitBuffer"}) do
            local model=self[name]
            if model and model.weaponToken then
                local details={pcall(SaureksClosetInspectPreview,model.weaponToken)}
                for i=1,table.getn(details) do details[i]=tostring(details[i]) end
                table.insert(lines,name..": "..table.concat(details,","))
            end
        end
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
