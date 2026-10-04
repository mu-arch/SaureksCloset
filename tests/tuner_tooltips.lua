-- Construct the real tuner and hover every field after construction. Simulate
-- Lua 5.0's exhausted generic-for upvalue when running under CI's Lua 5.1.
local function noop() end
local methods={SetScript=function(self,k,v) self.scripts[k]=v end,
    GetScript=function(self,k) return self.scripts[k] end,
    GetName=function(self) return self.name end,GetFrameLevel=function() return 1 end}
local function widget(name)
    return setmetatable({name=name or 'test',scripts={}}, {__index=function(_,key) return methods[key] or noop end})
end
methods.CreateTexture=function() return widget() end
methods.CreateFontString=methods.CreateTexture
CreateFrame=function(_,name) return widget(name) end
UIParent=widget();UISpecialFrames={};VanityStudio={slotNames={},frame=widget()}
local V=VanityStudio
GameTooltip={SetOwner=noop,ClearLines=function() GameTooltip.text='' end,
    AddLine=function(_,text) assert(type(text)=='string');GameTooltip.text=GameTooltip.text..text end,
    Show=noop,Hide=noop}
dofile('addon/SaureksCloset/BagTuner.lua')
dofile('addon/SaureksCloset/BagTunerUI.lua')
local function sheet() local f=widget();f.close=widget();return f end
local function make() return widget() end
V.bagModelChoices={};V.RefreshBagTunerUI=noop
V:CreateBagTunerUI(sheet,make,make,make,make,noop,make)
local function exhaustFieldClosure(callback)
    for i=1,20 do
        local name,value=debug.getupvalue(callback,i)
        if not name then break end
        if name=='text' and type(value)=='function' then
            for j=1,20 do
                local captured=debug.getupvalue(value,j)
                if not captured then break end
                if captured=='field' then debug.setupvalue(value,j,nil) end
            end
        end
    end
end
for _,slot in ipairs({1,101,108,109,110}) do
    V.placementTunerSlot=slot
    for _,row in ipairs(V.bagTunerWindow.rows) do
        for _,control in ipairs({row.editor,row.minus,row.plus,row.reset}) do
            this=control;local hover=assert(control:GetScript('OnEnter'))
            exhaustFieldClosure(hover);hover()
            assert(GameTooltip.text~='')
            if control~=row.reset then
                local expected=slot>=108 and row.editor.field.label or row.editor.field.help
                assert(string.find(GameTooltip.text,expected,1,true),'Wrong field tooltip')
                if slot>=108 then assert(string.find(GameTooltip.text,'normal position',1,true)) end
            end
            control:GetScript('OnLeave')()
        end
    end
end
print('PASS: all tuner field tooltips survive deferred Lua 5.0 callbacks for bags, decorations and equipped slots')
