ORIGINAL DOWNLOAD: https://www.moddb.com/games/freelancer/addons/compact-factions-display


INSTALLATION:

Check if you've already installed HudShift.
If not, then install:
	Copy HudShift.ini to "DATA\INTERFACE\"
	Copy HudShift.dll to "EXE\"
	Open "EXE\dacom.ini" and add HudShift.dll under [Libraries] section. Save changes.

1) Copy FactionList.dll to "EXE\"

2) Copy pstat_grandienthate.3db and pstat_grandientlove.3db to "DATA\INTERFACE\NEURONET\INVENTORY\"

3) Open "EXE\dacom.ini" and add FactionList.dll under [Libraries] section. Save changes.

4) Open "DATA\INTERFACE\HudShift.ini" and Copy-Paste this new section to the end then save changes:

[Group]
nickname = FactionList
location = center
position = 4A793B, 0.405, 4A7943, 0.238  ; PlayerInfoCloseButton
position = 4A79D5, -0.435 ; PlayerInfoTitle
position = 4A7A5A, -0.075  ; PlayerInfoCard
position = 4A7BD0, -0.263  ; PlayerInfoFactionFrame%d
position = 4A7C39, -0.257 ; PlayerInfoFactionName%d
position = 4A7CB6, -0.325 ; PlayerInfoUnfriendlyBar%d
position = 4A7DCC, -0.202 ; PlayerInfofriendlyBar%d
position = 4A7E29, -0.123 ; PlayerInfoScroll
position = 4A7A1C, 0.335  ; PlayerStatsTitle
position = 4A7F78, 0.11   ; PlayerInfoHeading%d
position = 4A7FDA, 0.41   ; PlayerInfoStat%d
position = 4A7AC1, 0.18, 4A7AC9, -0.210 ; CloseInfocardButton
position = 4A7B11, 0.05, 4A7B19, -0.190 ; CloseInfocardText







##################### PROGRAMMING BREAKING CHANGE #####################

This plugin eliminates 0x334-0x394 offsets in NN_PlayerInfo.
Those offsets were address range for wire[] text[] hate[] love[] arrays of controls which is NULL now.

New address range is now at the end of NN_PlayerInfo. 
This is done by increasing NN_PlayerInfo's allocation size at Freelancer.exe + 0x4A911B
and moving new extended array of components there with fixing up any place ones are being accessed.