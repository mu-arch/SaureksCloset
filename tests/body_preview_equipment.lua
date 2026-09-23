-- Exercise the real dressing and native-weapon paths without changing the
-- player's selections or the separate saved-look preview.
local addon=arg[1] or "addon/SaureksCloset/"
local now,nativeCalls,itemRequests=1,{},{}
local availableItems={}
local equipment={[1]=401,[4]=402,[5]=403,[16]=101,[17]=102,[18]=103,[20]=104}
GetTime=function() return now end
GetInventoryItemLink=function(unit,slot)
    assert(unit=="player")
    return equipment[slot] and "item:"..equipment[slot]..":0:0:0" or nil
end
GetItemInfo=function(id) return availableItems[id] and "Cached item" or nil end
SaureksClosetRendererVersion=function() return 30711 end
SaureksClosetSetWeapons=function(...)
    table.insert(nativeCalls,{...});return 1
end
SaureksClosetBeginPreview=function() return 1 end
local nextToken=30
SaureksClosetEndPreview=function() nextToken=nextToken+1;return nextToken end
SaureksClosetPreviewStatus=function() return 1 end
SaureksClosetWeaponAssets={
    [101]={1,7},[102]={3,6},[103]={4,19},[104]={5,0},
    [201]={1,7},[202]={3,6},[203]={4,3},[204]={5,0},
}
VanityStudioCharacter={enabled=true,selected={[1]=501,[4]=502,[5]=503},
    body={race=1,sex=1,skin=0,face=0,hairStyle=0,hairColor=0,facial=0},
    weapons={independent=true,carriedEnabled=true,backBag=1,quiverHorizontal=true,
        [101]=201,[105]=202,[106]=203,[107]=204,[108]=201,[109]=202,[110]=203}}
VanityStudio={tab="body",slotNames={},slotOrder={1,3,15,4,5,19,9,10,6,7,8,16,17,18},
    bodyKeys={"skin","face","hairStyle","hairColor","facial"},previewRequests={}}
local V=VanityStudio
function V:Copy(source) local result={};for k,v in pairs(source) do result[k]=v end;return result end
function V:NativeBody() return VanityStudioCharacter.body end
function V:FrameBodyPreview() end
function V:RestoreBodyPreview() end
function V:RefreshPortraits() end
function V:UpdateOutfitPreview() end
function V:RequestItem(id) table.insert(itemRequests,id) end
local function model(token)
    return {weaponToken=token,rotation=.61,alpha=1,tried={},undresses=0,
        IsVisible=function() return true end,
        SetAlpha=function(self,value) self.alpha=value end,
        SetRotation=function(self,value) self.rotation=value end,
        SetUnit=function() end,
        SetSequence=function(self,value) self.sequence=value end,
        Undress=function(self) self.tried={};self.undresses=self.undresses+1 end,
        TryOn=function(self,id) table.insert(self.tried,tonumber(id)) end}
end
V.model=model(11);V.previewBuffer=model(12)
V.previewNote={SetText=function(self,text) self.text=text end}
dofile(addon.."Weaponry.lua")
dofile(addon.."Preview.lua")
local function equalTable(actual,expected,label)
    for k,v in pairs(expected) do assert(actual[k]==v,label..": unexpected "..tostring(k)) end
    for k,v in pairs(actual) do assert(expected[k]==v,label..": extra "..tostring(k)) end
end
local selected=V:Copy(VanityStudioCharacter.selected)
local weapons=V:Copy(VanityStudioCharacter.weapons)
local body=V:Copy(VanityStudioCharacter.body)
local function assertSelectionsUnchanged()
    equalTable(VanityStudioCharacter.selected,selected,"saved armor")
    equalTable(VanityStudioCharacter.weapons,weapons,"saved weapons")
    equalTable(VanityStudioCharacter.body,body,"saved body")
    assert(VanityStudioCharacter.enabled,"body preview disabled the world appearance")
end
local function expectUndressedNative(call,token)
    assert(call[1]==token,"wrong preview token")
    for i=2,8 do assert(call[i]==0,"carried weapon leaked into Body preview") end
    -- Keep identities so the renderer can recognize and hide stock children.
    assert(call[9]==101 and call[10]==102 and call[11]==103)
    assert(call[15]==0 and call[16]==0,"quiver or bag leaked into Body preview")
    for i=17,19 do assert(call[i]==0,"attack appearance leaked into Body preview") end
    assert(call[20]==1 and call[21]==0 and call[22]==1,
        "Body needs independent, sheathed, empty carried display to hide stock weapons")
end
local function expectNormalNative(call,token)
    assert(call[1]==token)
    assert(call[2]==201 and call[6]==202 and call[7]==203 and call[8]==204)
    assert(call[9]==101 and call[10]==102 and call[11]==103)
    assert(call[15]==104 and call[16]==1)
    assert(call[17]==201 and call[18]==202 and call[19]==203)
end
local function contains(values,wanted)
    for _,value in ipairs(values) do if value==wanted then return true end end
    return false
end

-- Equipment and temporary browser drafts cannot obscure the Body controls.
V.draft={slot=5,id=999};V.weaponPreviewMode=2
local items=V:PreviewItems()
for _,slot in ipairs(V.slotOrder) do
    assert(items[slot]==(slot==4 and 6096 or 0),"Body dressed equipment slot "..slot)
end
assert(V:WeaponPreviewMode()==0,"Body inherited a ranged attack pose")
V.draft={slot=110,id=203}
assert(V:WeaponPreviewMode()==0,"Body inherited a draft attack pose")
for _,token in ipairs({11,12}) do
    assert(V:ApplyWeaponRenderer(token,weapons));expectUndressedNative(nativeCalls[#nativeCalls],token)
end
-- The same Body page must never undress the world or a saved-look preview.
for _,token in ipairs({0,99}) do
    assert(V:ApplyWeaponRenderer(token,weapons));expectNormalNative(nativeCalls[#nativeCalls],token)
end
assertSelectionsUnchanged()
V.draft=nil
V.tab="armor"
items=V:PreviewItems()
assert(items[1]==501 and items[4]==502 and items[5]==503)
assert(items[16]==101 and items[17]==102 and items[18]==103)
VanityStudioCharacter.enabled=false
items=V:PreviewItems();assert(items[1]==401 and items[4]==402 and items[5]==403)
V.tab="body"
items=V:PreviewItems();assert(items[4]==6096 and items[1]==0 and items[16]==0)
VanityStudioCharacter.enabled=true

-- Run cloning, dressing, and readiness on separate frames, as in the client.
V:RefreshPreview()
assert(V.previewDressingModel==V.previewBuffer)
now=now+.1;V:RefreshPreview()
local dressed=V.previewDressingModel
assert(dressed.undresses==1 and #dressed.tried==1 and dressed.tried[1]==6096)
expectUndressedNative(nativeCalls[#nativeCalls],dressed.weaponToken)
assert(V.previewWaiting[6096] and itemRequests[1]==6096)
now=now+.1;V:RefreshPreview()
assert(V.model==dressed and not V.previewReveal and V.model.alpha==1)
assert(V.model.sequence==0,"Body was posed holding a weapon")
local bodySignature=V.previewDressSignature
local undresses=V.model.undresses
availableItems[6096]=true
V:UpdatePreviewLoading()
assert(not V.previewWaiting[6096] and V.model.undresses==undresses)
for _,id in ipairs(V.model.tried) do assert(id==6096,"late cache response dressed equipment on Body") end

-- Tab changes must redress even though race/sex and the clone stay unchanged.
local token=V.model.weaponToken
V.tab="armor"
now=now+.1;V:RefreshPreview()
assert(V.model.undresses==undresses+1 and V.model.weaponToken==token)
assert(contains(V.model.tried,501) and contains(V.model.tried,502) and contains(V.model.tried,503))
assert(not contains(V.model.tried,6096),"Body shirt leaked into armor preview")
expectNormalNative(nativeCalls[#nativeCalls],token)
now=now+.1;V:RefreshPreview()
assert(V.previewDressSignature~=bodySignature)
assertSelectionsUnchanged()

-- Even identical item lists need distinct signatures to toggle native hiding.
for _,slot in ipairs(V.slotOrder) do VanityStudioCharacter.selected[slot]=slot==4 and 6096 or 0 end
VanityStudioCharacter.weapons={}
V.weaponPreviewMode=0
now=now+.1;V:RefreshPreview();now=now+.1;V:RefreshPreview()
local armorSignature=V.previewDressSignature
undresses=V.model.undresses
V.tab="body"
now=now+.1;V:RefreshPreview();now=now+.1;V:RefreshPreview()
assert(V.previewDressSignature~=armorSignature,"Body and normal preview shared a dressing signature")
assert(V.model.undresses==undresses+1)
expectUndressedNative(nativeCalls[#nativeCalls],V.model.weaponToken)
print("Body preview: shirt-only dressing, weapon hiding, late cache, tab restoration, and world/saved-look isolation passed")
