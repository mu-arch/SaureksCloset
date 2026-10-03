-- Local fitting tools; drafts and exports are handled by BagTuner.lua.
local V=VanityStudio
local function tooltip(widget,text)
    widget:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:ClearLines()
        GameTooltip:AddLine(type(text)=="function" and text() or text,1,1,1,true);GameTooltip:Show()
    end)
    widget:SetScript("OnLeave",function() GameTooltip:Hide() end)
end
local function showMessage(text)
    if not V.bagTunerWindow then return end
    V.bagTunerWindow.message=text
    V.bagTunerWindow.messageTime=4
end
local function fieldHelp(field)
    if V.placementTunerSlot and V.placementTunerSlot>=108 then
        return field.label..": adjust the stowed weapon on your body. Drawing restores its normal position, rotation and size in the hand."
    end
    return field.help
end
local function displayValue(field,value)
    return string.format("%."..(field.decimals or 2).."f",value or 0)
end
local mountNames={back="Back",leftHip="Left hip",rightHip="Right hip"}
local mountOrder={"back","leftHip","rightHip"}
function V:PrepareBagTunerSelection(id)
    local f=self.bagTunerWindow
    if not f or not f:IsShown() or self.placementTunerBag~=id or not self:BagInstance(id) then return false end
    for _,row in ipairs(f.rows) do
        if row.editor.editing then
            local accepted=self:CommitBagTunerEditor(row.editor);row.editor:ClearFocus()
            if not accepted then f.invalidInput=nil;return false end
        end
    end
    -- Focus loss can validate the editor before the selector gets its click.
    -- Leave the bag unchanged on that click when the number was rejected.
    if f.invalidInput then f.invalidInput=nil;return false end
    return self.placementTunerBag==id and self:BagInstance(id)~=nil
end
function V:CloseBagTunerModelPicker()
    local f=self.bagTunerWindow
    if f and f.modelPicker then f.modelPicker:Hide() end
end
function V:OpenBagTunerModelPicker()
    local f=self.bagTunerWindow;local id=self.placementTunerBag
    if not f or not f.modelPicker then return false end
    local emptySlot=f.emptySlot
    if emptySlot then
        if not f:IsShown() or not self:MultiBagRendererAvailable() or self:BagInSlot(emptySlot) then return false end
    elseif not self:PrepareBagTunerSelection(id) then return false end
    local picker=f.modelPicker
    if picker:IsShown() then picker:Hide();return true end
    local state=self:GetBagTunerState();local bag=id and self:BagInstance(id)
    if not emptySlot and (not bag or not state.available) then return false end
    CloseDropDownMenus();GameTooltip:Hide()
    picker.bagID=id;picker.emptySlot=emptySlot;picker.targetKey=state.key
    local current=bag and self:BagModelChoice(bag.model)
    for _,row in ipairs(picker.rows) do
        local selected=row.choice==current
        row.bagID=id;row.emptySlot=emptySlot;row.targetKey=state.key
        local asset=selected and self.bagCatalogByID[bag.model] or self.bagCatalogByID[row.modelID]
        row.icon:SetTexture(asset and asset.icon or row.choice.icon)
        if selected then row.selected:Show();row:SetBackdropBorderColor(1,.82,.25);row.caption:SetTextColor(1,.9,.6)
        else row.selected:Hide();row:SetBackdropBorderColor(.42,.38,.3);row.caption:SetTextColor(1,1,1) end
        f.enableControl(row,true)
    end
    picker:Show();return true
end
function V:CommitBagTunerEditor(editor)
    local f=self.bagTunerWindow
    if not f or f.refreshing or not editor.editing then return true end
    local state=self:GetBagTunerState()
    local value=tonumber(editor:GetText())
    local accepted=true
    editor.editing=nil
    if state.key~=editor.targetKey then
        accepted=false
        showMessage("The placement changed. The unfinished edit was discarded.")
    elseif not value or value~=value or value-value~=0 then
        accepted=false
        showMessage("Enter a number for "..editor.field.label..".")
    else
        local ok,err=self:SetBagTunerValue(editor.field.key,value)
        accepted=ok
        if not ok then showMessage(err or "That value could not be applied.") end
    end
    f.invalidInput=not accepted;f.invalidEditor=not accepted and editor or nil
    self:RefreshBagTunerUI()
    return accepted
end
function V:RefreshBagTunerUI()
    local f=self.bagTunerWindow
    if not f or f.refreshing then return end
    f.refreshing=true
    local state=self:GetBagTunerState()
    if self.activeOutfitLabel then self.activeOutfitLabel:SetText(self:ActiveOutfitText()) end
    if self.RefreshWardrobeSaveButton then self:RefreshWardrobeSaveButton() end
    local available=state.available and true or false
    local targetChanged=f.targetKey~=state.key
    if targetChanged and self.CloseBagPlacementEditor then self:CloseBagPlacementEditor() end
    f.targetKey=state.key
    f.target:SetText(state.title or "Bag fitting")
    local bag=self.placementTunerBag and self:BagInstance(self.placementTunerBag)
    local emptySlot=f.emptySlot
    local emptyAvailable=emptySlot and self:MultiBagRendererAvailable() and not self:BagInSlot(emptySlot)
    if f.modelPicker and f.modelPicker:IsShown() then
        local wrongTarget=f.modelPicker.targetKey~=state.key
        if emptySlot then wrongTarget=wrongTarget or not emptyAvailable or f.modelPicker.emptySlot~=emptySlot
        else wrongTarget=wrongTarget or not available or not bag or f.modelPicker.bagID~=bag.id end
        if wrongTarget then self:CloseBagTunerModelPicker() end
    end
    if bag then
        f.target:Hide();f.bagSelectors:Show()
        local model=self:BagModelChoice(bag.model)
        f.modelCaption:SetText(model and model.name or "Choose a bag")
        f.mountCaption:SetText(mountNames[bag.mount] or "Back")
        if model and model.colors then
            f.colorChoices:Show()
            for i,swatch in ipairs(f.colorSwatches) do
                local color=model.colors[i]
                if color then
                    swatch.color=color;swatch.modelID=color.id;swatch.bagID=bag.id;swatch:Show()
                    swatch.fill:SetVertexColor(color.r,color.g,color.b)
                    if bag.model==color.id then swatch.selectedBorder:Show();swatch.selectedCheck:Show()
                    else swatch.selectedBorder:Hide();swatch.selectedCheck:Hide() end
                    f.enableControl(swatch,available)
                else swatch:Hide();swatch.color=nil;swatch.modelID=nil;swatch.bagID=nil end
            end
        else f.colorChoices:Hide() end
    elseif emptySlot then
        f.target:Hide();f.bagSelectors:Show();f.colorChoices:Hide()
        f.modelCaption:SetText("Choose a bag");f.mountCaption:SetText("Back")
    else f.target:Show();f.bagSelectors:Hide();f.colorChoices:Hide() end
    f.enableControl(f.modelSelector,(bag and available) or emptyAvailable)
    f.enableControl(f.mountSelector,bag and available)
    f.status:SetText(f.message or state.status or "")
    f.live:SetChecked(state.enabled)
    f.enableControl(f.live,available)
    for _,row in ipairs(f.rows) do
        local e=row.editor
        if targetChanged and e.editing then
            e.editing=nil;e.cancelCommit=true;e:ClearFocus();e.cancelCommit=nil
        end
        if not e.editing then e:SetText(emptySlot and "" or displayValue(e.field,(state.values or {})[e.field.key])) end
        e:EnableMouse(available);e:SetAlpha(available and 1 or .45)
        row.caption:SetAlpha(available and 1 or .45)
        f.enableControl(row.minus,available);f.enableControl(row.plus,available);f.enableControl(row.reset,available)
    end
    f.enableControl(f.save,available)
    f.enableControl(f.load,available and state.saved)
    f.enableControl(f.reset,available)
    f.enableControl(f.export,available)
    if f.place then
        if bag then f.place:Show() else f.place:Hide() end
        f.enableControl(f.place,available and state.enabled and VanityStudioCharacter.enabled and bag~=nil)
    end
    f.refreshing=nil
end
function V:CreateBagTunerUI(sheet,section,label,edit,settingsButton,enabled)
    if self.bagTunerWindow then return end
    local f=sheet("SaureksClosetBagTuner",UIParent,"Placement Tuner")
    self.bagTunerWindow=f;f.enableControl=enabled;f.rows={}
    f:Hide();f:SetFrameStrata("DIALOG");f:SetClampedToScreen(true)
    f:SetPoint("TOPLEFT",self.frame,"TOPRIGHT",-30,0)
    f:SetMovable(true);f:RegisterForDrag("LeftButton")
    f:SetScript("OnDragStart",function() this:StartMoving() end)
    f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
    f.close:SetScript("OnClick",function() V.bagTunerWindow:Hide() end)
    f:SetScript("OnHide",function()
        if V.CloseBagPlacementEditor then V:CloseBagPlacementEditor() end
        GameTooltip:Hide()
        V:CloseBagTunerModelPicker()
        if this.mountMenu and UIDROPDOWNMENU_OPEN_MENU==this.mountMenu:GetName() then CloseDropDownMenus() end
        for _,row in ipairs(this.rows) do
            if row.editor.editing then V:CommitBagTunerEditor(row.editor);row.editor:ClearFocus() end
        end
        this.invalidInput=nil;this.invalidEditor=nil;this.resetHover=nil
        this.emptySlot=nil
        V:SetBagTunerPaused(false)
        if V.placementTunerSlot and V.placementTunerSlot>=108 then V:RefreshPreview() end
        if V.RefreshBagsPage then V:RefreshBagsPage() end
    end)
    f:SetScript("OnUpdate",function()
        local elapsed=arg1 or 0
        if this.messageTime then
            this.messageTime=this.messageTime-elapsed
            if this.messageTime<=0 then this.messageTime=nil;this.message=nil end
        end
        this.elapsed=(this.elapsed or 0)+elapsed
        if this.elapsed<.2 then return end
        this.elapsed=0;V:RefreshBagTunerUI()
    end)
    table.insert(UISpecialFrames,f:GetName())
    f.target=label(f,"",31,81,302,43)
    f.target:SetFont("Fonts\\FRIZQT__.TTF",12)
    f.bagSelectors=CreateFrame("Frame",nil,f);f.bagSelectors:SetAllPoints(f)
    local function selector(kind,title,name,y,width,help)
        local caption=label(f.bagSelectors,title,31,y+3,41,20,true)
        caption:SetFont("Fonts\\FRIZQT__.TTF",11);caption:SetJustifyV("MIDDLE")
        local control=section(f.bagSelectors,74,y,width,24,true,"Button",.75)
        -- Native dropdowns require a named anchor; the button itself can stay
        -- anonymous so it shares the existing section border/highlight style.
        local anchor=CreateFrame("Frame",name,control);anchor:SetAllPoints(control)
        local text=label(control,"",8,2,width-35,20,true)
        text:SetFont("Fonts\\FRIZQT__.TTF",11);text:SetJustifyV("MIDDLE")
        local icon=control:CreateTexture(nil,"ARTWORK")
        icon:SetPoint("TOPRIGHT",control,"TOPRIGHT",-1,-1);icon:SetWidth(22);icon:SetHeight(22)
        icon:SetTexture("Interface\\Buttons\\UI-ScrollBar-ScrollDownButton-Up")
        control:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        tooltip(control,help)
        if kind=="model" then
            control:SetScript("OnClick",function() V:OpenBagTunerModelPicker() end)
            return control,text
        end
        local menu=CreateFrame("Frame",name.."Menu",f);menu.displayMode="MENU";menu:Hide()
        menu.initialize=function()
            local bag=V.placementTunerBag and V:BagInstance(V.placementTunerBag)
            if not bag then return end
            local bagID=bag.id
            local function select(value)
                if not V:PrepareBagTunerSelection(bagID) then return end
                local ok=V:SetBagMount(bagID,value,true)
                if ok then
                    f.message=nil;f.messageTime=nil
                    CloseDropDownMenus();V:RefreshBagTunerUI()
                end
            end
            for _,mount in ipairs(mountOrder) do
                UIDropDownMenu_AddButton({text=mountNames[mount],checked=bag.mount==mount and 1 or nil,
                    arg1=mount,func=select})
            end
        end
        control:SetScript("OnClick",function()
            V:CloseBagTunerModelPicker()
            local id=V.placementTunerBag
            if not V:PrepareBagTunerSelection(id) then return end
            GameTooltip:Hide();ToggleDropDownMenu(1,nil,menu,name,0,0)
        end)
        return control,text,menu
    end
    f.modelSelector,f.modelCaption=selector("model","Model","SaureksClosetBagTunerModel",76,257,
        "Choose this bag's model. Your current position, rotation and size are kept.")
    f.mountSelector,f.mountCaption,f.mountMenu=selector("mount","Start","SaureksClosetBagTunerMount",104,105,
        "Start on the Back, Left hip or Right hip. Changing the starting position resets this bag's tuned fits.")
    local picker=CreateFrame("Frame","SaureksClosetBagTunerModelPicker",f);f.modelPicker=picker
    local pickerHeight=math.ceil(table.getn(self.bagModelChoices)/2)*64+14
    picker:SetPoint("TOPLEFT",f,"TOPLEFT",27,-102);picker:SetWidth(310);picker:SetHeight(pickerHeight)
    picker:SetFrameStrata("DIALOG");picker:SetFrameLevel(f:GetFrameLevel()+40);picker:EnableMouse(true)
    picker:SetBackdrop({bgFile="Interface\\Buttons\\WHITE8X8",
        edgeFile="Interface\\Tooltips\\UI-Tooltip-Border",tile=true,tileSize=16,edgeSize=12,
        insets={left=3,right=3,top=3,bottom=3}})
    picker:SetBackdropColor(.035,.032,.025,1);picker:SetBackdropBorderColor(.68,.59,.39)
    -- The native dialog finish is only 60% opaque. Put it over a solid base
    -- so the tuner's numeric controls cannot show through this visual menu.
    local finish=CreateFrame("Frame",nil,picker)
    finish:SetPoint("TOPLEFT",picker,"TOPLEFT",5,-5);finish:SetWidth(300);finish:SetHeight(pickerHeight-10)
    finish:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",tile=true,tileSize=16})
    finish:SetBackdropColor(1,1,1,1)
    picker.rows={};picker:Hide()
    picker:SetScript("OnHide",function()
        this.bagID=nil;this.emptySlot=nil;this.targetKey=nil;GameTooltip:Hide()
        for _,row in ipairs(this.rows) do row.bagID=nil;row.emptySlot=nil;row.targetKey=nil end
    end)
    -- In 1.12 Escape closes UIMenus before ordinary windows. Register the
    -- popover there so dismissing it keeps the placement tuner open.
    UIMenus=UIMenus or {};table.insert(UIMenus,picker:GetName())
    for i,choice in ipairs(self.bagModelChoices) do
        local row=CreateFrame("Button",nil,picker)
        row:SetPoint("TOPLEFT",picker,"TOPLEFT",8+math.mod(i-1,2)*149,-8-math.floor((i-1)/2)*64)
        row:SetWidth(145);row:SetHeight(62);row.choice=choice;row.modelID=choice.id
        row:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",
            edgeFile="Interface\\Tooltips\\UI-Tooltip-Border",tile=true,tileSize=16,edgeSize=10,
            insets={left=2,right=2,top=2,bottom=2}})
        row:SetBackdropColor(1,1,1,1)
        row.selected=row:CreateTexture(nil,"BACKGROUND")
        row.selected:SetPoint("TOPLEFT",row,"TOPLEFT",3,-3);row.selected:SetWidth(139);row.selected:SetHeight(56)
        row.selected:SetTexture("Interface\\Buttons\\WHITE8X8");row.selected:SetVertexColor(.43,.30,.08,.55)
        row.icon=row:CreateTexture(nil,"ARTWORK")
        row.icon:SetPoint("TOPLEFT",row,"TOPLEFT",7,-11);row.icon:SetWidth(40);row.icon:SetHeight(40)
        row.icon:SetTexture(choice.icon)
        row.caption=label(row,choice.name,54,4,84,36,true)
        row.caption:SetFont("Fonts\\FRIZQT__.TTF",10);row.caption:SetJustifyV("MIDDLE")
        row.bodyType=label(row,V:BagBodyType(choice.id),54,44,84,12,true)
        row.bodyType:SetFont("Fonts\\FRIZQT__.TTF",9);row.bodyType:SetTextColor(.83,.76,.58)
        row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        row:SetScript("OnClick",function()
            local id=this.bagID;local slot=this.emptySlot;local key=this.targetKey;local chosen=this.choice
            if not picker:IsShown() or picker.bagID~=id or picker.emptySlot~=slot or picker.targetKey~=key then return end
            local state=V:GetBagTunerState()
            if slot then
                if not f:IsShown() or f.emptySlot~=slot or state.key~=key or V:BagInSlot(slot) then V:CloseBagTunerModelPicker();return end
                V:CloseBagTunerModelPicker()
                local ok,bag=V:AddBag(chosen.id,"back",slot)
                if ok and bag then V:OpenBagTuner(bag.id)
                else showMessage("This bag could not be added. Try again.");V:RefreshBagTunerUI() end
                return
            end
            if state.key~=key or not V:PrepareBagTunerSelection(id) then V:CloseBagTunerModelPicker();return end
            local bag=V:BagInstance(id)
            local model=V:BagModelChoice(bag.model)==chosen and bag.model or chosen.id
            V:CloseBagTunerModelPicker()
            if V:SetBagModel(id,model,true) then
                f.message=nil;f.messageTime=nil;V:RefreshBagTunerUI()
            end
        end)
        tooltip(row,choice.name..(choice.colors and "\nChoose a color after selecting this model." or "").."\nYour current fit is kept.")
        table.insert(picker.rows,row)
    end
    f.colorChoices=CreateFrame("Frame",nil,f.bagSelectors)
    f.colorChoices:SetPoint("TOPLEFT",f.bagSelectors,"TOPLEFT",187,-104);f.colorChoices:SetWidth(144);f.colorChoices:SetHeight(24)
    f.colorCaption=label(f.colorChoices,"Color",0,2,45,20,true)
    f.colorCaption:SetFont("Fonts\\FRIZQT__.TTF",11);f.colorCaption:SetJustifyV("MIDDLE")
    f.colorSwatches={}
    local colorCount=0
    for _,choice in ipairs(self.bagModelChoices) do colorCount=math.max(colorCount,table.getn(choice.colors or {})) end
    for i=1,colorCount do
        local swatch=CreateFrame("Button",nil,f.colorChoices)
        swatch:SetPoint("TOPLEFT",f.colorChoices,"TOPLEFT",50+(i-1)*24,-1);swatch:SetWidth(22);swatch:SetHeight(22)
        local border=swatch:CreateTexture(nil,"BACKGROUND");border:SetAllPoints(swatch)
        border:SetTexture("Interface\\Buttons\\WHITE8X8");border:SetVertexColor(.36,.32,.24)
        swatch.selectedBorder=swatch:CreateTexture(nil,"BORDER");swatch.selectedBorder:SetAllPoints(swatch)
        swatch.selectedBorder:SetTexture("Interface\\Buttons\\WHITE8X8");swatch.selectedBorder:SetVertexColor(1,.82,0)
        swatch.fill=swatch:CreateTexture(nil,"ARTWORK");swatch.fill:SetPoint("TOPLEFT",swatch,"TOPLEFT",3,-3)
        swatch.fill:SetWidth(16);swatch.fill:SetHeight(16);swatch.fill:SetTexture("Interface\\Buttons\\WHITE8X8")
        swatch.selectedCheck=swatch:CreateTexture(nil,"OVERLAY");swatch.selectedCheck:SetPoint("CENTER",swatch,"CENTER",0,0)
        swatch.selectedCheck:SetWidth(18);swatch.selectedCheck:SetHeight(18);swatch.selectedCheck:SetTexture("Interface\\Buttons\\UI-CheckBox-Check")
        swatch:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square","ADD")
        swatch:SetScript("OnClick",function()
            local id=this.bagID;local color=this.color
            if not color or not V:PrepareBagTunerSelection(id) then return end
            local bag=V:BagInstance(id)
            if V:BagModelChoice(bag.model)~=V:BagModelChoice(color.id) then return end
            if V:SetBagModel(id,color.id,true) then
                f.message=nil;f.messageTime=nil;CloseDropDownMenus();V:RefreshBagTunerUI()
            end
        end)
        swatch:SetScript("OnEnter",function()
            if not this.color then return end
            GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:ClearLines()
            GameTooltip:AddLine(this.color.name,1,1,1)
            GameTooltip:AddLine("Change the bag color. Your fit is kept.",1,.82,0,true);GameTooltip:Show()
        end)
        swatch:SetScript("OnLeave",function() GameTooltip:Hide() end)
        table.insert(f.colorSwatches,swatch)
    end
    f.status=label(f,"",31,130,302,20,true)
    f.status:SetFont("Fonts\\FRIZQT__.TTF",10)
    local function checkbox(name,text,x,width,callback,help)
        local b=CreateFrame("CheckButton",name,f,"UICheckButtonTemplate")
        b:SetPoint("TOPLEFT",f,"TOPLEFT",x,-151);b:SetWidth(24);b:SetHeight(24)
        b:SetHitRectInsets(0,-width+24,0,0);b:SetScript("OnClick",callback)
        local caption=label(f,text,x+27,152,width-27,22,true)
        caption:SetFont("Fonts\\FRIZQT__.TTF",11);caption:SetJustifyV("MIDDLE")
        tooltip(b,help)
        return b
    end
    f.live=checkbox("SaureksClosetBagTunerLive","Live tuning",29,145,function()
        V:SetBagTunerEnabled(this:GetChecked() and true or false);V:RefreshBagTunerUI()
    end,"Preview your current fitting values on the character. Turn this off to compare with the program's default fit.")
    local function nudge(parent,text,x,field,direction)
        local b=section(parent,x,0,22,22,true,"Button",.75)
        local t=label(b,text,0,1,22,20,true)
        t:SetFont("Fonts\\FRIZQT__.TTF",14);t:SetJustifyH("CENTER");t:SetJustifyV("MIDDLE")
        b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        b.field=field;b.direction=direction
        b:SetScript("OnMouseDown",function() this.targetKey=V.bagTunerWindow.targetKey end)
        b:SetScript("OnClick",function()
            local owner=this;local window=V.bagTunerWindow
            local targetKey=owner.targetKey or window.targetKey;owner.targetKey=nil
            local state=V:GetBagTunerState()
            if not state.available or state.key~=targetKey then
                showMessage(state.available and "The placement changed. Click again for the current model." or state.status)
                V:RefreshBagTunerUI();return
            end
            for _,row in ipairs(window.rows) do
                if row.editor.editing then
                    local accepted=V:CommitBagTunerEditor(row.editor);row.editor:ClearFocus()
                    if not accepted then window.invalidInput=nil;return end
                end
            end
            -- Focus may leave the editor before the button receives OnClick.
            -- A rejected number must not fall through into a different change.
            if window.invalidInput then window.invalidInput=nil;return end
            state=V:GetBagTunerState()
            if not state.available or state.key~=targetKey then
                showMessage(state.available and "The placement changed. Click again for the current model." or state.status)
                V:RefreshBagTunerUI();return
            end
            local coarse=IsShiftKeyDown and IsShiftKeyDown() and 10 or 1
            local value=((state.values or {})[owner.field.key] or 0)+owner.field.step*owner.direction*coarse
            local low,high=V:BagTunerFieldBounds(owner.field,state.bag)
            value=math.max(low,math.min(high,value))
            local ok,err=V:SetBagTunerValue(owner.field.key,value)
            if not ok then showMessage(err or "That value could not be applied.") end
            V:RefreshBagTunerUI()
        end)
        tooltip(b,function() return fieldHelp(this.field).."\n\nClick to adjust by "..this.field.step..". Hold Shift for a larger step." end)
        return b
    end
    for i,field in ipairs(self.bagTunerFields) do
        local row=CreateFrame("Frame",nil,f)
        row:SetPoint("TOPLEFT",f,"TOPLEFT",31,-178-(i-1)*23);row:SetWidth(300);row:SetHeight(22)
        local caption=label(row,field.label,0,2,102,20,true)
        caption:SetFont("Fonts\\FRIZQT__.TTF",11);caption:SetJustifyV("MIDDLE")
        local e=edit(row,"SaureksClosetBagTuner"..field.key,139,0,67,16)
        e.field=field;e:SetJustifyH("CENTER")
        e:SetScript("OnEditFocusGained",function()
            this.editing=true;this.targetKey=V:GetBagTunerState().key
        end)
        e:SetScript("OnEditFocusLost",function()
            -- Reset is deliberately independent of every other unfinished field.
            -- Mouse focus may move before OnClick; retain that draft until its
            -- own Enter/Reset or an explicit Save/Export action handles it.
            if not this.cancelCommit and V.bagTunerWindow.resetHover then return end
            if not this.cancelCommit then V:CommitBagTunerEditor(this) end
            this.editing=nil
        end)
        e:SetScript("OnEnterPressed",function() V:CommitBagTunerEditor(this);this:ClearFocus() end)
        e:SetScript("OnEscapePressed",function()
            this.editing=nil;this.cancelCommit=true;this:ClearFocus();this.cancelCommit=nil;V:RefreshBagTunerUI()
        end)
        -- Lua 5.0 reuses the generic-for variable; deferred handlers must
        -- read the field stored on their widget after this loop has finished.
        tooltip(e,function() return fieldHelp(this.field) end)
        local reset=section(row,247,0,53,22,true,"Button",.75)
        local resetText=label(reset,"Reset",0,1,53,20,true)
        resetText:SetFont("Fonts\\FRIZQT__.TTF",10);resetText:SetJustifyH("CENTER");resetText:SetJustifyV("MIDDLE")
        reset.editor=e;reset:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        tooltip(reset,"Restore only this field to the built-in fit for the current placement, race and gender. Other fields and saved fits are unchanged.")
        local enter,leave=reset:GetScript("OnEnter"),reset:GetScript("OnLeave")
        reset:SetScript("OnEnter",function()
            if this.closetEnabled then V.bagTunerWindow.resetHover=this end
            enter()
        end)
        reset:SetScript("OnLeave",function()
            if V.bagTunerWindow.resetHover==this then V.bagTunerWindow.resetHover=nil end
            leave()
        end)
        reset:SetScript("OnMouseDown",function() this.targetKey=V.bagTunerWindow.targetKey end)
        reset:SetScript("OnClick",function()
            local owner=this;local window=V.bagTunerWindow
            local targetKey=owner.targetKey or window.targetKey;owner.targetKey=nil
            local state=V:GetBagTunerState()
            if not state.available or state.key~=targetKey then
                showMessage(state.available and "The placement changed. Click Reset again for the current model." or state.status)
                V:RefreshBagTunerUI();return
            end
            local editor=owner.editor
            editor.editing=nil;editor.cancelCommit=true;editor:ClearFocus();editor.cancelCommit=nil
            if window.invalidEditor==editor then window.invalidInput=nil;window.invalidEditor=nil end
            local ok,err=V:ResetBagTunerField(editor.field.key)
            if ok then window.message=nil;window.messageTime=nil
            else showMessage(err or "That field could not be reset.") end
            V:RefreshBagTunerUI()
        end)
        table.insert(f.rows,{editor=e,caption=caption,minus=nudge(row,"-",108,field,-1),plus=nudge(row,"+",216,field,1),reset=reset})
    end
    local hint=label(f,"Shift-click +/- for larger steps.",31,342,166,15,true)
    hint:SetFont("Fonts\\FRIZQT__.TTF",10);hint:SetTextColor(.72,.72,.72)
    if self.CreateBagPlacementEditor then
        self:CreateBagPlacementEditor(sheet,label,settingsButton)
        f.place=settingsButton(f,"Place on model",199,337,132,function()
            if not V:PrepareBagTunerSelection(V.placementTunerBag) then return end
            local ok,err=V:OpenBagPlacementEditor()
            if not ok then showMessage(err);V:RefreshBagTunerUI() end
        end,.75)
        tooltip(f.place,"Drag to place this bag on a body guide. Its back faces toward the player. Use the fitting controls afterward to adjust for clothing.")
    end
    local function action(name,text,x,y,method)
        local b=settingsButton(f,text,x,y,145,function()
            for _,row in ipairs(V.bagTunerWindow.rows) do
                if row.editor.editing then
                    local accepted=V:CommitBagTunerEditor(row.editor);row.editor:ClearFocus()
                    if not accepted then V.bagTunerWindow.invalidInput=nil;return end
                end
            end
            if V.bagTunerWindow.invalidInput then V.bagTunerWindow.invalidInput=nil;return end
            local ok,err=V[method](V)
            if ok then V.bagTunerWindow.message=nil;V.bagTunerWindow.messageTime=nil
            else showMessage(err or "The fit could not be changed.") end
            V:RefreshBagTunerUI()
        end,.75)
        f[name]=b
    end
    action("save","Save Fit",31,362,"SaveBagTunerFit")
    action("load","Load Saved",186,362,"LoadBagTunerFit")
    action("reset","Reset",31,392,"ResetBagTunerFit")
    f.export=settingsButton(f,"Export",186,392,145,function() V:OpenBagTunerExport() end,.75)
    tooltip(f.save,"Save this fit for the current placement, race and gender. Saved fits remain available after restarting the game.")
    tooltip(f.load,"Replace the current draft with the last fit saved for this placement, race and gender.")
    tooltip(f.reset,"Restore the current draft to the program's default fit. Your previously saved fit remains available.")
    tooltip(f.export,"Open a copyable report containing the current draft and all your saved fits, ready to send for implementation.")
    self:RefreshBagTunerUI()
end
function V:OpenBagSlotTuner(slot)
    if type(slot)~="number" or slot~=math.floor(slot) or slot<1 or slot>self.MAX_BAGS then return false end
    local bag=self:BagInSlot(slot)
    if bag then self:OpenBagTuner(bag.id);return true end
    if not self:MultiBagRendererAvailable() then return false end
    if not self.frame then self:Toggle(true) end
    local f=self.bagTunerWindow
    if not f then return false end
    if f:IsShown() then f:Hide() end
    self.placementTunerSlot=nil;self.placementTunerBag=nil;self.bagTunerTargetKey=nil
    f.emptySlot=slot;f.message=nil;f.messageTime=nil
    f:Show();self:RefreshBagTunerUI();self:OpenBagTunerModelPicker()
    if self.RefreshBagsPage then self:RefreshBagsPage() end
    return true
end
function V:OpenBagTuner(instanceID)
    if instanceID and self.BagInstance and not self:BagInstance(instanceID) then return end
    if not self.frame then self:Toggle(true) end
    if not self.bagTunerWindow then return end
    if self.bagTunerWindow:IsShown() then self.bagTunerWindow:Hide() end
    self.bagTunerWindow.emptySlot=nil
    self.placementTunerSlot=nil
    local first=self.GetBags and self:GetBags()[1]
    self.placementTunerBag=instanceID or (first and first.id)
    self.bagTunerWindow.message=nil;self.bagTunerWindow.messageTime=nil
    self.bagTunerWindow:Show();self:RefreshBagTunerUI()
    if self.RefreshBagsPage then self:RefreshBagsPage() end
end
function V:OpenPlacementTuner(slot)
    if not self:IsWeaponPosition(slot) then return end
    if slot>=108 and not self:HeldWeaponTuningAvailable() then return end
    if not self.frame then self:Toggle(true) end
    if not self.bagTunerWindow then return end
    if self.bagTunerWindow:IsShown() then self.bagTunerWindow:Hide() end
    self.bagTunerWindow.emptySlot=nil
    self.placementTunerSlot=slot;self.placementTunerBag=nil
    self.bagTunerWindow.message=nil;self.bagTunerWindow.messageTime=nil
    self.bagTunerWindow:Show();self:RefreshBagTunerUI()
    if slot>=108 then self:RefreshPreview() end
end
function V:OpenBagTunerExport()
    if not self.bagTunerWindow then return end
    for _,row in ipairs(self.bagTunerWindow.rows) do
        if row.editor.editing then
            local accepted=self:CommitBagTunerEditor(row.editor);row.editor:ClearFocus()
            if not accepted then self.bagTunerWindow.invalidInput=nil;return end
        end
    end
    if self.bagTunerWindow.invalidInput then self.bagTunerWindow.invalidInput=nil;return end
    local export=self:ExportBagTunerFits()
    if type(export)~="string" then
        showMessage("The current placement fit is not available for export.");self:RefreshBagTunerUI();return
    end
    if not self.bagTunerExportWindow then
        local f=CreateFrame("Frame","SaureksClosetBagTunerExport",UIParent)
        self.bagTunerExportWindow=f;f:Hide()
        f:SetWidth(530);f:SetHeight(410);f:SetPoint("CENTER",UIParent,"CENTER",0,0)
        f:SetFrameStrata("DIALOG");f:SetClampedToScreen(true);f:SetMovable(true);f:EnableMouse(true)
        f:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border",
            tile=true,tileSize=32,edgeSize=32,insets={left=11,right=12,top=12,bottom=11}})
        f:RegisterForDrag("LeftButton")
        f:SetScript("OnDragStart",function() this:StartMoving() end)
        f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
        local title=f:CreateFontString(nil,"OVERLAY","GameFontNormal")
        title:SetPoint("TOP",f,"TOP",0,-19);title:SetText("Placement Fit Export")
        local close=CreateFrame("Button","SaureksClosetBagTunerExportClose",f,"UIPanelCloseButton")
        close:SetPoint("TOPRIGHT",f,"TOPRIGHT",-6,-6)
        close:SetScript("OnClick",function() V.bagTunerExportWindow:Hide() end)
        local instructions=f:CreateFontString(nil,"OVERLAY","GameFontHighlightSmall")
        instructions:SetPoint("TOPLEFT",f,"TOPLEFT",23,-45);instructions:SetWidth(474);instructions:SetHeight(30)
        instructions:SetJustifyH("LEFT");instructions:SetText("Press Ctrl+C to copy these fitting values. Save the text or send it for implementation.")
        local scroll=CreateFrame("ScrollFrame","SaureksClosetBagTunerExportScroll",f,"UIPanelScrollFrameTemplate")
        scroll:SetPoint("TOPLEFT",f,"TOPLEFT",23,-83);scroll:SetWidth(462);scroll:SetHeight(294)
        local e=CreateFrame("EditBox","SaureksClosetBagTunerExportText",scroll)
        f.editor=e;f.scroll=scroll
        e:SetPoint("TOPLEFT",scroll,"TOPLEFT",0,0)
        e:SetWidth(455);e:SetHeight(294);e:SetMultiLine(true);e:SetAutoFocus(false)
        e:SetFontObject(GameFontHighlightSmall);e:SetMaxLetters(0)
        e:SetScript("OnEscapePressed",function() V.bagTunerExportWindow:Hide() end)
        e:SetScript("OnTextChanged",function()
            ScrollingEdit_OnTextChanged(V.bagTunerExportWindow.scroll)
        end)
        e:SetScript("OnCursorChanged",function()
            ScrollingEdit_OnCursorChanged(arg1,arg2,arg3,arg4)
        end)
        e:SetScript("OnUpdate",function() ScrollingEdit_OnUpdate(V.bagTunerExportWindow.scroll) end)
        scroll:SetScrollChild(e)
        f:SetScript("OnHide",function() this.editor:ClearFocus() end)
        table.insert(UISpecialFrames,f:GetName())
    end
    local f=self.bagTunerExportWindow
    f.editor:SetText(export);f:Show()
    f.editor:SetFocus();f.editor:HighlightText()
    f.editor.cursorOffset=nil;f.scroll:SetVerticalScroll(0)
end
