#include <GUIConstants.au3>
#include <GUIListView.au3>
#include <GuiEdit.au3>

; AutoItSetOption("MustDeclareVars", 1)

Dim $gs_ConsoleMsgs

;
; Path of HP USB DIsk Format Utility, syslinux, EXTIMAGE
;

Const $LIVEXBMC_FILES = "LiveXBMC"

Const $HPUSBFORMAT_EXE = "HPUSBF.exe"
Const $HPUSBFORMAT_SYSFILES = "BOOTFILES"

Const $SYSLINUX_EXE = "syslinux.exe"
Const $SYSLINUX_SWITCHES = "-f -m -a"

Const $EXTIMAGE_EXE = "EXTIMAGE.exe"
Const $EXTIMAGE_DLL = "WIMADLL.DLL"


$HPUSBFORMAT_Path = @ScriptDir & "\Helpers\" & $HPUSBFORMAT_EXE
$HPUSBFORMAT_SYSFILES_Path = @ScriptDir & "\" & $HPUSBFORMAT_SYSFILES
$HPUSBFORMAT_SWITCHES = "-Y -FS:FAT -B:" & '"' & $HPUSBFORMAT_SYSFILES_Path & '"'

$SYSLINUX_Path = @ScriptDir & "\Helpers\" & $SYSLINUX_EXE

$EXTIMAGE_Path = @ScriptDir & "\Helpers\" & $EXTIMAGE_EXE
$EXTIMAGE_DLL_Path = @ScriptDir & "\Helpers\" & $EXTIMAGE_DLL


if not IsAdmin() Then
	MsgBox(0, "Bye!", "You must have administrator rights to continue, exiting ...")
	Exit
EndIf

If not FileExists($HPUSBFORMAT_Path) Then
	MsgBox(0, "Bye!", "File: " &  $HPUSBFORMAT_Path & " does not exist, exiting ...")
	Exit
EndIf

If not FileExists($SYSLINUX_Path) Then
	MsgBox(0, "Bye!", "File: " &  $SYSLINUX_Path & " does not exist, exiting ...")
	Exit
EndIf

If not FileExists($EXTIMAGE_Path) Then
	MsgBox(0, "Bye!", "File: " &  $EXTIMAGE_Path & " does not exist, exiting ...")
	Exit
EndIf

If not FileExists($EXTIMAGE_DLL_Path) Then
	MsgBox(0, "Bye!", "File: " &  $EXTIMAGE_DLL_Path & " does not exist, exiting ...")
	Exit
EndIf



; Create the GUI and the controls inside it
$GUI = GUICreate("LiveXBMC Image Creator", 512, 376)

; Create the menu
$M_File = GUICtrlCreateMenu("File")
$M_Exit = GUICtrlCreateMenuItem("Exit", $M_File)

; Create the "Select" button
$hFileSelButton = GuiCtrlCreateButton("Select Image", 8, 8, 92, 24)

; Create the read-only textbox 
$hImageFileName = GuiCtrlCreateInput("", 104, 8, 398, 24, $ES_READONLY)

; Create the removable drive combo-box
GuiCtrlCreateLabel("Select drive:", 40, 44, 72, 24)
$hDriveCombo = GuiCtrlCreatecombo("", 104, 40, 120, 100)

; Create the "Go" button
$hWriteImageButton = GuiCtrlCreateButton("Write Image", 392, 38, 92, 24)

$ConsoleBox = GUICtrlCreateEdit("", 8, 72, 496, 276, $ES_AUTOVSCROLL)
GUICtrlSetFont(-1, 10, 400, Default, "Courier New")

GUICtrlSetState($hDriveCombo, $GUI_DISABLE)
GUICtrlSetState($hWriteImageButton, $GUI_DISABLE)

; We have to show the GUI
GUISetState(@SW_SHOW)


writeConsole("Welcome to LiveXBMC Image Creator by Luigi Capriotti")

$remDrives = findRemovableDisks()
If $remDrives = "" Then
	MsgBox(0, "Bye!", "No removable disks found, exiting ...")
else
	$diskArray= StringSplit($remDrives, "|")
	GUICtrlSetState($hDriveCombo, $GUI_ENABLE)
	GUICtrlSetData($hDriveCombo, $remDrives, $diskArray[1])

	While 1
		$iMsg = GUIGetMsg()
		Select
		Case $iMsg = $hFileSelButton
			$sImageFileName = FileOpenDialog("Select file:", @ScriptDir, "Image files (*.img)")
			if FileExists($sImageFileName) Then
				GUICtrlSetData($hImageFileName, $sImageFileName)
				GUICtrlSetState($hWriteImageButton, $GUI_ENABLE)
			Endif
		Case $iMsg = $hWriteImageButton
			$sRemovableDrive = GUICtrlRead($hDriveCombo)
			writeImage($sImageFileName, $sRemovableDrive)
		Case $iMsg = $M_Exit
			Exit
		Case $iMsg = $GUI_EVENT_CLOSE
			Exit
		EndSelect
	WEnd
EndIf

Exit


Func findRemovableDisks()
	$drives = ""
	$drivesrem = DriveGetDrive("REMOVABLE")
	If IsArray($drivesrem) Then
    		For $i = 1 To $drivesrem[0]
        		$drives = $drivesrem[$i] & "|" & $drives
    		Next
	EndIf
	Return StringUpper($drives)
EndFunc

Func writeImage($sFName, $sDrive)
	;
	; Formatting Removable Disk
	;
	writeConsole("Formatting Drive " & $sDrive & " ...") 

	$ExternalCmdLine = $HPUSBFORMAT_Path & " " & $sDrive & " " & $HPUSBFORMAT_SWITCHES
	$oExec = RunWait($ExternalCmdLine, @ScriptDir, @SW_HIDE)
	;
	; Remove command.com from flash (not needed any more)
	;
	; FileDelete($sDrive & "\command.com" )

	;
	; Extract files from LiveXBMC image
	;
	writeConsole("Extracting files from " & $sFName)

	$ExternalCmdLine = $EXTIMAGE_Path & " " & $sFName & " " & $sDrive & "\"
	$oExec = RunWait($ExternalCmdLine, @ScriptDir, @SW_HIDE)

	;
	; Syslinux-ing the drive
	;
	writeConsole("Now injecting syslinux on Drive " & $sDrive & " ...") 

	$ExternalCmdLine = $SYSLINUX_Path & " " & $SYSLINUX_SWITCHES & " " & $sDrive 
	$oExec = RunWait($ExternalCmdLine, @ScriptDir, @SW_HIDE)

	writeConsole("All done!" & @CRLF) 
EndFunc

Func writeConsole($aText)
	$gs_ConsoleMsgs &= $aText & @CRLF
	GUICtrlSetData($ConsoleBox, $gs_ConsoleMsgs)
	GUICtrlSendMsg ($ConsoleBox, $EM_SCROLLCARET,0,0)
EndFunc
