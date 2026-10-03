-- Reuse the actual Settings fixture, then create the page with sharing loaded.
dofile('tests/settings_donations.lua')
VanityStudioCharacter={enabled=true,selected={},weapons={}}
dofile('addon/SaureksCloset/Sharing.lua')
local V=VanityStudio
V:CreateSettingsPage(CreateFrame('Frame',nil,V.frame))
assert(V.broadcastTransmogCheckbox and V.receiveTransmogCheckbox)
assert(not V.broadcastTransmogCheckbox.checked and not V.receiveTransmogCheckbox.checked)
assert(V.sharingConnectionButton.anchor[5]+V.sharingConnectionButton.height<0)
assert(-V.sharingConnectionButton.anchor[5]+V.sharingConnectionButton.height<=437,'sharing controls must fit inside the sheet trim without scrolling')
V:OpenSharingConnection();assert(V.sharingConnectionWindow:IsShown());assert(V.sharingEndpointEdit:GetText()=='')
print('PASS: sharing controls, independent defaults, separate connection form, no scrolling or bottom-trim overflow')
