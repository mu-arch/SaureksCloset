-- Pure gesture math plus actual mouse handlers, including cancellation paths.
math.mod=math.mod or math.fmod
local function copy(t) local o={};for k,v in pairs(t) do o[k]=type(v)=="table" and copy(v) or v end;return o end
local bag={id=1,mount="back"}
local state={available=true,enabled=true,key="bag:1",bag=201,race=1,sex=1,
    values={left=0,inset=0,up=-.03,pitch=0,roll=0,yaw=0,scale=85}}
local defaults=copy(state.values)
VanityStudioCharacter={enabled=true}
VanityStudio={PlacementChanged=function(self) self:SyncBagTuning() end,bagPlacementEditing=true,placementTunerBag=1,bagTunerDrafts={},
    Copy=function(self,t) return copy(t) end,
    BagInstance=function(self,id) return id==bag.id and bag or nil end,
    GetBagTunerState=function(self) state.values=self.bagTunerDrafts[state.key] or state.values;return state end,
    BagTunerDefaults=function() return copy(defaults) end,
    SyncBagTuning=function(self) self.syncs=(self.syncs or 0)+1 end,
    SetBagTunerPaused=function(self,v) self.bagTunerPaused=v;self:SyncBagTuning() end}
local V=VanityStudio
dofile("addon/SaureksCloset/BagPlacementEditor.lua")
assert(V:BeginBagPlacementDrag() and V.bagTunerPaused)
assert(V:MoveBagPlacement(10,-8,272,math.pi))
local moved=V.bagTunerDrafts[state.key]
assert(moved.up<defaults.up and moved.left<0 and moved.inset>0)
assert(moved.yaw>0 and moved.yaw<90 and moved.scale==85)
-- Auto-facing is the inward normal of the guide, not a fixed back-facing yaw.
local x=-.16+moved.inset;local y=moved.left
local normal=math.atan((-y/(.20*.20))/(-x/(.16*.16)))*180/math.pi
assert(math.abs(normal-moved.yaw)<.00001)
-- Mouse release restores the prior Pause setting; no per-frame save occurs.
V:EndBagPlacementDrag();assert(not V.bagTunerPaused and not V.bagPlacementGesture)
V.bagTunerPaused=true;assert(V:BeginBagPlacementDrag());V:EndBagPlacementDrag();assert(V.bagTunerPaused)
V.bagTunerPaused=false
-- High DPI is handled by the mouse layer, very large movement stays bounded.
for race=1,8 do for sex=0,1 do
    state.race=race;state.sex=sex;assert(V:BeginBagPlacementDrag())
    assert(V:MoveBagPlacement(100000,100000,272,math.pi/2))
    local value=V.bagTunerDrafts[state.key]
    for _,key in ipairs({"left","inset","up"}) do assert(value[key]>=-1 and value[key]<=1) end
    assert(value.yaw>=-180 and value.yaw<=180)
    assert(V:MoveBagPlacement(0,-100000,272,math.pi/2))
    assert(V.bagTunerDrafts[state.key].up==-3,"Dragging can reach the feet and still clamps below them")
    V:EndBagPlacementDrag()
end end
-- Replaced bag, mount, body or disabled wardrobe cannot inherit a gesture.
assert(V:BeginBagPlacementDrag());bag.mount="leftHip"
assert(not V:MoveBagPlacement(1,1,272,0) and not V.bagPlacementGesture and not V.bagTunerPaused)
assert(V:BeginBagPlacementDrag());state.key="other"
assert(not V:MoveBagPlacement(1,1,272,0) and not V.bagPlacementGesture)
assert(V:BeginBagPlacementDrag());VanityStudioCharacter.enabled=false
assert(not V:MoveBagPlacement(1,1,272,0) and not V:BeginBagPlacementDrag())
VanityStudioCharacter.enabled=true;assert(V:BeginBagPlacementDrag())
assert(not V:MoveBagPlacement(0/0,0,272,0));V:EndBagPlacementDrag()
-- The placement window borrows and restores physical frames, even when an
-- asynchronous preview load swaps the visible/buffer references while open.
local methods={}
local function widget(parent) return setmetatable({parent=parent,points={{"TOPLEFT",parent,"TOPLEFT",61,-86}},width=244,height=340,level=3,scripts={}}, {__index=methods}) end
function methods:GetParent() return self.parent end
function methods:SetParent(p) self.parent=p end
function methods:GetWidth() return self.width end
function methods:GetHeight() return self.height end
function methods:SetWidth(v) self.width=v end
function methods:SetHeight(v) self.height=v end
function methods:GetFrameLevel() return self.level end
function methods:SetFrameLevel(v) self.level=v end
function methods:GetNumPoints() return table.getn(self.points) end
function methods:GetPoint(i) return unpack(self.points[i]) end
function methods:SetPoint(...) table.insert(self.points,arg) end
function methods:ClearAllPoints() self.points={} end
function methods:SetAllPoints(w) self.points={{"TOPLEFT",w,"TOPLEFT",0,0},{"BOTTOMRIGHT",w,"BOTTOMRIGHT",0,0}} end
function methods:SetRotation(v) self.rotation=v end
function methods:SetScript(name,f) self.scripts[name]=f end
function methods:Hide() if self.shown then self.shown=false;if self.scripts.OnHide then self.scripts.OnHide() end end end
function methods:Show() self.shown=true end
function methods:IsShown() return self.shown end
function methods:GetName() return "test" end
for _,name in ipairs({"SetFrameStrata","SetClampedToScreen","SetFont","SetJustifyH"}) do methods[name]=function() end end
local function sheet(_,parent) local f=widget(parent);f.close=widget(f);return f end
local function label(parent) return widget(parent) end
UISpecialFrames={}
V.frame=widget();local oldParent=widget(V.frame)
V.model=widget(oldParent);V.previewBuffer=widget(oldParent);V.previewDragSurface=widget(oldParent)
V.previewNote=widget(oldParent);local oldNote=V.previewNote;local first=V.model
V.HidePreviewUntilReady=function() end;V.InvalidatePreviewModel=function() end;V.RefreshPreview=function() end
V:CreateBagPlacementEditor(sheet,label,label)
assert(V:OpenBagPlacementEditor() and V.model.parent==V.bagPlacementWindow and V.previewNote~=oldNote)
assert(V:OpenBagPlacementEditor()) -- Idempotent: do not overwrite restore data.
V.model,V.previewBuffer=V.previewBuffer,V.model
assert(V:BeginBagPlacementDrag());V:CloseBagPlacementEditor()
assert(not V.bagPlacementEditing and not V.bagPlacementGesture and not V.bagTunerPaused)
for _,w in ipairs({V.model,V.previewBuffer,V.previewDragSurface}) do
    assert(w.parent==oldParent and w.width==244 and w.height==340 and w.level==3)
    assert(w.points[1][4]==61 and w.points[1][5]==-86)
end
assert(V.previewBuffer==first and V.previewNote==oldNote)
print("PASS: surface drag, inward orientation, all bodies, bounded offsets, pause restoration and stale target cancellation")
