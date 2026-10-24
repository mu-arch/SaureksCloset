dofile('tests/bags_list_ui.lua')
dofile('addon/SaureksCloset/Haircraft.lua')
local V=VanityStudio
local cog=V.secondHatCog
assert(cog and cog.parent==V.slotButtons[1] and cog.width==16 and cog.height==16)
assert(cog.anchor[1]=='BOTTOMRIGHT' and cog.anchor[2]==V.slotButtons[1] and cog.anchor[4]==-1 and cog.anchor[5]==1)
assert(cog:GetFrameLevel()==V.slotButtons[1]:GetFrameLevel()+3)
local opened
local open=V.OpenBrowser
V.OpenBrowser=function(_,slot) opened=slot end
this=cog;cog.scripts.OnClick();assert(opened==112)
V.OpenBrowser=open
SaureksClosetSecondHatVersion=function() return 1 end
SaureksClosetSetSecondHat=function() return 1 end
-- Exercise real OpenBrowser with the independent picker identity; the widget
-- shell stands in for the shared browser already covered by the UI tests.
V.browser=CreateFrame('Frame',nil,V.frame)
V.search={SetText=function() end}
V.CancelDraft=function() end;V.SetPanelArea=function() end;V.CloseWeaponOptions=function() end
V:OpenBrowser(112)
assert(V.slot==112 and V.browser:IsShown() and V.tab~='outfits')
SaureksClosetSetSecondHat=nil;V.browser:Hide();V.Message=function() end
V:OpenBrowser(112);assert(not V.browser:IsShown())
local f=assert(io.open('addon/SaureksCloset/UI.lua'));local source=f:read('*a');f:close()
assert(string.find(source,'self.removeSecondHatButton=button(f,"Remove second hat",24,405,154',1,true))
assert(string.find(source,'if self.slot==112 then self.removeSecondHatButton:Show()',1,true))
print('PASS: head-slot cog artwork/anchor/layer, second hat picker navigation, removal control and unsupported DLL gating')
