-- Reuse the real main-window construction fixture, then exercise the shared
-- builder with template anchors that SetPoint does not silently replace.
dofile('tests/weapon_full_page.lua')
local V=VanityStudio
local sheet
for i=1,40 do
    local name,value=debug.getupvalue(V.CreateUI,i)
    if name=='sheet' then sheet=value;break end
end
assert(sheet,'Main window must use the shared sheet builder')
local originalCreateFrame=CreateFrame
function CreateFrame(kind,name,parent,template)
    local frame=originalCreateFrame(kind,name,parent,template)
    if template=='UIPanelCloseButton' then
        frame:SetWidth(32);frame:SetHeight(32)
        local setPoint,clear=frame.SetPoint,frame.ClearAllPoints
        frame.points={TOPRIGHT={'TOPRIGHT',parent,'TOPRIGHT',0,0}}
        function frame:SetPoint(point,...)
            self.points[point]={point,...};setPoint(self,unpack(self.points[point]))
        end
        function frame:ClearAllPoints() self.points={};clear(self) end
    end
    return frame
end
local function check(frame)
    local close=frame.close;local count=0
    for _ in pairs(close.points) do count=count+1 end
    assert(count==1,frame.name..': inherited anchors must be cleared')
    local a=close.points.CENTER
    assert(a and a[2]==frame and a[3]=='TOPRIGHT',frame.name..': close button must use its own sheet')
    -- Existing corrected main-window center is x=338, y=24 on the 384px art.
    assert(frame.width+a[4]==338 and -a[5]==24,frame.name..': close button moved off the artwork')
    assert(close.width==32 and close.height==32,'Native close artwork must keep its size')
    return a[4],a[5]
end
V:CreateUI()
local x,y=check(V.frame)
for _,name in ipairs({'Browser','OutfitDetails','Information','BagTuner','BagPlacement','FutureSecondary'}) do
    local frame=sheet('CloseTest'..name,V.frame,name)
    local sx,sy=check(frame)
    assert(sx==x and sy==y,name..': secondary close differs from the main window')
end
print('PASS: main and secondary close buttons share artwork alignment and exactly one anchor')
