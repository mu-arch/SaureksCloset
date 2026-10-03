math.mod=math.mod or math.fmod;unpack=unpack or table.unpack
VanityStudioDB={};VanityStudioCharacter={enabled=true,selected={[1]=0,[3]=123},weapons={[108]=42}}
local calls,updates={},{}
VanityStudio={EffectiveWeapons=function(self,w)return w or {} end,WeaponStowMask=function()return 5 end}
GetCVar=function()return "login.example.test"end;GetRealmName=function()return "Realm"end;UnitName=function()return "Player"end;GetBuildInfo=function()return "1.12.1","5875"end
SaureksClosetConfigureSharing=function(...)table.insert(calls,{...});return 1 end
SaureksClosetUpdateSharing=function(...)table.insert(updates,{...});return 2 end
dofile('addon/SaureksCloset/Sharing.lua');local V=VanityStudio
V:InitializeSharing();assert(not VanityStudioDB.broadcastTransmog and not VanityStudioDB.receiveTransmog);assert(calls[1][1]==0 and calls[1][2]==0)
V:SetSharingOption('broadcastTransmog',true);assert(calls[2][1]==1 and calls[2][2]==0);assert(calls[2][5]=='login.example.test' and calls[2][8]=='1.12.1 / 5875');assert(updates[1][9]==0 and updates[1][11]==123 and updates[1][24]==42 and updates[1][28]==5)
V:SetSharingOption('receiveTransmog',true);assert(calls[3][1]==1 and calls[3][2]==1)
V:SetSharingOption('broadcastTransmog',false);assert(calls[4][1]==0 and calls[4][2]==1)
local before=#calls;V:UpdateSharing();assert(#calls==before,'unchanged options must not reconnect')
VanityStudioCharacter.body={race=6,sex=1,skin=0,face=0,hairStyle=0,hairColor=0,facial=0};V:UpdateSharing();assert(updates[#updates][1]==1 and updates[#updates][2]==6)
VanityStudioCharacter.enabled=false;V:SetSharingOption('broadcastTransmog',true);assert(calls[#calls][1]==0 and calls[#calls][2]==1);assert(updates[#updates][1]==0 and updates[#updates][9]==-1)
V:StopSharingSession();assert(calls[#calls][1]==0 and calls[#calls][2]==0 and V.sharingStatus==0)
local pausedUpdates=#updates;V:UpdateSharing(true);assert(#updates==pausedUpdates,'no snapshots while leaving the world')
V:SetSharingOption('receiveTransmog',true);assert(calls[#calls][1]==0 and calls[#calls][2]==0,'settings cannot restart sharing during zoning')
V.sharingWorldPaused=nil;V:UpdateSharing();assert(calls[#calls][2]==1 and #updates>pausedUpdates,'entering world resumes saved opt-in')
local configure=SaureksClosetConfigureSharing
SaureksClosetConfigureSharing=function(...)local args={...};if args[1]+args[2]>0 then return -3 end;return configure(...)end
V.sharingConfigured=nil;V:UpdateSharing();assert(V.sharingStatus==-3 and calls[#calls][1]==0 and calls[#calls][2]==0,'invalid new settings disconnect the old endpoint')
assert(not V:SetSharingOption('unrelated',true));SaureksClosetConfigureSharing=nil;V:UpdateSharing();assert(V.sharingStatus==-2)
print('PASS: explicit opt-in, independent send/receive, metadata, hide/inherit, body and weapon snapshots, disabled addon and missing DLL')
