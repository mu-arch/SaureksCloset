-- Weapon fits retain their account profiles; bag-instance fits belong to looks.
local V=VanityStudio
V.bagTunerFields={
    {key="left",label="Lateral shift",step=.005,min=-1,max=1,decimals=4,help="Move across the character's left-right axis. Positive moves left; negative moves right."},
    {key="inset",label="Longitudinal",step=.005,min=-1,max=1,decimals=4,help="Move along the character's front-back axis. Positive moves inward toward the character; negative moves outward, relative to the default placement."},
    {key="up",label="Vertical shift",step=.005,min=-1,max=1,decimals=4,help="Move along the vertical axis. Positive raises the item; negative lowers it. Position uses fixed model units, so zoom does not change the fit."},
    {key="pitch",label="Lateral tilt",step=1,min=-180,max=180,decimals=1,help="Rotate around the character's lateral (left-right) axis. Positive pulls the bottom inward toward the back."},
    {key="roll",label="Longitudinal tilt",step=1,min=-180,max=180,decimals=1,help="Rotate around the character's longitudinal (front-back) axis. Positive moves the bottom toward the character's right."},
    {key="yaw",label="Vertical twist",step=1,min=-180,max=180,decimals=1,help="Rotate around the item's vertical axis, twisting it left or right."},
    {key="scale",label="Size (%)",step=1,min=25,max=200,decimals=1,help="Percentage of the original size. Weapons default to 100%; bags start at 85% on the back or 70% on a hip."},
}
local function keyFor(bag,race,sex) return bag..":"..race..":"..sex end
local function validIdentity(bag,race,sex)
    return (bag==1 or (type(bag)=="number" and ((bag>=101 and bag<=111) or (bag>=201 and bag<=208)) and bag==math.floor(bag))) and type(race)=="number" and race>=1 and race<=8 and race==math.floor(race) and (sex==0 or sex==1)
end
local function instanceFor(target) return type(target)=="number" and target>=201 and target<=208 and V.BagInstance and V:BagInstance(target-200) end
local function draftKey(target,race,sex)
    local bag=instanceFor(target)
    return bag and V:BagDraftKey(bag,race,sex) or keyFor(target,race,sex)
end
local function targets()
    local result=V:HeldWeaponTuningAvailable() and {1,101,102,103,104,105,106,107,108,109,110} or
        V:WeaponTuningAvailable() and {1,101,102,103,104,105,106,107} or {1}
    if V.HatTuningAvailable and V:HatTuningAvailable() then table.insert(result,111) end
    return result
end
function V:HeldWeaponTuningAvailable()
    if not self:WeaponTuningAvailable() then return false end
    local ok,status=pcall(SaureksClosetGetBagFitDefaults,108,1,0)
    return ok and status==1
end
function V:WeaponTuningAvailable()
    if not self:BagTuningAvailable() then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and version>=30608
end
function V:BagTunerFieldBounds(field,target)
    local bag=target==1 or (type(target)=="number" and target>=201 and target<=208)
    return field.key=="up" and bag and -3 or field.min,field.max
end
local function validValues(values,target)
    if type(values)~="table" then return false end
    for _,field in ipairs(V.bagTunerFields) do
        local n=values[field.key]
        local low,high=V:BagTunerFieldBounds(field,target)
        if type(n)~="number" or not (n>=low and n<=high) then return false end
    end
    return true
end
local function sameValues(a,b)
    if not a or not b then return false end
    for _,field in ipairs(V.bagTunerFields) do if math.abs(a[field.key]-b[field.key])>.000001 then return false end end
    return true
end
local function timestamp() return type(date)=="function" and date("%Y-%m-%d %H:%M:%S") or "" end
local function rendererVersion()
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30510 and version<=2147483647 and math.floor(version)==version and version or 30510
end
function V:BagTuningAvailable()
    if type(SaureksClosetSetBagFit)~="function" or type(SaureksClosetGetBagFitDefaults)~="function" or type(SaureksClosetRendererVersion)~="function" then return false end
    local ok,version=pcall(SaureksClosetRendererVersion)
    return ok and type(version)=="number" and version>=30510
end
function V:BagTunerStore()
    VanityStudioDB=VanityStudioDB or {}
    if VanityStudioDB.bagTuning==nil then VanityStudioDB.bagTuning={schema=1,enabled=true,fits={}} end
    local store=VanityStudioDB.bagTuning
    -- Preserve unknown future formats; never overwrite saved fitting work.
    if type(store)~="table" or store.schema~=1 or type(store.fits)~="table" then return nil end
    return store
end
function V:BagTunerSaved(bag,race,sex)
    local instance=instanceFor(bag)
    if instance then
        local values=instance.fits and instance.fits[race..":"..sex]
        if validValues(values,bag) then return {schema=1,bag=bag,model=instance.model,mount=instance.mount,race=race,sex=sex,values=values} end
        return nil
    end
    local store=self:BagTunerStore();local saved=store and store.fits[keyFor(bag,race,sex)]
    if type(saved)=="table" and saved.schema==1 and saved.bag==bag and saved.race==race and saved.sex==sex and validValues(saved.values,bag) then return saved end
end
function V:BagTunerDefaults(bag,race,sex)
    if not self:BagTuningAvailable() or not validIdentity(bag,race,sex) then return nil end
    self.bagTunerDefaults=self.bagTunerDefaults or {}
    local instance=instanceFor(bag)
    local key=keyFor(bag,race,sex)..(instance and ":"..instance.mount or "")
    if self.bagTunerDefaults[key] then return self:Copy(self.bagTunerDefaults[key]) end
    local ok,status,left,inset,up,pitch,roll,yaw,scale=pcall(SaureksClosetGetBagFitDefaults,bag,race,sex,instance and self.bagMounts[instance.mount] or 0)
    local values={left=left,inset=inset,up=up,pitch=pitch,roll=roll,yaw=yaw,scale=scale}
    if not ok or status~=1 or not validValues(values,bag) then return nil end
    self.bagTunerDefaults[key]=self:Copy(values);return values
end
function V:BagTunerIdentity()
    local c=VanityStudioCharacter or {}
    local body=c.enabled and c.body or nil
    if not body then body=self:NativeBody() end
    if self.placementTunerBag and not self:BagInstance(self.placementTunerBag) then return nil end
    local target=self.placementTunerBag and 200+self.placementTunerBag or self.placementTunerSlot or 1
    if not body or not validIdentity(target,body.race,body.sex) then return nil end
    return target,body.race,body.sex
end
function V:InitializeBagTuning()
    self.bagTunerDrafts={};self.bagTunerDefaults={};self.bagTunerSynced={}
    self.bagTunerPaused=false;self.bagTunerTargetKey=nil;self.bagTunerError=nil
    self:BagTunerStore();self:SyncBagTuning()
end
function V:SyncBagTuning()
    if not self:BagTuningAvailable() then return false end
    self.bagTunerDrafts=self.bagTunerDrafts or {};self.bagTunerSynced=self.bagTunerSynced or {}
    local store=self:BagTunerStore();local enabled=store and store.enabled~=false
    local failure
    for _,target in ipairs(targets()) do for race=1,8 do for sex=0,1 do
        local key=keyFor(target,race,sex);local saved=self:BagTunerSaved(target,race,sex)
        local values=enabled and VanityStudioCharacter.enabled and (self.bagTunerDrafts[key] or (saved and saved.values)) or nil
        local motion=not (self.bagTunerPaused and self.bagTunerTargetKey==key)
        local signature="off"
        if values then
            signature=motion and "on" or "paused"
            for _,field in ipairs(self.bagTunerFields) do signature=signature..":"..string.format("%.6f",values[field.key]) end
        end
        if self.bagTunerSynced[key]~=signature then
            local ok,status
            if values then
                ok,status=pcall(SaureksClosetSetBagFit,target,race,sex,1,values.left,values.inset,values.up,values.pitch,values.roll,values.yaw,values.scale,motion and 1 or 0)
            else ok,status=pcall(SaureksClosetSetBagFit,target,race,sex,0) end
            if ok and status==1 then self.bagTunerSynced[key]=signature
            else failure="The placement renderer is not ready. Your values are kept; retrying." end
        end
    end end end
    if self.SyncLiveBagFits and not self:SyncLiveBagFits() then failure="The bag renderer is not ready. Your values are kept; retrying." end
    self.bagTunerError=failure;return not failure
end
function V:GetBagTunerState()
    local result={available=false,title="Placement Tuner",values={},saved=false,dirty=false,enabled=false,paused=self.bagTunerPaused and true or false}
    local emptySlot=self.bagTunerWindow and self.bagTunerWindow.emptySlot
    if emptySlot then
        -- Selecting a model for an empty slot must never initialize or edit
        -- the legacy single-bag fit while there is no instance to tune.
        result.emptySlot=emptySlot;result.key="empty:"..emptySlot;result.title="Choose a bag";result.status=""
        if not self:MultiBagRendererAvailable() then result.status="Fully restart WoW with the updated DLL to add bags." end
        return result
    end
    if not self:BagTuningAvailable() then result.status="Update SaureksCloset.dll and fully restart WoW to use the tuner.";return result end
    if self.placementTunerBag and not self:MultiBagRendererAvailable() then result.status="Fully restart WoW with the updated DLL to tune each bag.";return result end
    if self.placementTunerSlot==111 and not self:HatTuningAvailable() then result.status="Update the DLL and restart WoW to adjust hats.";return result end
    if self.placementTunerSlot and self.placementTunerSlot>=108 and self.placementTunerSlot<=110 and not self:HeldWeaponTuningAvailable() then result.status="Update the DLL and restart WoW to tune stowed weapons.";return result end
    if self.placementTunerSlot and not self:WeaponTuningAvailable() then result.status="Fully restart WoW with the updated DLL to tune weapons and quivers.";return result end
    local store=self:BagTunerStore()
    if not store then result.status="Saved tuner data uses an unsupported format; it has been preserved.";return result end
    local bag,race,sex=self:BagTunerIdentity()
    if not bag then result.status="Waiting for your character model.";return result end
    local defaults=self:BagTunerDefaults(bag,race,sex)
    if not defaults then result.status="The built-in placement fit is not ready yet.";return result end
    local instance=instanceFor(bag)
    local key=draftKey(bag,race,sex);local saved=self:BagTunerSaved(bag,race,sex)
    self.bagTunerDrafts=self.bagTunerDrafts or {}
    if not self.bagTunerDrafts[key] then self.bagTunerDrafts[key]=self:Copy(saved and saved.values or defaults) end
    self.bagTunerTargetKey=key;self:SyncBagTuning()
    result.available=true;result.bag=bag;result.race=race;result.sex=sex;result.key=key
    local name=instance and self.bagCatalogByID[instance.model].name or (bag==111 and "Hat" or bag==1 and "Runecloth Bag" or self.slotNames[bag])
    result.title=name.." - "..VanityStudioRaces[race][1]..(sex==0 and " Male" or " Female")
    result.values=self:Copy(self.bagTunerDrafts[key]);result.saved=saved~=nil
    result.dirty=not sameValues(result.values,saved and saved.values or defaults);result.enabled=store.enabled~=false
    result.status=result.dirty and "Unsaved changes - this session only." or ""
    if not result.enabled then result.status="Live tuning is off. The built-in fits are in use."
    elseif not VanityStudioCharacter.enabled then result.status="Enable the wardrobe to see placement changes."
    elseif bag==1 and (not VanityStudioCharacter.weapons or VanityStudioCharacter.weapons.backBag~=1) then result.status="Select Runecloth Bag to see changes."
    elseif not instance and bag>=101 and bag<=107 and not (VanityStudioCharacter.weapons or {})[bag] then result.status="Select a Custom Item for this slot to tune its stored placement."
    elseif self.bagTunerError then result.status=self.bagTunerError end
    return result
end
function V:SetBagTunerValue(key,value)
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local definition
    for _,field in ipairs(self.bagTunerFields) do if key==field.key then definition=field end end
    if not definition then return false,"Unknown fit control." end
    local low,high=self:BagTunerFieldBounds(definition,state.bag)
    if type(value)~="number" or not (value>=low and value<=high) then
        return false,"Enter a number from "..low.." to "..high.."."
    end
    self.bagTunerDrafts[state.key][key]=value;self:SyncBagTuning();return true
end
function V:SetBagTunerEnabled(enabled)
    local store=self:BagTunerStore();if not store then return false end
    store.enabled=enabled and true or false;self:SyncBagTuning();return true
end
function V:SetBagTunerPaused(paused)
    self.bagTunerPaused=paused and true or false;self:SyncBagTuning();return true
end
function V:SaveBagTunerFit()
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local instance=instanceFor(state.bag)
    if instance then
        instance.fits=instance.fits or {};instance.fits[state.race..":"..state.sex]=self:Copy(state.values)
        self:TrackUnsaved();self:SyncBagTuning();self:Refresh()
        self:Message("Bag fit saved to this look. Save the look in Outfits to keep a named copy.")
        return true
    end
    local saved={schema=1,bag=state.bag,race=state.race,sex=state.sex,values=self:Copy(state.values),renderer=rendererVersion(),savedAt=timestamp()}
    self:BagTunerStore().fits[state.key]=saved
    self:Message("Placement fit saved for "..VanityStudioRaces[state.race][1]..(state.sex==0 and " Male" or " Female")..". Log out or /reload to write SavedVariables; Export makes a report now.")
    return true
end
function V:LoadBagTunerFit()
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local saved=self:BagTunerSaved(state.bag,state.race,state.sex)
    if not saved then return false,"No saved fit for this placement, race, and gender yet." end
    self.bagTunerDrafts[state.key]=self:Copy(saved.values);self:SyncBagTuning();return true
end
function V:ResetBagTunerField(key)
    local known=false
    for _,field in ipairs(self.bagTunerFields) do if key==field.key then known=true;break end end
    if not known then return false,"Unknown fit control." end
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local defaults=self:BagTunerDefaults(state.bag,state.race,state.sex)
    if not defaults then return false,"The built-in placement fit is not ready yet." end
    self.bagTunerDrafts[state.key][key]=defaults[key]
    self:SyncBagTuning();return true
end
function V:ResetBagTunerFit()
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    self.bagTunerDrafts[state.key]=self:BagTunerDefaults(state.bag,state.race,state.sex)
    self:SyncBagTuning();return true
end
local function quote(text)
    text=string.gsub(tostring(text or ""),"\\","\\\\");text=string.gsub(text,'"','\\"')
    text=string.gsub(text,"[%z\1-\31]",function(c) return string.format("\\u%04x",string.byte(c)) end)
    return '"'..text..'"'
end
local function fitJSON(record)
    local fields={}
    for _,field in ipairs(V.bagTunerFields) do table.insert(fields,'"'..field.key..'": '..string.format("%.6f",record.values[field.key])) end
    local version=tonumber(record.renderer)
    if not version or not (version>=30510 and version<=2147483647) or math.floor(version)~=version then version=rendererVersion() end
    local target=record.bag or 1;local instance=instanceFor(target)
    local slot=instance and ("bag_"..instance.id.."_"..instance.mount) or (target==1 and "back_top_left" or target==111 and "hat" or V.slotNames[target])
    return '{"bag": '..target..', "slot": '..quote(slot)..(instance and (', "model": '..instance.model) or '')..', "race": '..record.race..', "sex": '..record.sex..', "renderer": '..version..', "savedAt": '..quote(record.savedAt)..', "values": {'..table.concat(fields,", ")..'}}'
end
function V:ExportBagTunerFits()
    local state=self:GetBagTunerState();if not state.available then self:Message(state.status);return nil end
    local records={}
    local exportTargets=targets()
    if self.GetBags then for _,bag in ipairs(self:GetBags()) do table.insert(exportTargets,200+bag.id) end end
    for _,target in ipairs(exportTargets) do for race=1,8 do for sex=0,1 do
        local saved=self:BagTunerSaved(target,race,sex);if saved then table.insert(records,"    "..fitJSON(saved)) end
    end end end
    local current={bag=state.bag,race=state.race,sex=state.sex,values=state.values,renderer=rendererVersion(),savedAt="unsaved draft"}
    local text='{\n  "schema": 1,\n  "type": "SaureksClosetPlacementFits",\n  "addon": '..quote(self.VERSION)..',\n  "units": {"position": "character model units", "rotation": "degrees", "scale": "percent (bag: 0.45 base scale; weapons: original size)"},\n  "axes": {"left": "positive character left", "inset": "positive toward back, added to built-in contact depth", "up": "positive upward"},\n  "currentDraft": '..fitJSON(current)..',\n  "savedFits": [\n'..table.concat(records,",\n")..'\n  ]\n}\n'
    local store=self:BagTunerStore();store.lastExport=text
    local written=false
    if type(WriteFile)=="function" then
        local ok,value=pcall(WriteFile,"SaureksCloset-bag-fits.json","w",text);written=ok and value~=false
    end
    self:Message(written and "Export saved: VanillaHelpersData/SaureksCloset-bag-fits.json in the game folder."
        or "Copy the export text. It is also stored in VanityStudioDB.bagTuning.lastExport for the next logout or /reload.")
    return text
end
