-- Lua 5.0-compatible fixture: invoke the real deferred callbacks after the
-- generic-for loops finish. Also emulate exhausted loop upvalues under 5.1.
local function noop() end
local methods={}
local function widget(name)
    return setmetatable({name=name or 'test',scripts={},text=''}, {__index=function(_,key) return methods[key] or noop end})
end
function methods:SetScript(name,fn) self.scripts[name]=fn end
function methods:GetScript(name) return self.scripts[name] end
function methods:GetName() return self.name end
function methods:SetText(text) self.text=tostring(text) end
function methods:GetText() return self.text end
function methods:ClearFocus() self.cleared=true end
local function make() return widget() end
local function sheet(name) local f=widget(name);f.close=widget();return f end
local function button(_,_,_,_,_,callback) local b=widget();b:SetScript('OnClick',callback);return b end
CreateFrame=function(_,name) return widget(name) end
UIParent=widget();UISpecialFrames={};VanityStudio={frame=widget()}
local V=VanityStudio
GameTooltip={SetOwner=function(_,owner) assert(owner==this) end,
    SetText=function(_,text) GameTooltip.title=text end,
    AddLine=function(_,text) GameTooltip.tip=text end,Show=noop,Hide=noop}
dofile('addon/SaureksCloset/Haircraft.lua')
V:CreateHairMaskWindow(sheet,make,button,make)
local function exhaust(callback)
    for i=1,20 do
        local name=debug.getupvalue(callback,i)
        if not name then break end
        if name=='field' or name=='control' then debug.setupvalue(callback,i,nil) end
    end
end
local rows=V.hairMaskRows
for _,row in ipairs(rows) do
    for _,control in ipairs({row.editor,row.minus,row.plus}) do
        local callback=control:GetScript('OnEnter');exhaust(callback)
        this=control;callback()
        assert(GameTooltip.title==row.field.title..(row.field.key=='height' and ' (%)' or ' (degrees)') and GameTooltip.tip==row.field.tip)
        control:GetScript('OnLeave')()
    end
    for _,control in ipairs({row.minus,row.plus}) do exhaust(control:GetScript('OnClick')) end
    row.editor:SetText(row.field.default);this=row.minus;row.minus:GetScript('OnClick')()
    assert(tonumber(row.editor:GetText())==row.field.default-1)
    this=row.plus;row.plus:GetScript('OnClick')();assert(tonumber(row.editor:GetText())==row.field.default)
    row.editor:SetText(row.field.min);this=row.minus;row.minus:GetScript('OnClick')();assert(tonumber(row.editor:GetText())==row.field.min)
    row.editor:SetText(row.field.max);this=row.plus;row.plus:GetScript('OnClick')();assert(tonumber(row.editor:GetText())==row.field.max)
    row.editor:SetText('');this=row.plus;row.plus:GetScript('OnClick')();assert(tonumber(row.editor:GetText())==row.field.default+1)
    -- Native calls and nested UI events can change the global callback frame.
    local applied=0;V.ApplyHairMask=function() applied=applied+1;this=nil end
    this=row.editor;row.editor:GetScript('OnEnterPressed')();assert(applied==1 and row.editor.cleared)
    local opened=0;V.OpenHairMask=function() opened=opened+1;this=nil end
    this=row.editor;row.editor:GetScript('OnEscapePressed')();assert(opened==1)
end
print('PASS: every hair-mask tooltip, +/- field mapping, bounds, empty values and Enter/Escape callbacks under Lua 5.0 closure semantics')
