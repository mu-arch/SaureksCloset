-- Built-in static portraits: no models, capture workers, cache files or DLL calls.
local V=VanityStudio
local races={[1]="Human",[2]="Orc",[3]="Dwarf",[4]="NightElf",[5]="Scourge",[6]="Tauren",[7]="Gnome",[8]="Troll"}
local nativeRaces={Human="Human",Orc="Orc",Dwarf="Dwarf",NightElf="NightElf",Scourge="Scourge",Undead="Scourge",Tauren="Tauren",Gnome="Gnome",Troll="Troll"}
local prefix="Interface\\CharacterFrame\\TemporaryPortrait"
function V:OutfitPortraitTexture(look)
    local race,sex
    if look.body then
        race=races[look.body.race];sex=look.body.sex
    else
        local _,nativeRace=UnitRace("player")
        race=nativeRaces[nativeRace]
        local nativeSex=UnitSex("player")
        if nativeSex==2 then sex=0 elseif nativeSex==3 then sex=1 end
    end
    if not race or (sex~=0 and sex~=1) then return prefix end
    return prefix.."-"..(sex==0 and "Male" or "Female").."-"..race
end
function V:BindOutfitPortrait(row,look)
    row.portraitImage:SetTexture(self:OutfitPortraitTexture(look))
end
