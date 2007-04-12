;FileZilla Server Setup script
;written by Tim Kosse (Tim.Kosse@gmx.de)
;Based on
;NSIS Modern User Interface version 1.6
;Basic Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"
  !define MUI_LICENSEBKCOLOR /grey

;--------------------------------
;Product Info

  !define VERSION "0.8.8" ;Define your own software version here
  Name "FileZilla Server ${VERSION}" ;Define your own software name here

;StartOptions Page strings
LangString StartOptionsTitle ${LANG_ENGLISH} ": Server startup settings"

;--------------------------------
;Preprocessing of input files
  !system 'upx --best -f -q "..\release\FileZilla Server.exe" "..\interface\release\FileZilla Server Interface.exe" "SettingsConverter.dll"' ignore
  !packhdr temp.dat "upx.exe --best temp.dat"

;--------------------------------
;Modern UI Configuration

  !define MUI_ABORTWARNING
  
  !define MUI_ICON "..\res\filezilla server.ico"
  !define MUI_UNICON "uninstall.ico"
  
;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "..\..\license.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  Page custom StartOptions
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;More
  
  ;General
  OutFile "../../FileZilla_Server.exe"
  
  ;Installation types
  InstType "Standard"
  InstType "Full"

  ;Descriptions
  LangString DESC_SecFileZillaServer ${LANG_ENGLISH} "Copy all required files of FileZilla server to the application folder."
  LangString DESC_SecSourceCode ${LANG_ENGLISH} "Copy the source code of FileZilla Server to the application folder"
  LangString DESC_SecStartMenu ${LANG_ENGLISH} "Create shortcuts to FileZilla Server in the Start menu"
  LangString DESC_SecDesktopIcon ${LANG_ENGLISH} "Create an Icon on the desktop for quick access to FileZilla Server"

  ;Folder-selection page
  InstallDir "$PROGRAMFILES\FileZilla Server"

  ShowInstDetails show

;--------------------------------
;Installer Sections

Section "FileZilla Server (required)" SecFileZillaServer
  SectionIn 1 2 RO
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  IfFileExists "$INSTDIR\FileZilla Server.exe" found

  File "..\Release\FileZilla Server.exe"
  DetailPrint "Stopping service..."
  ExecWait '"$INSTDIR\FileZilla Server.exe" /stop'
  Sleep 500
  Push "FileZilla Server Helper Window"
  call CloseWindowByName
  DetailPrint "Uninstalling service..."
  ExecWait '"$INSTDIR\FileZilla Server.exe" /uninstall'  
  Sleep 500
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server"
  goto copy_main_done
 found:
  GetTempFileName $R1
  File /oname=$R1 "..\Release\FileZilla Server.exe"
  DetailPrint "Stopping service..."
  ExecWait '"$R1" /stop'
  Sleep 500
  Push "FileZilla Server Helper Window"
  call CloseWindowByName
  DetailPrint "Uninstalling service..."
  ExecWait '"$R1" /uninstall'  
  Sleep 500
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server"
  Delete "$INSTDIR\FileZilla Server.exe"
  Rename $R1 "$INSTDIR\FileZilla Server.exe"
 copy_main_done:

  File "..\Release\FileZilla server.pdb"

  ; Stopping interface
  DetailPrint "Closing interface..."
  Push "FileZilla Server Main Window"
  Call CloseWindowByName

  ; Put file there
  File "..\Release\FileZilla server.pdb"
  File "..\Interface\Release\FileZilla Server Interface.exe"
  File "FzGss.dll"
  File "..\..\readme.htm"
  File "..\..\license.txt"
  File "dbghelp.dll"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileZilla Server" "DisplayName" "FileZilla Server (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileZilla Server" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Source Code" SecSourceCode
SectionIn 2
  SetOutPath $INSTDIR\source
  File "..\*.cpp"
  File "..\*.h"
  File "..\FileZilla Server.dsw"
  File "..\FileZilla Server.dsp"
  File "..\FileZilla Server.rc"
  SetOutPath $INSTDIR\source\res
  File "..\res\*.ico"
  SetOutPath $INSTDIR\source\misc
  File "..\misc\*.h"
  File "..\misc\*.cpp"
  SetOutPath $INSTDIR\source\interface
  File "..\interface\*.cpp"
  File "..\interface\*.h"
  File "..\interface\FileZilla Server Interface.dsp"
  File "..\interface\FileZilla Server.rc"
  SetOutPath $INSTDIR\source\interface\res
  File "..\interface\res\*.bmp"
  File "..\interface\res\*.ico"
  File "..\interface\res\*.rc2"
  SetOutPath $INSTDIR\source\interface\misc
  File "..\interface\misc\*.h"
  File "..\interface\misc\*.cpp"
  SetOutPath "$INSTDIR\source\Settings Converter\"
  File "..\Settings Converter\*.h"
  File "..\Settings Converter\*.cpp"
  File "..\Settings Converter\Settings Converter.dsw"
  File "..\Settings Converter\Settings Converter.dsp"
  SetOutPath "$INSTDIR\source\Settings Converter\misc"
  File "..\Settings Converter\misc\*.h"
  File "..\Settings Converter\misc\*.cpp"
  SetOutPath $INSTDIR\source\install
  File "FileZilla Server.nsi"
  File "StartupOptions9x.ini"
  File "StartupOptions.ini"
  File "uninstall.ico"
  StrCpy $0 "source"
  
SectionEnd

; optional section
Section "Start Menu Shortcuts" SecStartMenu
SectionIn 1 2
  CreateDirectory "$SMPROGRAMS\FileZilla Server"
  CreateShortCut "$SMPROGRAMS\FileZilla Server\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\FileZilla Server\FileZilla Server Interface.lnk" "$INSTDIR\FileZilla Server Interface.exe" "" "$INSTDIR\FileZilla Server Interface.exe" 0

  strcmp $1 "9x" shortcut9x
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 2" "State"
  StrCmp $R0 "Do not install as service, start server automatically (not recommended)" shortcut9x

  ;NT service shortcuts
  CreateShortCut "$SMPROGRAMS\FileZilla Server\Start FileZilla Server.lnk" "$INSTDIR\FileZilla Server.exe" "/start" "$INSTDIR\FileZilla Server.exe" 0
  CreateShortCut "$SMPROGRAMS\FileZilla Server\Stop FileZilla Server.lnk" "$INSTDIR\FileZilla Server.exe" "/stop" "$INSTDIR\FileZilla Server.exe" 0
  goto shortcut_done
 shortcut9x: 
  ;Compat mode
  CreateShortCut "$SMPROGRAMS\FileZilla Server\Start FileZilla Server.lnk" "$INSTDIR\FileZilla Server.exe" "/compat /start" "$INSTDIR\FileZilla Server.exe" 0
  CreateShortCut "$SMPROGRAMS\FileZilla Server\Stop FileZilla Server.lnk" "$INSTDIR\FileZilla Server.exe" "/compat /stop" "$INSTDIR\FileZilla Server.exe" 0

 shortcut_done:
  
  StrCmp $0 "source" CreateSourceShortcuts NoSourceShortcuts
 CreateSourceShortcuts:
  CreateShortCut "$SMPROGRAMS\FileZilla Server\FileZilla Server Source Project.lnk" "$INSTDIR\source\FileZilla Server.dsw" "" "$INSTDIR\source\FileZilla Server.dsw" 0
 NoSourceShortcuts:
SectionEnd

Section "Desktop Icon" SecDesktopIcon
SectionIn 1 2
  CreateShortCut "$DESKTOP\FileZilla Server Interface.lnk" "$INSTDIR\FileZilla Server Interface.exe" "" "$INSTDIR\FileZilla Server Interface.exe" 0
SectionEnd

Section "-PostInst"

  ; Convert old settings
  ; We've to use Pre 2.0a4 syntax because the new syntax does not check the working dir if a plugin is present in it
  DetailPrint "Converting old settings..."
  SetDetailsPrint none
  SetOutPath $TEMP
  GetTempFileName $R0
  File /oname=$R0 SettingsConverter.dll
  Push "$INSTDIR\FileZilla Server.xml"
  CallInstDLL $R0 convert
  Delete $R0
  SetDetailsPrint both

  ; Write the installation path into the registry
  WriteRegStr HKCU "SOFTWARE\FileZilla Server" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "SOFTWARE\FileZilla Server" "Install_Dir" "$INSTDIR"

  ;Set Adminport
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 4" "State"
  ExecWait '"$INSTDIR\FileZilla Server.exe" /adminport $R0'
  ExecWait '"$INSTDIR\FileZilla Server Interface.exe" /adminport $R0'
  
  StrCmp $1 "9x" Install_9x
  
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 2" "State"
  StrCmp $R0 "Do not install as service, started automatically (not recommended)" Install_Standard_Auto
  DetailPrint "Installing Service..."
  StrCmp $R0 "Install as service, started manually" Install_AsService_Manual

  ExecWait '"$INSTDIR\FileZilla Server.exe" /install auto'
  goto done
 Install_AsService_Manual:
  ExecWait '"$INSTDIR\FileZilla Server.exe" /install'
  goto done
 Install_9x:
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 2" "State"
  StrCmp $R0 "Start manually" done  
 Install_Standard_Auto:
  DetailPrint "Put FileZilla Server into registry..."
  ClearErrors
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server" '"$INSTDIR\FileZilla Server.exe" /compat /start'
  IfErrors Install_Standard_Auto_CU
  goto done
 Install_Standard_Auto_CU:
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server" '"$INSTDIR\FileZilla Server.exe" /compat /start'
 done:

  ;Write interface startup settings
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 6" "State"
  StrCmp $R0 "Start manually" interface_done
  DetailPrint "Put FileZilla Server Interface into registry..."
  StrCmp $R0 "Start if user logs on, apply only to current user" interface_cu
  ClearErrors
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server Interface" '"$INSTDIR\FileZilla Server Interface.exe"'
  IfErrors interface_cu
  goto interface_done
 interface_cu:
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server Interface" '"$INSTDIR\FileZilla Server Interface.exe"'
 interface_done:  

SectionEnd

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFileZillaServer} $(DESC_SecFileZillaServer)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSourceCode} $(DESC_SecSourceCode)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} $(DESC_SecStartMenu)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopIcon} $(DESC_SecDesktopIcon)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Installer Functions

Function .onInit

  ;Detect Windows type (NT or 9x)

  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $R0 "" 0 detection_NT

  ; we are not NT.
  StrCpy $1 "9x"
  
  ;Extract InstallOptions INI Files
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "StartupOptions9x.ini"
  StrCpy $2 "StartupOptions9x.ini"

  goto detection_end
 detection_NT:
  ; we are NT
  StrCpy $1 "NT"
  
  ;Extract InstallOptions INI Files
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "StartupOptions.ini"
  strcpy $2 "StartupOptions.ini"

 detection_end:

FunctionEnd
 
LangString TEXT_IO_TITLE ${LANG_ENGLISH} "Startup settings"
LangString TEXT_IO_SUBTITLE ${LANG_ENGLISH} "Select startup behaviour for FileZilla Server"

Function StartOptions
  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
  StrCmp $1 "9x" sodisplay_9x
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "StartupOptions.ini"
  goto sodisplay_end
 sodisplay_9x:
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "StartupOptions9x.ini"
 sodisplay_end:
FunctionEnd

Function .onInstSuccess
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 7" "State"
  strcmp $R0 "0" startserverend

  strcmp §1 "9x" startserver9x
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 2" "State"
  StrCmp $R0 "Do not install as service, started automatically (not recommended)" startserver9x

  Exec '"$INSTDIR\FileZilla Server.exe" /start'
  goto startserverend
 startserver9x:
  Exec '"$INSTDIR\FileZilla Server.exe" /compat /start'
 startserverend:

  !insertmacro MUI_INSTALLOPTIONS_READ $R0 $2 "Field 8" "State"
  strcmp $R0 "0" NoStartInterface
  Exec '"$INSTDIR\FileZilla Server Interface.exe"'
 NoStartInterface:
FunctionEnd

Function CloseWindowByName
  Exch $R1
 closewindow_start:
  FindWindow $R0 $R1
  strcmp $R0 0 closewindow_end
  SendMessage $R0 ${WM_CLOSE} 0 0
  Sleep 500
  goto closewindow_start
 closewindow_end:
  Pop $R1
FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; Stopping and uninstalling service
  DetailPrint "Stopping service..."
  ExecWait '"$INSTDIR\FileZilla Server.exe" /stop'
  Sleep 500
  Push "FileZilla Server Helper Window"
  Call un.CloseWindowByName
  DetailPrint "Uninstalling service..."
  ExecWait '"$INSTDIR\FileZilla Server.exe" /uninstall'  
  Sleep 500

  ; Stopping interface
  DetailPrint "Closing interface..."
  Push "FileZilla Server Main Window"
  Call un.CloseWindowByName

  ; remove registry keys
  DeleteRegValue HKCU "Software\FileZilla Server" "Install_Dir"
  DeleteRegValue HKLM "Software\FileZilla Server" "Install_Dir"
  DeleteRegKey /ifempty HKCU "Software\FileZilla Server"
  DeleteRegKey /ifempty HKLM "Software\FileZilla Server"
  MessageBox MB_YESNO "Delete settings?" IDNO NoSettingsDelete
  Delete "$INSTDIR\FileZilla Server.xml"
  Delete "$INSTDIR\FileZilla Server Interface.xml"
 NoSettingsDelete:
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileZilla Server"
  ; remove files
  Delete "$INSTDIR\FileZilla Server.exe"
  Delete "$INSTDIR\FileZilla Server Interface.exe"
  Delete "$INSTDIR\FileZilla server.pdb"
  Delete $INSTDIR\FzGss.dll
  Delete "$INSTDIR\dbghelp.dll"
  Delete $INSTDIR\license.txt
  Delete $INSTDIR\readme.htm
  Delete $INSTDIR\source\*.cpp
  Delete $INSTDIR\source\*.h
  Delete "$INSTDIR\source\FileZilla Server.dsw"
  Delete "$INSTDIR\source\FileZilla Server.dsp"
  Delete "$INSTDIR\source\FileZilla Server.rc"
  Delete $INSTDIR\source\res\*.ico
  Delete $INSTDIR\source\res\*.bmp
  Delete $INSTDIR\source\res\*.rc2
  Delete $INSTDIR\source\misc\*.h
  Delete $INSTDIR\source\misc\*.cpp
  Delete $INSTDIR\source\interface\*.cpp
  Delete $INSTDIR\source\interface\*.h
  Delete "$INSTDIR\source\interface\FileZilla Server Interface.dsp"
  Delete "$INSTDIR\source\interface\FileZilla Server.rc"
  Delete $INSTDIR\source\interface\res\*.ico
  Delete $INSTDIR\source\interface\res\*.bmp
  Delete $INSTDIR\source\interface\res\*.rc2
  Delete $INSTDIR\source\interface\misc\*.h
  Delete $INSTDIR\source\interface\misc\*.cpp
  Delete "$INSTDIR\source\Settings Converter\*.h"
  Delete "$INSTDIR\source\Settings Converter\*.cpp"
  Delete "$INSTDIR\source\Settings Converter\Settings Converter.dsw"
  Delete "$INSTDIR\source\Settings Converter\Settings Converter.dsp"
  Delete "$INSTDIR\source\Settings Converter\misc\*.h"
  Delete "$INSTDIR\source\Settings Converter\misc\*.cpp"
  Delete $INSTDIR\source\install\uninstall.ico
  Delete "$INSTDIR\source\install\FileZilla Server.nsi"
  Delete "$INSTDIR\source\install\StartupOptions.ini"
  Delete "$INSTDIR\source\install\StartupOptions9x.ini"

  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  
  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\FileZilla Server\*.*"
  Delete "$DESKTOP\FileZilla Server Interface.lnk"
  RMDir "$SMPROGRAMS\FileZilla Server"

  ; remove directories used.

  RMDir "$INSTDIR\source\res"
  RMDir "$INSTDIR\source\misc"
  RMDir "$INSTDIR\source\interface\res"
  RMDir "$INSTDIR\source\interface\misc"
  RMDir "$INSTDIR\source\interface"
  RMDir "$INSTDIR\source\Settings Converter\res"
  RMDir "$INSTDIR\source\Settings Converter"
  RMDir "$INSTDIR\source\install"
  RMDir "$INSTDIR\source"
  RMDir "$INSTDIR"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server Interface"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "FileZilla Server Interface"

  RMDir "$INSTDIR"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.CloseWindowByName
  Exch $R1
 unclosewindow_start:
  FindWindow $R0 $R1
  strcmp $R0 0 unclosewindow_end
  SendMessage $R0 ${WM_CLOSE} 0 0
  Sleep 500
  goto unclosewindow_start
  Pop $R1
 unclosewindow_end:
FunctionEnd