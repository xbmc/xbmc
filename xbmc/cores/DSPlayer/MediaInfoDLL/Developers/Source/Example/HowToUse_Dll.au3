;Loading the DLL
$DLL=DllOpen("MediaInfo.dll")

;Info
$Info_Parameters=DllCall($DLL, "wstr", "MediaInfo_Option", "ptr", 0, "wstr", "Info_Parameters", "wstr", "")
MsgBox(0, "MediaInfo_Option - Info_Parameters", $Info_Parameters[0])
$Info_Capacities=DllCall($DLL, "wstr", "MediaInfo_Option", "ptr", 0, "wstr", "Info_Capacities", "wstr", "")
MsgBox(0, "MediaInfo_Option - Info_Capacities", $Info_Capacities[0])
$Info_Codecs=DllCall($DLL, "wstr", "MediaInfo_Option", "ptr", 0, "wstr", "Info_Codecs", "wstr", "")
MsgBox(0, "MediaInfo_Option - Info_Codecs", $Info_Codecs[0])

;New MediaInfo handle
$Handle=DllCall($DLL, "ptr", "MediaInfo_New")

;Open
$Open_Result=DllCall($DLL, "int", "MediaInfo_Open", "ptr", $Handle[0], "wstr", "Example.ogg")

;Inform with Complete=false
DllCall($DLL, "wstr", "MediaInfo_Option", "ptr", 0, "wstr", "Complete", "wstr", "")
$Inform=DllCall($DLL, "wstr", "MediaInfo_Inform", "ptr", $Handle[0], "int", 0)
MsgBox(0, "Inform with Complete=false", $Inform[0])

;Inform with Complete=true
DllCall($DLL, "wstr", "MediaInfo_Option", "ptr", 0, "wstr", "Complete", "wstr", "1")
$Inform=DllCall($DLL, "wstr", "MediaInfo_Inform", "ptr", $Handle[0], "int", 0)
MsgBox(0, "Inform with Complete=true", $Inform[0])

;Custom Inform 
DllCall($DLL, "wstr", "MediaInfo_Option", "ptr", 0, "wstr", "Inform", "wstr", "General;Example : FileSize=%FileSize%")
$Inform=DllCall($DLL, "wstr", "MediaInfo_Inform", "ptr", $Handle[0], "int", 0)
MsgBox(0, "Custom Inform", $Inform[0])

;Get with Stream=General and Parameter=FileSize
$Info_Get=DllCall($DLL, "wstr", "MediaInfo_Get", "ptr", $Handle[0], "int", 0, "int", 0, "wstr", "FileSize", "int", 1, "int", 0)
MsgBox(0, "Get with Stream=General and Parameter=FileSize", $Info_Get[0])

;GetI with Stream=General and Parameter=46
$Info_GetI=DllCall($DLL, "wstr", "MediaInfo_GetI", "ptr", $Handle[0], "int", 0, "int", 0, "int", 46, "int", 1, "int", 0)
MsgBox(0, "Get with Stream=General and Parameter=46", $Info_GetI[0])

;Count_Get with StreamKind=Stream_Audio
$Count_Get=DllCall($DLL, "int", "MediaInfo_Count_Get", "ptr", $Handle[0], "int", 2, "int", 0)
MsgBox(0, "Count_Get with StreamKind=Stream_Audio", $Count_Get[0])

;Get with Stream=General and Parameter=AudioCount
$Info_Get=DllCall($DLL, "wstr", "MediaInfo_Get", "ptr", $Handle[0], "int", 0, "int", 0, "wstr", "AudioCount", "int", 1, "int", 0)
MsgBox(0, "Get with Stream=General and Parameter=AudioCount", $Info_Get[0])

;Get with Stream=Audio and Parameter=StreamCount
$Info_Get=DllCall($DLL, "wstr", "MediaInfo_Get", "ptr", $Handle[0], "int", 2, "int", 0, "wstr", "StreamCount", "int", 1, "int", 0)
MsgBox(0, "Get with Stream=Audio and Parameter=StreamCount", $Info_Get[0])

;Delete MediaInfo handle
$Handle=DllCall($DLL, "none", "MediaInfo_Delete", "ptr", $Handle[0])

;Close the DLL
DllClose($dll)
