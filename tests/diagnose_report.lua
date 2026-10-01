-- Empty inventory slots in the supported client may return zero Lua values.
VanityStudio={VERSION="3.7.11",slotOrder={1,5},applied={},errors={}}
VanityStudioCharacter={enabled=true,selected={[5]=123},managed={}}
VanityStudioDB={}
function VanityStudio:BodyAvailable() return false end
function VanityStudio:Available() return true end
function VanityStudio:Message(message) self.lastMessage=message end
function GetBuildInfo() return "1.12.1","5875" end
function GetInventoryItemLink(_,slot)
    if slot==5 then return "item:123:0:0:0" end
end
local saved
function WriteFile(name,mode,report)
    assert(name=="SaureksCloset-diagnostics.txt" and mode=="w")
    saved=report
    return true
end
dofile("addon/SaureksCloset/Body.lua")
VanityStudio.armorHistory={"recent armor repair"}
VanityStudio:Diagnose()
assert(saved==VanityStudioDB.diagnostics)
assert(saved:find("Slot 1: selected=nil; applied=nil; managed=nil; equipped=nil; error=nil",1,true))
assert(saved:find("Slot 5: selected=123; applied=nil; managed=nil; equipped=item:123:0:0:0; error=nil",1,true))
assert(saved:find("recent armor repair",1,true))
assert(VanityStudio.lastMessage:find("SaureksCloset-diagnostics.txt",1,true))
WriteFile=function() return false end
VanityStudio:Diagnose()
assert(VanityStudio.lastMessage:find("VanityStudioDB",1,true))
print("Diagnostic report handles empty slots and records armor history")
-- Include the actual compositor state, not just Lua's applied-item cache.
function VanityStudio:ArmorVisualSelection(slot) if slot==5 then return 888 end end
function SaureksClosetInspectArmor(slot,expected)
    assert(slot==5 and expected==888)
    return 0,999,0,6
end
WriteFile=function(_,_,report) saved=report;return true end
VanityStudio:Diagnose()
assert(saved:find("Armor visual inspection: true",1,true))
assert(saved:find("Armor visual 5: expected=888; ok=true; status=0; rendered=999; dirty=0; attachments=6",1,true))
print("Diagnostic report records actual rendered armor mismatches")
-- A visible authored cape and an uninitialized simulation distinguish a draw
-- mapping failure from a cape that is actually hidden. No pointers are logged.
function SaureksClosetInspectCapePhysics() return 1,1,1,1,1,1,2,0,0,0,1,7,0,0 end
VanityStudio:Diagnose()
assert(saved:find("visible cape sections,simulation ready",1,true))
assert(saved:find("Cape inspection: true,1,1,1,1,1,1,2,0,0,0,1,7,0,0",1,true))
SaureksClosetInspectCapePhysics=function() error("test unavailable") end
VanityStudio:Diagnose()
assert(saved:find("Cape inspection: false,",1,true))
SaureksClosetInspectCapePhysics=nil
VanityStudio:Diagnose()
assert(not saved:find("Cape inspection:",1,true))
print("Diagnostic report distinguishes cape draw failures and supports older renderers")
