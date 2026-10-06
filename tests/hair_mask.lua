dofile('tests/haircraft.lua')
local V=VanityStudio
local helpers={}
for i=1,80 do local name,value=debug.getupvalue(V.CreateUI,i);if not name then break end;helpers[name]=value end
SaureksClosetSetHaircraft=function() return 1 end
local generation,status,changed,calls=0,1,12,{}
SaureksClosetSetHairMask=function(...) table.insert(calls,arg);return status,changed,generation end
local c=VanityStudioCharacter
c.enabled=true;c.keepHairWithHat=true;c.selected={[1]=501};c.body={race=1,sex=1,hairStyle=3}
V:CreateHairMaskWindow(helpers.sheet,helpers.label,helpers.button,helpers.edit)
V:RefreshHaircraftPage();V:SetTab('haircraft');V:OpenHairMask()
assert(V.hairMaskWindow:IsShown() and V.haircraftMask.enabled and not V.haircraftDefault)
assert(V.hairMaskKey=='1:1:3:501' and V.hairMaskRows[1].editor:GetText()==90)
V.hairMaskToggle:SetChecked(true);V.hairMaskRows[1].editor:SetText('80');generation=1
assert(V:ApplyHairMask());assert(c.hairMasks['1:1:3:501'].width==80 and calls[#calls][1]==1)
V.hairMaskRows[3].editor:SetText('20');assert(not V:ApplyHairMask());assert(c.hairMasks['1:1:3:501'].top==95)
V.hairMaskRows[3].editor:SetText('nan');assert(not V:ApplyHairMask())
V:OpenHairMask();local saved=c.hairMasks[V.hairMaskKey]
status=-5;V.hairMaskRows[1].editor:SetText('70');assert(not V:ApplyHairMask())
assert(c.hairMasks[V.hairMaskKey]==saved and string.find(V.hairMaskMessage:GetText(),'Could not bake',1,true))
status=1
V:OpenHairMask();local old=c.hairMasks['1:1:3:501']
-- Switching head/body/style must discard unfinished edits, not save onto a new fit.
V.hairMaskRows[1].editor:SetText('60');c.selected[1]=502
assert(not V:ApplyHairMask() and c.hairMasks['1:1:3:501']==old and not c.hairMasks['1:1:3:502'])
assert(V.hairMaskKey=='1:1:3:502' and V.hairMaskRows[1].editor:GetText()==90)
c.selected[1]=501;V:SyncHairMask();assert(V.hairMaskRows[1].editor:GetText()==80)
c.body.hairStyle=4;V:SyncHairMask();assert(V.hairMaskRows[1].editor:GetText()==90)
c.body.hairStyle=3;V:SyncHairMask();assert(V.hairMaskRows[1].editor:GetText()==80)
-- Pending loads remain pending; generation changes invalidate the preview once.
local invalidations=0;V.InvalidatePreviewModel=function() invalidations=invalidations+1 end
status=0;assert(V:SyncHairMask());assert(string.find(V.hairMaskMessage:GetText(),'Waiting',1,true))
status=1;generation=2;V:SyncHairMask();V:SyncHairMask();assert(invalidations==1)
-- Turning off masking preserves original hairstyle and the saved fit values.
V.hairMaskToggle:SetChecked(false);assert(V:ApplyHairMask());assert(not c.hairMasks[V.hairMaskKey].enabled and calls[#calls][1]==0 and c.keepHairWithHat)
V.hairMaskToggle:SetChecked(true);assert(V:ApplyHairMask());c.enabled=false;V:SyncHairMask();assert(calls[#calls][1]==0)
c.enabled=true;c.keepHairWithHat=false;V:SyncHairMask();assert(calls[#calls][1]==0)
c.keepHairWithHat=true;c.selected[1]=0;V:SyncHairMask();assert(calls[#calls][1]==0 and not V.hairMaskKey and not V.hairMaskApply.enabled)
c.selected[1]=501;V:OpenHairMask();V:SetTab('armor');assert(not V.hairMaskWindow:IsShown())
SaureksClosetSetHairMask=nil;V:RefreshHaircraftPage();assert(not V.haircraftMask.enabled)
print('PASS: hair mask window, no Default button, per-hat/style persistence, numeric validation, stale editor isolation, pending loads, preview generations and disable/hidden/old DLL gating')
