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
  Name "XBMC Media Center"
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
  UninstPage custom un.UnPageProfile un.UnPageProfileLeave
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
  File "${xbmc_root}\Xbmc\copying.txt"
  File "${xbmc_root}\Xbmc\LICENSE.GPL"
  File "${xbmc_root}\Xbmc\known_issues.txt"
  File "${xbmc_root}\Xbmc\*.dll"
  SetOutPath "$INSTDIR\credits"
  File /r /x *.so "${xbmc_root}\Xbmc\credits\*.*"
  SetOutPath "$INSTDIR\media"
  File /r /x *.so "${xbmc_root}\Xbmc\media\*.*"
  SetOutPath "$INSTDIR\sounds"
  File /r /x *.so "${xbmc_root}\Xbmc\sounds\*.*"
  SetOutPath "$INSTDIR\system"
  File /r /x *.so /x mplayer "${xbmc_root}\Xbmc\system\*.*"
  
  ;Turn off overwrite to prevent files in xbmc\userdata\ from being overwritten
  SetOverwrite off
  
  SetOutPath "$INSTDIR\userdata"
  File /r /x *.so  "${xbmc_root}\Xbmc\userdata\*.*"
  
  ;Turn on overwrite for rest of install
  SetOverwrite on
  
  SetOutPath "$INSTDIR\visualisations"
  File "${xbmc_root}\Xbmc\visualisations\*_win32.vis"
  SetOutPath "$INSTDIR\visualisations\projectM"
  File /nonfatal /r "${xbmc_root}\Xbmc\visualisations\projectM\*.*"
  SetOutPath "$INSTDIR\web"
  File /r "${xbmc_root}\Xbmc\web\*.*"

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
                 "DisplayName" "XBMC Media Center"
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
  File /r "${xbmc_root}\Xbmc\language\English\*.*"
SectionEnd
;languages.nsi is generated by genNsisIncludes.bat
!include /nonfatal "languages.nsi"
SectionGroupEnd

SectionGroup "Skins" SecSkins
Section "PM3.HD" SecSkinPM3.HD
  SectionIn 1 2 #section is in installtype Full and Minimal
  SectionIn RO
  SetOutPath "$INSTDIR\skin\PM3.HD"
  File /r "${xbmc_root}\Xbmc\skin\PM3.HD\*.*"
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

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecXBMC ${LANG_ENGLISH} "XBMC Media Center."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecXBMC} $(DESC_SecXBMC)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Var UnPageProfileDialog
Var UnPageProfileCheckbox
Var UnPageProfileCheckbox_State
Var UnPageProfileEditBox

Function un.UnPageProfile
    !insertmacro MUI_HEADER_TEXT "Uninstall XBMC Media Center" "Remove XBMC's profile folder from your computer."
	nsDialogs::Create /NOUNLOAD 1018
	Pop $UnPageProfileDialog

	${If} $UnPageProfileDialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 12u "Do you want to delete the profile folder?"
	Pop $0

	${NSD_CreateText} 0 13u 100% 12u "$APPDATA\XBMC\"
	Pop $UnPageProfileEditBox
    SendMessage $UnPageProfileEditBox ${EM_SETREADONLY} 1 0

	${NSD_CreateLabel} 0 46u 100% 24u "Leave unchecked to keep the profile folder for later use or check to delete the profile folder."
	Pop $0

	${NSD_CreateCheckbox} 0 71u 100% 8u "Yes, also delete the profile folder."
	Pop $UnPageProfileCheckbox
	

	nsDialogs::Show
FunctionEnd

Function un.UnPageProfileLeave
${NSD_GetState} $UnPageProfileCheckbox $UnPageProfileCheckbox_State
FunctionEnd

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...
  Delete "$INSTDIR\XBMC.exe"
  Delete "$INSTDIR\copying.txt"
  Delete "$INSTDIR\known_issues.txt"
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
  RMDir /r "$INSTDIR\visualisations"
  RMDir /r "$INSTDIR\web"
  RMDir /r "$INSTDIR\cache"

  Delete "$INSTDIR\Uninstall.exe"
  
;Uninstall User Data if option is checked, otherwise skip
  ${If} $UnPageProfileCheckbox_State == ${BST_CHECKED}
    RMDir /r "$INSTDIR\userdata"  
    RMDir "$INSTDIR"
    RMDir /r "$APPDATA\XBMC\"
  ${Else}
;Even if userdata is kept in %appdata%\xbmc\userdata, the $INSTDIR\userdata should be cleaned up on uninstall if not used
;If guisettings.xml exists in the XBMC\userdata directory, do not delete XBMC\userdata directory
;If that file does not exists, then delete that folder and $INSTDIR
    IfFileExists $INSTDIR\userdata\guisettings.xml +3
      RMDir /r "$INSTDIR\userdata"  
      RMDir "$INSTDIR"
  ${EndIf}

  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC (Windowed).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Visit XBMC Online.url"
  RMDir "$SMPROGRAMS\$StartMenuFolder"  
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC"

  DeleteRegKey /ifempty HKCU "Software\XBMC"

SectionEnd
