-- Local fitting tools; drafts and exports are handled by BagTuner.lua.
local V=VanityStudio
local function tooltip(widget,text)
    widget:SetScript("OnEnter",function()
        GameTooltip:SetOwner(this,"ANCHOR_RIGHT");GameTooltip:ClearLines()
        GameTooltip:AddLine(text,1,1,1,true);GameTooltip:Show()
    end)
    widget:SetScript("OnLeave",function() GameTooltip:Hide() end)
end
local function showMessage(text)
    if not V.bagTunerWindow then return end
    V.bagTunerWindow.message=text
    V.bagTunerWindow.messageTime=4
end
local function displayValue(field,value)
    return string.format("%."..(field.decimals or 2).."f",value or 0)
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
        showMessage("The character changed. The unfinished edit was discarded.")
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
    local available=state.available and true or false
    local targetChanged=f.targetKey~=state.key
    f.targetKey=state.key
    f.target:SetText(state.title or "Bag fitting")
    f.status:SetText(f.message or state.status or "")
    f.live:SetChecked(state.enabled)
    f.pause:SetChecked(state.paused)
    f.enableControl(f.live,available)
    f.enableControl(f.pause,available and state.enabled)
    for _,row in ipairs(f.rows) do
        local e=row.editor
        if targetChanged and e.editing then
            e.editing=nil;e.cancelCommit=true;e:ClearFocus();e.cancelCommit=nil
        end
        if not e.editing then e:SetText(displayValue(e.field,(state.values or {})[e.field.key])) end
        e:EnableMouse(available);e:SetAlpha(available and 1 or .45)
        row.caption:SetAlpha(available and 1 or .45)
        f.enableControl(row.minus,available);f.enableControl(row.plus,available);f.enableControl(row.reset,available)
    end
    f.enableControl(f.save,available)
    f.enableControl(f.load,available and state.saved)
    f.enableControl(f.reset,available)
    f.enableControl(f.export,available)
    f.savedState:SetText(state.dirty and "Unsaved changes" or (state.saved and "Saved fit" or "Default fit"))
    f.refreshing=nil
end
function V:CreateBagTunerUI(sheet,section,label,edit,settingsButton,enabled)
    if self.bagTunerWindow then return end
    local f=sheet("SaureksClosetBagTuner",UIParent,"Bag Tuner")
    self.bagTunerWindow=f;f.enableControl=enabled;f.rows={}
    f:Hide();f:SetFrameStrata("DIALOG");f:SetClampedToScreen(true)
    f:SetPoint("TOPLEFT",self.frame,"TOPRIGHT",-30,0)
    f:SetMovable(true);f:RegisterForDrag("LeftButton")
    f:SetScript("OnDragStart",function() this:StartMoving() end)
    f:SetScript("OnDragStop",function() this:StopMovingOrSizing() end)
    f.close:SetScript("OnClick",function() V.bagTunerWindow:Hide() end)
    f:SetScript("OnHide",function()
        GameTooltip:Hide()
        for _,row in ipairs(this.rows) do
            if row.editor.editing then V:CommitBagTunerEditor(row.editor);row.editor:ClearFocus() end
        end
        this.invalidInput=nil;this.invalidEditor=nil;this.resetHover=nil
        V:SetBagTunerPaused(false)
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
    f.target=label(f,"",31,81,302,30)
    f.target:SetFont("Fonts\\FRIZQT__.TTF",12)
    f.status=label(f,"",31,112,302,26,true)
    f.status:SetFont("Fonts\\FRIZQT__.TTF",10)
    local function checkbox(name,text,x,width,callback,help)
        local b=CreateFrame("CheckButton",name,f,"UICheckButtonTemplate")
        b:SetPoint("TOPLEFT",f,"TOPLEFT",x,-139);b:SetWidth(24);b:SetHeight(24)
        b:SetHitRectInsets(0,-width+24,0,0);b:SetScript("OnClick",callback)
        local caption=label(f,text,x+27,140,width-27,22,true)
        caption:SetFont("Fonts\\FRIZQT__.TTF",11);caption:SetJustifyV("MIDDLE")
        tooltip(b,help)
        return b
    end
    f.live=checkbox("SaureksClosetBagTunerLive","Live tuning",29,145,function()
        V:SetBagTunerEnabled(this:GetChecked() and true or false);V:RefreshBagTunerUI()
    end,"Preview your current fitting values on the character. Turn this off to compare with the program's default fit.")
    f.pause=checkbox("SaureksClosetBagTunerPause","Pause motion",185,146,function()
        V:SetBagTunerPaused(this:GetChecked() and true or false);V:RefreshBagTunerUI()
    end,"Hold the bag still relative to its attachment while adjusting its fit. Closing this window resumes motion.")
    local function nudge(parent,text,x,field,direction)
        local b=section(parent,x,0,22,22,true,"Button",.75)
        local t=label(b,text,0,1,22,20,true)
        t:SetFont("Fonts\\FRIZQT__.TTF",14);t:SetJustifyH("CENTER");t:SetJustifyV("MIDDLE")
        b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        b.field=field;b.direction=direction
        b:SetScript("OnClick",function()
            local owner=this
            for _,row in ipairs(V.bagTunerWindow.rows) do
                if row.editor.editing then V:CommitBagTunerEditor(row.editor);row.editor:ClearFocus() end
            end
            local state=V:GetBagTunerState()
            local coarse=IsShiftKeyDown and IsShiftKeyDown() and 10 or 1
            local value=((state.values or {})[owner.field.key] or 0)+owner.field.step*owner.direction*coarse
            value=math.max(owner.field.min,math.min(owner.field.max,value))
            local ok,err=V:SetBagTunerValue(owner.field.key,value)
            if not ok then showMessage(err or "That value could not be applied.") end
            V:RefreshBagTunerUI()
        end)
        tooltip(b,field.help.."\n\nClick to adjust by "..field.step..". Hold Shift for a larger step.")
        return b
    end
    for i,field in ipairs(self.bagTunerFields) do
        local row=CreateFrame("Frame",nil,f)
        row:SetPoint("TOPLEFT",f,"TOPLEFT",31,-166-(i-1)*23);row:SetWidth(300);row:SetHeight(22)
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
        tooltip(e,field.help)
        local reset=section(row,247,0,53,22,true,"Button",.75)
        local resetText=label(reset,"Reset",0,1,53,20,true)
        resetText:SetFont("Fonts\\FRIZQT__.TTF",10);resetText:SetJustifyH("CENTER");resetText:SetJustifyV("MIDDLE")
        reset.editor=e;reset:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight","ADD")
        tooltip(reset,"Restore only this field to the built-in fit for the current bag, race and gender. Other fields and saved fits are unchanged.")
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
                showMessage(state.available and "The character changed. Click Reset again for the current model." or state.status)
                V:RefreshBagTunerUI();return
            end
            local editor=owner.editor
            editor.editing=nil;editor.cancelCommit=true;editor:ClearFocus();editor.cancelCommit=nil
            if window.invalidEditor==editor then window.invalidInput=nil;window.invalidEditor=nil end
            local ok,err=V:ResetBagTunerField(editor.field.key)
            showMessage(ok and (editor.field.label.." restored to its default.") or (err or "That field could not be reset."))
            V:RefreshBagTunerUI()
        end)
        table.insert(f.rows,{editor=e,caption=caption,minus=nudge(row,"-",108,field,-1),plus=nudge(row,"+",216,field,1),reset=reset})
    end
    local hint=label(f,"Shift-click +/- for larger steps.",31,332,300,15,true)
    hint:SetFont("Fonts\\FRIZQT__.TTF",10);hint:SetTextColor(.72,.72,.72)
    local function action(name,text,x,y,method,message)
        local b=settingsButton(f,text,x,y,145,function()
            for _,row in ipairs(V.bagTunerWindow.rows) do
                if row.editor.editing then
                    local accepted=V:CommitBagTunerEditor(row.editor);row.editor:ClearFocus()
                    if not accepted then V.bagTunerWindow.invalidInput=nil;return end
                end
            end
            if V.bagTunerWindow.invalidInput then V.bagTunerWindow.invalidInput=nil;return end
            local ok,err=V[method](V)
            showMessage(ok and message or (err or "The fit could not be changed."))
            V:RefreshBagTunerUI()
        end,.75)
        f[name]=b
    end
    action("save","Save Fit",31,353,"SaveBagTunerFit","Fit saved for this bag, race and gender.")
    action("load","Load Saved",186,353,"LoadBagTunerFit","Saved fit loaded.")
    action("reset","Reset",31,385,"ResetBagTunerFit","Draft restored to the program's default fit.")
    f.export=settingsButton(f,"Export",186,385,145,function() V:OpenBagTunerExport() end,.75)
    tooltip(f.save,"Save this fit for the current bag, race and gender. Saved fits remain available after restarting the game.")
    tooltip(f.load,"Replace the current draft with the last fit saved for this bag, race and gender.")
    tooltip(f.reset,"Restore the current draft to the program's default fit. Your previously saved fit remains available.")
    tooltip(f.export,"Open a copyable report containing the current draft and all your saved fits, ready to send for implementation.")
    f.savedState=label(f,"",31,417,300,14,true)
    f.savedState:SetFont("Fonts\\FRIZQT__.TTF",10);f.savedState:SetJustifyH("CENTER")
    self:RefreshBagTunerUI()
end
function V:OpenBagTuner()
    if not self.frame then self:Toggle(true) end
    if not self.bagTunerWindow then return end
    if not (VanityStudioCharacter.weapons or {}).backBag then self:SelectBackBag(1) end
    self.bagTunerWindow:Show();self:RefreshBagTunerUI()
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
        showMessage("The current bag fit is not available for export.");self:RefreshBagTunerUI();return
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
        title:SetPoint("TOP",f,"TOP",0,-19);title:SetText("Bag Fit Export")
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
