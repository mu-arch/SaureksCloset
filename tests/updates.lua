local now,starts,enabled,remote=0,0,false,{1}
local messages={}
GetTime=function() return now end
VanityStudioDB={}
VanityStudio={VERSION="3.4.36",Message=function(self,value) table.insert(messages,value) end}
SaureksClosetRendererVersion=function() return VanityStudio.REQUIRED_RENDERER end
SaureksClosetSetUpdateChecks=function(value) enabled=value==1;return 1 end
SaureksClosetStartUpdateCheck=function() assert(enabled);starts=starts+1;return 1 end
SaureksClosetPollUpdateCheck=function() return unpack(remote) end
local title={SetText=function(self,value) self.text=value end,SetFont=function() end}
VanityStudio.frame={title=title}
dofile("addon/SaureksCloset/Updates.lua")
local V=VanityStudio
V:InitializeUpdates()
assert(VanityStudioDB.autoCheckUpdates==true and enabled and starts==0)
assert(not V.updateMismatch and table.getn(messages)==0)
now=3;V:UpdateUpdates();assert(starts==1 and V.updatePolling)
remote={2,3,4,37,V.REQUIRED_RENDERER+1,0};now=3.3;V:UpdateUpdates()
assert(V.remoteUpdateAvailable and title.text=="Saurek's Closet (Update Available)")
assert(table.getn(messages)==1)
V:CheckForUpdates(true);now=4;V:UpdateUpdates();assert(table.getn(messages)==1)
V:CheckForUpdates(true);local before=starts
V:SetAutoUpdates(false);remote={2,9,9,9,90909,0};now=50;V:UpdateUpdates()
assert(not enabled and not V.updatePolling and starts==before and V.remoteRelease.addon=="3.4.37")
assert(V:CheckForUpdates(true)==false and starts==before)
V:InitializeUpdates();now=100;V:UpdateUpdates();assert(not enabled and starts==before)
-- Local mismatch detection stays enabled even when networking is disabled.
SaureksClosetRendererVersion=function() return V.REQUIRED_RENDERER-1 end
V:CheckLocalRenderer();assert(V.updateMismatch and table.getn(messages)==2)
V:CheckLocalRenderer();assert(table.getn(messages)==2)
SaureksClosetRendererVersion=nil;V:CheckLocalRenderer();assert(V.updateMismatch and table.getn(messages)==3)
SaureksClosetRendererVersion=function() return V.REQUIRED_RENDERER end
V:CheckLocalRenderer();assert(not V.updateMismatch)
V:SetAutoUpdates(true);now=104;remote={2,3,4,15,30400,0};V:UpdateUpdates()
assert(not V.remoteUpdateAvailable and title.text=="Saurek's Closet")
assert(V:VersionIsNewer("3.4.10","3.4.9"))
assert(V:VersionIsNewer("3.4.100","3.4.99") and not V:VersionIsNewer("3.4.100","3.5.0"))
V:CheckForUpdates(true);remote={-1,0,0,0,0,12007};now=105;V:UpdateUpdates()
assert(string.find(V.updateStatus,"Could not",1,true) and table.getn(messages)==3)
V:CheckForUpdates(true);remote={2,3,4,"invalid",30433,0};now=106;V:UpdateUpdates()
assert(string.find(V.updateStatus,"Could not",1,true))
V:CheckForUpdates(true);remote={1};now=137;V:UpdateUpdates()
assert(not V.updatePolling and enabled and string.find(V.updateStatus,"timed out",1,true))
local opened
SaureksClosetOpenWebsite=function(page) opened=page;return 1 end
assert(V:OpenWebsite(2) and opened==2 and not V:OpenWebsite(9))
print("PASS: default-on/persisted-off checks, cancellation, mismatch/missing DLL alerts, numeric versions, deduplication, errors, timeout and website actions")
