-- Debug fit data is separate from saved looks and actual character equipment.
local V=VanityStudio
V.bagTunerFields={
    {key="left",label="Left / right",step=.005,min=-1,max=1,decimals=4,help="Positive moves toward the character's left. Negative moves right."},
    {key="inset",label="Depth / inset",step=.005,min=-1,max=1,decimals=4,help="Positive pulls the bag closer to the back; negative moves it outward. Added to the fitted back surface."},
    {key="up",label="Up / down",step=.005,min=-1,max=1,decimals=4,help="Positive raises the bag; negative lowers it. Position uses fixed model units, so zoom does not change the fit."},
    {key="pitch",label="Inward tilt",step=1,min=-180,max=180,decimals=1,help="Degrees around the lateral axis. Positive pulls the bottom inward toward the back."},
    {key="roll",label="Side tilt",step=1,min=-180,max=180,decimals=1,help="Degrees of side tilt. Positive moves the bottom toward the character's right."},
    {key="yaw",label="Twist",step=1,min=-180,max=180,decimals=1,help="Degrees around the bag's upright axis. Turns the left and right edges inward or outward."},
    {key="scale",label="Size (%)",step=1,min=25,max=200,decimals=1,help="Percentage of the original bag size. The current built-in size is 85%. Resizing keeps the attachment point fixed."},
}
local function keyFor(bag,race,sex) return bag..":"..race..":"..sex end
local function validIdentity(bag,race,sex)
    return bag==1 and type(race)=="number" and race>=1 and race<=8 and race==math.floor(race) and (sex==0 or sex==1)
end
local function validValues(values)
    if type(values)~="table" then return false end
    for _,field in ipairs(V.bagTunerFields) do
        local n=values[field.key]
        if type(n)~="number" or not (n>=field.min and n<=field.max) then return false end
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
    local store=self:BagTunerStore();local saved=store and store.fits[keyFor(bag,race,sex)]
    if type(saved)=="table" and saved.schema==1 and saved.bag==bag and saved.race==race and saved.sex==sex and validValues(saved.values) then return saved end
end
function V:BagTunerDefaults(bag,race,sex)
    if not self:BagTuningAvailable() or not validIdentity(bag,race,sex) then return nil end
    self.bagTunerDefaults=self.bagTunerDefaults or {}
    local key=keyFor(bag,race,sex)
    if self.bagTunerDefaults[key] then return self:Copy(self.bagTunerDefaults[key]) end
    local ok,status,left,inset,up,pitch,roll,yaw,scale=pcall(SaureksClosetGetBagFitDefaults,bag,race,sex)
    local values={left=left,inset=inset,up=up,pitch=pitch,roll=roll,yaw=yaw,scale=scale}
    if not ok or status~=1 or not validValues(values) then return nil end
    self.bagTunerDefaults[key]=self:Copy(values);return values
end
function V:BagTunerIdentity()
    local c=VanityStudioCharacter or {}
    local body=c.enabled and c.body or nil
    if not body then body=self:NativeBody() end
    if not body or not validIdentity(1,body.race,body.sex) then return nil end
    return 1,body.race,body.sex
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
    for race=1,8 do for sex=0,1 do
        local key=keyFor(1,race,sex);local saved=self:BagTunerSaved(1,race,sex)
        local values=enabled and (self.bagTunerDrafts[key] or (saved and saved.values)) or nil
        local motion=not (self.bagTunerPaused and self.bagTunerTargetKey==key)
        local signature="off"
        if values then
            signature=motion and "on" or "paused"
            for _,field in ipairs(self.bagTunerFields) do signature=signature..":"..string.format("%.6f",values[field.key]) end
        end
        if self.bagTunerSynced[key]~=signature then
            local ok,status
            if values then
                ok,status=pcall(SaureksClosetSetBagFit,1,race,sex,1,values.left,values.inset,values.up,values.pitch,values.roll,values.yaw,values.scale,motion and 1 or 0)
            else ok,status=pcall(SaureksClosetSetBagFit,1,race,sex,0) end
            if ok and status==1 then self.bagTunerSynced[key]=signature
            else failure="The bag renderer is not ready. Your values are kept; retrying." end
        end
    end end
    self.bagTunerError=failure;return not failure
end
function V:GetBagTunerState()
    local result={available=false,title="Bag Tuner",values={},saved=false,dirty=false,enabled=false,paused=self.bagTunerPaused and true or false}
    if not self:BagTuningAvailable() then result.status="Update SaureksCloset.dll and fully restart WoW to use the tuner.";return result end
    local store=self:BagTunerStore()
    if not store then result.status="Saved tuner data uses an unsupported format; it has been preserved.";return result end
    local bag,race,sex=self:BagTunerIdentity()
    if not bag then result.status="Waiting for your character model.";return result end
    local defaults=self:BagTunerDefaults(bag,race,sex)
    if not defaults then result.status="The built-in bag fit is not ready yet.";return result end
    local key=keyFor(bag,race,sex);local saved=self:BagTunerSaved(bag,race,sex)
    self.bagTunerDrafts=self.bagTunerDrafts or {}
    if not self.bagTunerDrafts[key] then self.bagTunerDrafts[key]=self:Copy(saved and saved.values or defaults) end
    self.bagTunerTargetKey=key;self:SyncBagTuning()
    result.available=true;result.bag=bag;result.race=race;result.sex=sex;result.key=key
    result.title="Runecloth Bag - "..VanityStudioRaces[race][1]..(sex==0 and " Male" or " Female")
    result.values=self:Copy(self.bagTunerDrafts[key]);result.saved=saved~=nil
    result.dirty=not sameValues(result.values,saved and saved.values or defaults);result.enabled=store.enabled~=false
    result.status=result.dirty and "Unsaved changes - this session only." or (saved and "Saved fit loaded." or "Built-in fit. Adjust, then Save Fit.")
    if not result.enabled then result.status="Live tuning is off. The built-in fits are in use."
    elseif not VanityStudioCharacter.enabled then result.status="Turn on the wardrobe Toggle to see the bag."
    elseif not VanityStudioCharacter.weapons or VanityStudioCharacter.weapons.backBag~=1 then result.status="Select Runecloth Bag in the Top Left slot to see changes."
    elseif self.bagTunerError then result.status=self.bagTunerError end
    return result
end
function V:SetBagTunerValue(key,value)
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local definition
    for _,field in ipairs(self.bagTunerFields) do if key==field.key then definition=field end end
    if not definition or type(value)~="number" or not (value>=definition.min and value<=definition.max) then
        return false,definition and ("Enter a number from "..definition.min.." to "..definition.max..".") or "Unknown fit control."
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
    local saved={schema=1,bag=state.bag,race=state.race,sex=state.sex,values=self:Copy(state.values),renderer=rendererVersion(),savedAt=timestamp()}
    self:BagTunerStore().fits[state.key]=saved
    self:Message("Bag fit saved for "..VanityStudioRaces[state.race][1]..(state.sex==0 and " Male" or " Female")..". Log out or /reload to write SavedVariables; Export makes a report now.")
    return true
end
function V:LoadBagTunerFit()
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local saved=self:BagTunerSaved(state.bag,state.race,state.sex)
    if not saved then return false,"No saved fit for this bag, race, and gender yet." end
    self.bagTunerDrafts[state.key]=self:Copy(saved.values);self:SyncBagTuning();return true
end
function V:ResetBagTunerField(key)
    local known=false
    for _,field in ipairs(self.bagTunerFields) do if key==field.key then known=true;break end end
    if not known then return false,"Unknown fit control." end
    local state=self:GetBagTunerState();if not state.available then return false,state.status end
    local defaults=self:BagTunerDefaults(state.bag,state.race,state.sex)
    if not defaults then return false,"The built-in bag fit is not ready yet." end
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
    return '{"bag": 1, "model": "DarkSchoolbag.m2", "slot": "back_top_left", "race": '..record.race..', "sex": '..record.sex..', "renderer": '..version..', "savedAt": '..quote(record.savedAt)..', "values": {'..table.concat(fields,", ")..'}}'
end
function V:ExportBagTunerFits()
    local state=self:GetBagTunerState();if not state.available then self:Message(state.status);return nil end
    local records={}
    for race=1,8 do for sex=0,1 do
        local saved=self:BagTunerSaved(1,race,sex);if saved then table.insert(records,"    "..fitJSON(saved)) end
    end end
    local current={race=state.race,sex=state.sex,values=state.values,renderer=rendererVersion(),savedAt="unsaved draft"}
    local text='{\n  "schema": 1,\n  "type": "SaureksClosetBagFits",\n  "addon": '..quote(self.VERSION)..',\n  "units": {"position": "character model units", "rotation": "degrees", "scale": "percent of 0.45 base scale"},\n  "axes": {"left": "positive character left", "inset": "positive toward back, added to built-in contact depth", "up": "positive upward"},\n  "currentDraft": '..fitJSON(current)..',\n  "savedFits": [\n'..table.concat(records,",\n")..'\n  ]\n}\n'
    local store=self:BagTunerStore();store.lastExport=text
    local written=false
    if type(WriteFile)=="function" then
        local ok,value=pcall(WriteFile,"SaureksCloset-bag-fits.json","w",text);written=ok and value~=false
    end
    self:Message(written and "Export saved: VanillaHelpersData/SaureksCloset-bag-fits.json in the game folder."
        or "Copy the export text. It is also stored in VanityStudioDB.bagTuning.lastExport for the next logout or /reload.")
    return text
end
