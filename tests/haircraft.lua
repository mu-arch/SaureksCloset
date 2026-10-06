dofile('tests/bags_list_ui.lua')
local V=VanityStudio
dofile('addon/SaureksCloset/Haircraft.lua')
local helpers={}
for i=1,80 do local name,value=debug.getupvalue(V.CreateUI,i);if not name then break end;helpers[name]=value end
local calls={};local result=1
SaureksClosetSetHaircraft=function(value) table.insert(calls,value);return result end
local c=VanityStudioCharacter;c.enabled=true;c.keepHairWithHat=nil
local invalidated=0;V.InvalidatePreviewModel=function() invalidated=invalidated+1 end
V.pagesByName.haircraft=CreateFrame('Frame',nil,V.frame)
V:CreateHaircraftPage(V.pagesByName.haircraft,helpers.label,helpers.button,helpers.enabled)
assert(#calls==0 and V.haircraftToggle:GetText()=='Toggle Hair')
local function buttonArt(state)
    for _,part in ipairs({'Left','Middle','Right'}) do
        assert(getglobal(V.haircraftToggle:GetName()..part).texture[1]=='Interface\\Buttons\\UI-Panel-Button-'..state)
    end
end
assert(V.haircraftToggle.enabled);buttonArt('Disabled')
this=V.haircraftToggle;this.scripts.OnMouseDown();buttonArt('Disabled');this.scripts.OnMouseUp();buttonArt('Disabled')
local guidance=V.haircraftStatus:GetText()
assert(guidance=="It's normal for hats to clip when getting started. Don't give up, adjust the position and fit. Most hats can be made to look okay.")
V:SetTab('haircraft')
assert(V.tab=='haircraft' and V.wardrobePage=='haircraft')
assert(V.pagesByName.armor:IsShown() and V.pagesByName.haircraft:IsShown())
assert(not V.pagesByName.bags:IsShown() and not V.pagesByName.body:IsShown())
assert(V.rotationControls:IsShown() and V.wardrobeSelectorLabel:GetText()=='Haircraft')
-- Reuse the Body camera/viewport for every race and sex, including both
-- buffers. Repeated close-up tab switches must not compound the zoom or lose
-- the original Outfit camera.
V.leftPaneFrameOverlay=CreateFrame("Frame",nil,V.frame)
for race=1,8 do for sex=0,1 do
    c.body={race=race,sex=sex}
    V:SetTab('armor')
    local scale=V.model:GetModelScale()
    local x,y,z=V.model:GetPosition()
    V:SetTab('body')
    local bx,by,bz=V.model:GetPosition()
    V:SetTab('haircraft')
    for _,model in ipairs({V.model,V.previewBuffer}) do
        local hx,hy,hz=model:GetPosition()
        assert(model:GetModelScale()==2.2 and hx==bx and hy==by and hz==bz)
        assert(model.width==213 and model.height==311)
        assert(model.anchor[4]==129 and model.anchor[5]==-75)
    end
    assert(V.bodyPreviewFade:IsShown() and V.leftPaneFrameOverlay:IsShown())
    V:SetTab('armor')
    local ax,ay,az=V.model:GetPosition()
    assert(V.model:GetModelScale()==scale and ax==x and ay==y and az==z)
    assert(not V.bodyPreviewFade:IsShown() and not V.model.closetBodyFramed)
end end
V:SetTab('haircraft')
-- Periodic fit synchronization keeps Haircraft consistent with the active
-- outfit, including its custom bags.
local originalApply=V.ApplyBagRenderer
local fitCalls={}
V.ApplyBagRenderer=function(_,token,weapons) fitCalls[token]=weapons;return true,1 end
local originalMulti=V.MultiBagRendererAvailable
V.MultiBagRendererAvailable=function() return true end
V.model.weaponToken=81;V.previewBuffer.weaponToken=82
assert(V:SyncLiveBagFits())
assert(fitCalls[0]==c.weapons and fitCalls[81]==c.weapons and fitCalls[82]==c.weapons)
V.ApplyBagRenderer=originalApply;V.MultiBagRendererAvailable=originalMulti
local baseline=invalidated
V.haircraftToggle.scripts.OnClick()
assert(c.keepHairWithHat and calls[#calls]==1 and invalidated==baseline+1)
assert(V.haircraftToggle:GetText()=='Toggle Hair');buttonArt('Up')
this=V.haircraftToggle;this.scripts.OnMouseDown();buttonArt('Down');this.scripts.OnMouseUp();buttonArt('Up')
for i=1,4 do V:RefreshHaircraftPage() end
assert(#calls==1,'Refreshing the page must not change appearance')
c.enabled=false;assert(V:SyncHaircraft() and calls[#calls]==0 and c.keepHairWithHat)
c.enabled=true;assert(V:SyncHaircraft() and calls[#calls]==1)
V.sharingWorldPaused=true;local count=#calls;assert(not V:SyncHaircraft() and #calls==count);V.sharingWorldPaused=nil
result=-1;assert(not V:SetHaircraft(false) and c.keepHairWithHat and invalidated==baseline+1)
result=1;assert(not V.haircraftDefault,"Haircraft must not have a Default button");V.haircraftToggle.scripts.OnClick()
assert(not c.keepHairWithHat and calls[#calls]==0 and invalidated==baseline+2);buttonArt('Disabled');assert(V.haircraftToggle.enabled and V.haircraftStatus:GetText()==guidance)
SaureksClosetSetHaircraft=nil;V:RefreshHaircraftPage()
assert(not V.haircraftToggle.enabled and not V.haircraftMask)
assert(not V:SetHaircraft(true) and not c.keepHairWithHat)
V:SetTab('bags');assert(not V.pagesByName.haircraft:IsShown() and not V.haircraftToggle:IsVisible())
print('PASS: Haircraft navigation, preview, toggles, defaults, persistence, disable, failed apply and old DLL gating')
