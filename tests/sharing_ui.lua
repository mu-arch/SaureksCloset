-- Reuse the actual Settings fixture, then create the page with sharing loaded.
dofile('tests/settings_donations.lua')
VanityStudioCharacter={enabled=true,selected={},weapons={}}
dofile('addon/SaureksCloset/Sharing.lua')
local V=VanityStudio
V:CreateSettingsPage(CreateFrame('Frame',nil,V.frame))
assert(not V.broadcastTransmogCheckbox and not V.receiveTransmogCheckbox)
assert(not V.sharingConnectionButton and not V.sharingConnectionWindow and not V.sharingStatusLabel)
V:OpenSharingConnection()
assert(V.autoUpdatesCheckbox,'hiding sharing must preserve update/privacy controls')
-- Keep coverage of the dormant UI for when the feature is enabled for launch.
V.SHARING_RELEASE_ENABLED=true
V:CreateSettingsPage(CreateFrame('Frame',nil,V.frame))
assert(V.broadcastTransmogCheckbox and V.receiveTransmogCheckbox)
assert(not V.broadcastTransmogCheckbox.checked and not V.receiveTransmogCheckbox.checked)
assert(V.sharingConnectionButton.anchor[5]+V.sharingConnectionButton.height<0)
assert(-V.sharingConnectionButton.anchor[5]+V.sharingConnectionButton.height<=437,'sharing controls must fit inside the sheet trim without scrolling')
V:OpenSharingConnection();assert(V.sharingConnectionWindow:IsShown());assert(V.sharingEndpointEdit:GetText()=='')
print('PASS: sharing UI absent before launch; dormant sharing controls, independent defaults, separate connection form, no scrolling or bottom-trim overflow')
