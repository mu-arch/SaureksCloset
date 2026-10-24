dofile('tests/bags_list_ui.lua')
dofile('addon/SaureksCloset/Haircraft.lua')
local V=VanityStudio
local cog=V.secondHatCog
assert(cog and cog.parent==V.slotButtons[1] and cog.width==16 and cog.height==16)
assert(cog.anchor[1]=='BOTTOMRIGHT' and cog.anchor[2]==V.slotButtons[1] and cog.anchor[4]==4 and cog.anchor[5]==-3)
assert(cog:GetFrameLevel()==V.slotButtons[1]:GetFrameLevel()+3)
-- Compare to the actual weapon cog, rather than hardcoding a separate style.
local weapon=V.weaponCards[108].gear
assert(cog.caption and cog.caption:GetText()=='2' and not weapon.caption)
assert(cog.normal=='Interface\\AddOns\\SaureksCloset\\Textures\\CornerButton.tga')
assert(weapon.normal=='Interface\\AddOns\\SaureksCloset\\Textures\\PlacementCog.tga')
assert(weapon.width==cog.width and weapon.height==cog.height)
assert(weapon.anchor[1]==cog.anchor[1] and weapon.anchor[3]==cog.anchor[3])
assert(weapon.anchor[4]==cog.anchor[4] and weapon.anchor[5]==cog.anchor[5])
cog:Disable();cog:SetAlpha(.4)
V:RefreshSecondHatCog();assert(cog.enabled and cog.alpha==1)
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
-- A missing version getter must not block a working setter.
SaureksClosetSecondHatVersion=nil;assert(V:SecondHatAvailable())
SaureksClosetSetSecondHat=nil;V.browser:Hide();local message
V.Message=function(_,text) message=text end
V:RefreshSecondHatCog();assert(cog.enabled and cog.alpha==1)
V:OpenBrowser(112);assert(V.browser:IsShown() and string.find(message,'fully close and reopen WoW',1,true))
-- Native-dependent Apply/Remove are guarded while browsing remains available.
local function control() return CreateFrame('Button',nil,V.browser) end
V.uiReady=true;V.rows={};V.visibleItemRows=9;V.offset=0
V.hideHigherLevelCheckbox=control();V.scrollUp=control();V.scrollDown=control()
V.scroll=control();V.scroll.SetMinMaxValues=function() end;V.scroll.SetValue=function() end;V.scrollThumb=control()
V.qualityLabel=control();V.materialLabel=control();V.resultsLabel=control();V.emptyLabel=control()
V.browser.title=control();V.commitButton=control();V.removeSecondHatButton=control()
V.Filter=function() return {} end;V.ItemTypeName=function() return 'All' end;V.qualityNames={}
UnitLevel=function() return 60 end
V.draft={slot=112,id=123};VanityStudioCharacter.secondHat=123
V:RefreshList()
assert(V.browser.title:GetText()=='Second hat - Restart required')
assert(not V.commitButton.enabled and not V.removeSecondHatButton.enabled and V.removeSecondHatButton:IsShown())
SaureksClosetSetSecondHat=function() return 1 end;V:RefreshList()
assert(V.browser.title:GetText()=='Second hat Appearance' and V.commitButton.enabled and V.removeSecondHatButton.enabled)
V.slot=1;V.slotNames[1]='Head';V:RefreshList()
assert(V.commitButton.enabled and not V.removeSecondHatButton:IsShown())
local f=assert(io.open('addon/SaureksCloset/UI.lua'));local source=f:read('*a');f:close()
assert(string.find(source,'self.removeSecondHatButton=button(f,"Remove second hat",24,405,154',1,true))
assert(string.find(source,'if self.slot==112 then self.removeSecondHatButton:Show()',1,true))
print('PASS: numbered second-hat corner button, unchanged weapon cogs, clickable missing-DLL navigation, setter-only capability, clear restart notice, guarded Apply/Remove and normal browser isolation')
