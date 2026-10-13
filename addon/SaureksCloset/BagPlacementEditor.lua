-- A surface-guided placement gesture. The usual numeric fit remains editable.
local V=VanityStudio
local function clamp(v,a,b) return math.max(a,math.min(b,v)) end
-- Half-view heights from the native full-body cameras (male/female).
local heights={{2.145804,1.990295},{2.117404,2.123710},{1.626445,1.616408},{2.503745,2.276414},
    {1.980775,1.858947},{1.967832,2.107813},{1.047558,1.002644},{2.148758,2.329100}}
-- Torso proxies, not armor/weapon collision meshes. X is depth, Y is width.
local radii={{{.22,.26},{.16,.20}},{{.30,.34},{.19,.26}},{{.25,.30},{.25,.28}},{{.23,.26},{.19,.22}},
    {{.29,.24},{.13,.19}},{{.46,.47},{.33,.37}},{{.16,.20},{.19,.21}},{{.26,.25},{.19,.22}}}
local function geometry(state,mount)
    local size=(radii[state.race] or radii[1])[state.sex+1]
    local x=mount=="back" and -size[1] or 0
    local y=mount=="leftHip" and size[2] or mount=="rightHip" and -size[2] or 0
    return size[1],size[2],x,y
end
local function angle(x,y)
    if math.atan2 then return math.atan2(y,x) end
    if x>0 then return math.atan(y/x) end
    if x<0 then return math.atan(y/x)+(y>=0 and math.pi or -math.pi) end
    return y>0 and math.pi/2 or y<0 and -math.pi/2 or 0
end
function V:BagPlacementView()
    local bag=self.placementTunerBag and self:BagInstance(self.placementTunerBag)
    local rotation=bag and bag.mount=="leftHip" and -math.pi/2 or bag and bag.mount=="rightHip" and math.pi/2 or math.pi
    for _,model in ipairs({self.model,self.previewBuffer}) do model.rotation=rotation;model:SetRotation(rotation) end
end
function V:BeginBagPlacementDrag()
    if not self.bagPlacementEditing then return false end
    local state=self:GetBagTunerState()
    if not state.available or not state.enabled or not VanityStudioCharacter.enabled or not self.placementTunerBag then return false end
    local bag=self:BagInstance(self.placementTunerBag)
    if not bag then return false end
    self.bagPlacementGesture={key=state.key,bag=bag.id,mount=bag.mount,paused=self.bagTunerPaused}
    self:SetBagTunerPaused(true)
    return true
end
function V:EndBagPlacementDrag()
    local gesture=self.bagPlacementGesture
    self.bagPlacementGesture=nil
    if gesture then self:SetBagTunerPaused(gesture.paused) end
end
function V:MoveBagPlacement(dx,dy,height,rotation)
    local gesture=self.bagPlacementGesture
    if not gesture then return false end
    local state=self:GetBagTunerState()
    local bag=self:BagInstance(gesture.bag)
    if not state.available or state.key~=gesture.key or not bag or bag.mount~=gesture.mount or not state.enabled or not VanityStudioCharacter.enabled then
        self:EndBagPlacementDrag();return false
    end
    if type(dx)~="number" or type(dy)~="number" or dx-dx~=0 or dy-dy~=0 or not height or height<=0 then return false end
    local defaults=self:BagTunerDefaults(state.bag,state.race,state.sex)
    if not defaults then return false end
    local values=self:Copy(state.values)
    local units=2*(heights[state.race] or heights[1])[state.sex+1]/(height*1.30)
    values.up=clamp(values.up+dy*units,-3,1)
    if dx~=0 then
        local rx,ry,bx,by=geometry(state,bag.mount)
        local x=bx+values.inset-defaults.inset;local y=by+values.left-defaults.left
        local sx,sy=math.sin(rotation),math.cos(rotation)
        local span=math.sqrt((sx*rx)^2+(sy*ry)^2)
        local u=clamp((sx*x+sy*y+dx*units)/span,-.985,.985)
        local ex,ey=sx*rx/span,sy*ry/span
        local nx,ny=-ey,ex
        -- Choose the visible half of the ellipse, in the current preview view.
        if nx*math.cos(rotation)*rx-ny*math.sin(rotation)*ry<0 then nx=-nx;ny=-ny end
        local depth=math.sqrt(1-u*u)
        x=rx*(ex*u+nx*depth);y=ry*(ey*u+ny*depth)
        values.inset=clamp(defaults.inset+x-bx,-1,1)
        values.left=clamp(defaults.left+y-by,-1,1)
        values.yaw=angle(-x/(rx*rx),-y/(ry*ry))*180/math.pi
        values.pitch=defaults.pitch;values.roll=0
    end
    self.bagTunerDrafts[state.key]=values
    self:PlacementChanged()
    return true
end
function V:CloseBagPlacementEditor()
    if self.bagPlacementWindow then self.bagPlacementWindow:Hide() end
end
function V:OpenBagPlacementEditor()
    local state=self:GetBagTunerState()
    if not state.available or not state.enabled or not VanityStudioCharacter.enabled or not self.placementTunerBag then return false,"Enable live tuning and select a bag first." end
    if not self.bagPlacementWindow or not self.model then return false,"The preview is not ready." end
    local f=self.bagPlacementWindow
    if f:IsShown() then return true end
    -- Borrow the wardrobe's independently owned double-buffered preview. It
    -- already knows how to dress equipment and recover after a body rebuild.
    f.borrowed={}
    for _,widget in ipairs({self.model,self.previewBuffer,self.previewDragSurface}) do
        local saved={widget=widget,parent=widget:GetParent(),level=widget:GetFrameLevel(),width=widget:GetWidth(),height=widget:GetHeight(),points={}}
        for i=1,widget:GetNumPoints() do table.insert(saved.points,{widget:GetPoint(i)}) end
        table.insert(f.borrowed,saved);widget:SetParent(f);widget:ClearAllPoints()
        widget:SetFrameLevel(f:GetFrameLevel()+(widget==self.previewDragSurface and 3 or 2))
    end
    for _,model in ipairs({self.model,self.previewBuffer}) do
        model:SetPoint("TOPLEFT",f,"TOPLEFT",38,-112);model:SetWidth(288);model:SetHeight(272)
    end
    self.previewDragSurface:SetAllPoints(self.model)
    f.originalNote=self.previewNote;self.previewNote=f.note
    self.bagPlacementEditing=true;f:Show();self:BagPlacementView()
    self:HidePreviewUntilReady();self:InvalidatePreviewModel(0,true);self:RefreshPreview()
    return true
end
function V:CreateBagPlacementEditor(sheet,label,settingsButton)
    local f=sheet("SaureksClosetBagPlacement",self.frame,"Place Bag")
    self.bagPlacementWindow=f;f:Hide();f:SetFrameStrata("DIALOG");f:SetClampedToScreen(true)
    f:SetPoint("TOPLEFT",self.frame,"TOPLEFT",0,0)
    local hint=label(f,"Left-drag to place. Right-drag to turn.",31,78,300,20,true)
    hint:SetFont("Fonts\\FRIZQT__.TTF",11);hint:SetJustifyH("CENTER")
    local help=label(f,"Fine-tune the fit with the controls alongside.",31,98,300,16,true)
    help:SetFont("Fonts\\FRIZQT__.TTF",10);help:SetJustifyH("CENTER")
    f.note=label(f,"",38,370,288,20,true);f.note:SetJustifyH("CENTER")
    settingsButton(f,"Reset view",31,392,145,function() V:BagPlacementView() end,.75)
    settingsButton(f,"Done",186,392,145,function() V:CloseBagPlacementEditor() end,.75)
    f.close:SetScript("OnClick",function() V:CloseBagPlacementEditor() end)
    f:SetScript("OnHide",function()
        V:EndBagPlacementDrag();V.bagPlacementEditing=nil
        for _,saved in ipairs(f.borrowed or {}) do
            local widget=saved.widget;widget:SetParent(saved.parent);widget:ClearAllPoints()
            widget:SetFrameLevel(saved.level)
            widget:SetWidth(saved.width);widget:SetHeight(saved.height)
            for _,point in ipairs(saved.points) do widget:SetPoint(unpack(point)) end
        end
        if f.originalNote then V.previewNote=f.originalNote;f.originalNote=nil end
        f.borrowed=nil
    end)
    table.insert(UISpecialFrames,f:GetName())
end
