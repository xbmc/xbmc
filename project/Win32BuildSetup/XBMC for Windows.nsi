;XBMC for Windows install script
;Copyright (C) 2005-2008 Team XBMC
;http://xbmc.org

;Script by chadoe

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "nsDialogs.nsh"
  !include "LogicLib.nsh"
;--------------------------------
;General

  ;Name and file
  Name "XBMC for Windows"
  OutFile "XBMCSetup-Rev${xbmc_revision}.exe"

  XPStyle on
  
  ;Default installation folder
  InstallDir "$PROGRAMFILES\XBMC"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\XBMC" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel highest

;--------------------------------
;Variables

  Var StartMenuFolder
  
;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  ;!define MUI_HEADERIMAGE_BITMAP "xbmc-banner.bmp"
  ;!define MUI_HEADERIMAGE_RIGHT
  !define MUI_WELCOMEFINISHPAGE_BITMAP "xbmc-left.bmp"
  !define MUI_COMPONENTSPAGE_SMALLDESC
  ;!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\webapps\ROOT\RELEASE-NOTES.txt"
  !define MUI_FINISHPAGE_LINK "Please visit http://xbmc.org for more information."
  !define MUI_FINISHPAGE_LINK_LOCATION "http://xbmc.org"
  !define MUI_FINISHPAGE_RUN "$INSTDIR\XBMC.exe"
  !define MUI_FINISHPAGE_RUN_PARAMETERS "-fs -p"
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_ABORTWARNING  
;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "..\..\LICENSE.GPL"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\XBMC" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder  

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

InstType "Full"
InstType "Minimal" 

Section "XBMC" SecXBMC
  SectionIn RO
  SectionIn 1 2 #section is in installtype Full and Minimal
  ;ADD YOUR OWN FILES HERE...
  SetOutPath "$INSTDIR"
  File "${xbmc_root}\Xbmc\XBMC.exe"
  File "${xbmc_root}\copying.txt"
  File "${xbmc_root}\keymapping.txt"
  File "${xbmc_root}\LICENSE.GPL"
  File "dependencies\*.*"
  SetOutPath "$INSTDIR\credits"
  File /r /x *.so ${xbmc_root}\Xbmc\credits\*.*
  SetOutPath "$INSTDIR\media"
  File /r /x *.so ${xbmc_root}\Xbmc\media\*.*
  SetOutPath "$INSTDIR\scripts"
  File /nonfatal /r /x *.so ${xbmc_root}\Xbmc\scripts\*.*
  SetOutPath "$INSTDIR\sounds"
  File /r /x *.so ${xbmc_root}\Xbmc\sounds\*.*
  SetOutPath "$INSTDIR\system"
  File /r /x *.so /x mplayer ${xbmc_root}\Xbmc\system\*.*
  SetOutPath "$INSTDIR\userdata"
  File /r /x *.so  ${xbmc_root}\Xbmc\userdata\*.*
  SetOutPath "$INSTDIR\visualisations"
  File ${xbmc_root}\Xbmc\visualisations\*_win32.vis
  ;File /r ${xbmc_root}\visualisations\projectM\*.*

  ;Store installation folder
  WriteRegStr HKCU "Software\XBMC" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\XBMC.lnk" "$INSTDIR\XBMC.exe" \
    "-fs -p" "$INSTDIR\XBMC.exe" 0 SW_SHOWNORMAL \
    "" "Start XBMC in fullscreen."
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\XBMC (Windowed).lnk" "$INSTDIR\XBMC.exe" \
    "-p" "$INSTDIR\XBMC.exe" 0 SW_SHOWNORMAL \
    "" "Start XBMC in windowed mode."
  
  WriteINIStr "$SMPROGRAMS\$StartMenuFolder\Visit XBMC Online.url" "InternetShortcut" "URL" "http://xbmc.org"
  !insertmacro MUI_STARTMENU_WRITE_END  
	
  ;add entry to add/remove programs
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "DisplayName" "XBMC for Windows"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "NoRepair" 1
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "DisplayIcon" "$INSTDIR\XBMC.exe,0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "Publisher" "Team XBMC"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "HelpLink" "http://xbmc.org/forum/index.php"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "HelpLink" "http://xbmc.org/forum/index.php"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "URLInfoAbout" "http://xbmc.org"
SectionEnd

SectionGroup "Language" SecLanguages
Section "English" SecLanguageEnglish
  SectionIn 1 2 #section is in installtype Full and Minimal
  SectionIn RO
  SetOutPath "$INSTDIR\language\English"
  File /r ${xbmc_root}\Xbmc\language\English\*.*
SectionEnd
;languages.nsi is generated by genNsisIncludes.bat
!include /nonfatal "languages.nsi"
SectionGroupEnd

SectionGroup "Skins" SecSkins
Section "Project Mayhem III" SecSkinPMIII
  SectionIn 1 2 #section is in installtype Full and Minimal
  SectionIn RO
  SetOutPath "$INSTDIR\skin\Project Mayhem III"
  File /r "${xbmc_root}\Xbmc\skin\Project Mayhem III\*.*"
SectionEnd
;skins.nsi is generated by genNsisIncludes.bat
!include /nonfatal "skins.nsi"
SectionGroupEnd

;SectionGroup "Scripts" SecScripts
;scripts.nsi is generated by genNsisIncludes.bat
!include /nonfatal "scripts.nsi"
;SectionGroupEnd

;SectionGroup "Plugins" SecPlugins
;plugins.nsi is generated by genNsisIncludes.bat
!include /nonfatal "plugins.nsi"
;SectionGroupEnd

Section "-hidden section"
;setup sources.xml
  SetOutPath "$APPDATA\XBMC\UserData"
  SetOverwrite off
  File "${xbmc_root}\Xbmc\sources.xml"
  SetOverwrite on

  ;setup sources.xml for plugins  
  IfFileExists "$APPDATA\XBMC\UserData\sources.xml" 0
  !ifdef SecPluginsMusic
  ${If} ${SectionIsSelected} ${SecPluginsMusic}
    ;check if plugin is already added to sources
    Push "$APPDATA\XBMC\UserData\sources.xml"
    Push "plugin://music"
    Call FileSearch
    Pop $0 #Number of times found throughout
    Pop $1 #Number of lines found on
	;if not found add the new plugin source
	${If} $0 == 0
      Push "</music>" #text to be replaced
      Push "<source><name>Music Plugins</name><path>plugin://music/</path></source></music>" #replace with
      Push all #replace all occurrences
      Push all #replace all occurrences
      Push "$APPDATA\XBMC\UserData\sources.xml" #file to replace in
      Call AdvReplaceInFile
    ${EndIf}
  ${EndIf}
  !endif
  !ifdef SecPluginsVideo
  ${If} ${SectionIsSelected} ${SecPluginsVideo}
    ;check if plugin is already added to sources
    Push "$APPDATA\XBMC\UserData\sources.xml"
    Push "plugin://video"
    Call FileSearch
    Pop $0 #Number of times found throughout
    Pop $1 #Number of lines found on
	;if not found add the new plugin source
	${If} $0 == 0
      Push "</video>" #text to be replaced
      Push "<source><name>Video Plugins</name><path>plugin://video/</path></source></video>" #replace with
      Push all #replace all occurrences
      Push all #replace all occurrences
      Push "$APPDATA\XBMC\UserData\sources.xml" #file to replace in
      Call AdvReplaceInFile
    ${EndIf}
  ${EndIf}
  !endif
  !ifdef SecPluginsPrograms
  ${If} ${SectionIsSelected} ${SecPluginsPrograms}
    ;check if plugin is already added to sources
    Push "$APPDATA\XBMC\UserData\sources.xml"
    Push "plugin://programs"
    Call FileSearch
    Pop $0 #Number of times found throughout
    Pop $1 #Number of lines found on
	;if not found add the new plugin source
	${If} $0 == 0
      Push "</programs>" #text to be replaced
      Push "<source><name>Program Plugins</name><path>plugin://programs/</path></source></programs>" #replace with
      Push all #replace all occurrences
      Push all #replace all occurrences
      Push "$APPDATA\XBMC\UserData\sources.xml" #file to replace in
      Call AdvReplaceInFile
    ${EndIf}
  ${EndIf}
  !endif
  !ifdef SecPluginsPictures
  ${If} ${SectionIsSelected} ${SecPluginsPictures}
    ;check if plugin is already added to sources
    Push "$APPDATA\XBMC\UserData\sources.xml"
    Push "plugin://pictures"
    Call FileSearch
    Pop $0 #Number of times found throughout
    Pop $1 #Number of lines found on
	;if not found add the new plugin source
	${If} $0 == 0
      Push "</pictures>" #text to be replaced
      Push "<source><name>Picture Plugins</name><path>plugin://pictures/</path></source></pictures>" #replace with
      Push all #replace all occurrences
      Push all #replace all occurrences
      Push "$APPDATA\XBMC\UserData\sources.xml" #file to replace in
      Call AdvReplaceInFile
    ${EndIf}
  ${EndIf}
  !endif
SectionEnd
;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecXBMC ${LANG_ENGLISH} "XBMC for Windows."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecXBMC} $(DESC_SecXBMC)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...
  Delete "$INSTDIR\XBMC.exe"
  Delete "$INSTDIR\copying.txt"
  Delete "$INSTDIR\keymapping.txt"
  Delete "$INSTDIR\LICENSE.GPL"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\jpeg.dll"
  Delete "$INSTDIR\libpng12-0.dll"
  Delete "$INSTDIR\libtiff-3.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\SDL_image.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\xbmc.log"
  Delete "$INSTDIR\xbmc.old.log"
  RMDir /r "$INSTDIR\credits"
  RMDir /r "$INSTDIR\language"
  RMDir /r "$INSTDIR\media"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\scripts"
  RMDir /r "$INSTDIR\skin"
  RMDir /r "$INSTDIR\sounds"
  RMDir /r "$INSTDIR\system"
  RMDir /r "$INSTDIR\userdata"
  RMDir /r "$INSTDIR\visualisations"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC (Windowed).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Visit XBMC Online.url"
  RMDir "$SMPROGRAMS\$StartMenuFolder"  
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC"

  DeleteRegKey /ifempty HKCU "Software\XBMC"

SectionEnd


Function AdvReplaceInFile
Exch $0 ;file to replace in
Exch
Exch $1 ;number to replace after
Exch
Exch 2
Exch $2 ;replace and onwards
Exch 2
Exch 3
Exch $3 ;replace with
Exch 3
Exch 4
Exch $4 ;to replace
Exch 4
Push $5 ;minus count
Push $6 ;universal
Push $7 ;end string
Push $8 ;left string
Push $9 ;right string
Push $R0 ;file1
Push $R1 ;file2
Push $R2 ;read
Push $R3 ;universal
Push $R4 ;count (onwards)
Push $R5 ;count (after)
Push $R6 ;temp file name
 
  GetTempFileName $R6
  FileOpen $R1 $0 r ;file to search in
  FileOpen $R0 $R6 w ;temp file
   StrLen $R3 $4
   StrCpy $R4 -1
   StrCpy $R5 -1
 
loop_read:
 ClearErrors
 FileRead $R1 $R2 ;read line
 IfErrors exit
 
   StrCpy $5 0
   StrCpy $7 $R2
 
loop_filter:
   IntOp $5 $5 - 1
   StrCpy $6 $7 $R3 $5 ;search
   StrCmp $6 "" file_write2
   StrCmp $6 $4 0 loop_filter
 
StrCpy $8 $7 $5 ;left part
IntOp $6 $5 + $R3
IntCmp $6 0 is0 not0
is0:
StrCpy $9 ""
Goto done
not0:
StrCpy $9 $7 "" $6 ;right part
done:
StrCpy $7 $8$3$9 ;re-join
 
IntOp $R4 $R4 + 1
StrCmp $2 all file_write1
StrCmp $R4 $2 0 file_write2
IntOp $R4 $R4 - 1
 
IntOp $R5 $R5 + 1
StrCmp $1 all file_write1
StrCmp $R5 $1 0 file_write1
IntOp $R5 $R5 - 1
Goto file_write2
 
file_write1:
 FileWrite $R0 $7 ;write modified line
Goto loop_read
 
file_write2:
 FileWrite $R0 $R2 ;write unmodified line
Goto loop_read
 
exit:
  FileClose $R0
  FileClose $R1
 
   SetDetailsPrint none
  Delete $0
  Rename $R6 $0
  Delete $R6
   SetDetailsPrint both
 
Pop $R6
Pop $R5
Pop $R4
Pop $R3
Pop $R2
Pop $R1
Pop $R0
Pop $9
Pop $8
Pop $7
Pop $6
Pop $5
Pop $0
Pop $1
Pop $2
Pop $3
Pop $4
FunctionEnd

Function FileSearch
Exch $R0 ;search for
Exch
Exch $R1 ;input file
Push $R2
Push $R3
Push $R4
Push $R5
Push $R6
Push $R7
Push $R8
Push $R9
 
  StrLen $R4 $R0
  StrCpy $R7 0
  StrCpy $R8 0
 
  ClearErrors
  FileOpen $R2 $R1 r
  IfErrors Done
 
  LoopRead:
    ClearErrors
    FileRead $R2 $R3
    IfErrors DoneRead
 
    IntOp $R7 $R7 + 1
    StrCpy $R5 -1
    StrCpy $R9 0
 
    LoopParse:
      IntOp $R5 $R5 + 1
      StrCpy $R6 $R3 $R4 $R5
      StrCmp $R6 "" 0 +4
        StrCmp $R9 1 LoopRead
          IntOp $R7 $R7 - 1
          Goto LoopRead
      StrCmp $R6 $R0 0 LoopParse
        StrCpy $R9 1
        IntOp $R8 $R8 + 1
        Goto LoopParse
 
  DoneRead:
    FileClose $R2
  Done:
    StrCpy $R0 $R8
    StrCpy $R1 $R7
 
Pop $R9
Pop $R8
Pop $R7
Pop $R6
Pop $R5
Pop $R4
Pop $R3
Pop $R2
Exch $R1 ;number of lines found on
Exch
Exch $R0 ;output count found
FunctionEnd