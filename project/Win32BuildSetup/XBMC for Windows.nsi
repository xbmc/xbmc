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
  Name "XBMC"
  OutFile "XBMCSetup-${xbmc_revision}-${xbmc_target}.exe"

  XPStyle on
  
  ;Default installation folder
  InstallDir "$PROGRAMFILES\XBMC"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\XBMC" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Variables

  Var StartMenuFolder
  Var PageProfileState
  Var RunArgs
  Var DirectXSetupError
  Var VSRedistSetupError
  
;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  !define MUI_ICON "..\..\xbmc\win32\xbmc.ico"
  ;!define MUI_HEADERIMAGE_BITMAP "xbmc-banner.bmp"
  ;!define MUI_HEADERIMAGE_RIGHT
  !define MUI_WELCOMEFINISHPAGE_BITMAP "xbmc-left.bmp"
  !define MUI_COMPONENTSPAGE_SMALLDESC
  ;!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\webapps\ROOT\RELEASE-NOTES.txt"
  !define MUI_FINISHPAGE_LINK "Please visit http://xbmc.org for more information."
  !define MUI_FINISHPAGE_LINK_LOCATION "http://xbmc.org"
  !define MUI_FINISHPAGE_RUN "$INSTDIR\XBMC.exe"
  ;!define MUI_FINISHPAGE_RUN_PARAMETERS $RunArgs
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
  SetShellVarContext current
  SectionIn RO
  SectionIn 1 2 #section is in installtype Full and Minimal
  ;ADD YOUR OWN FILES HERE...
  SetOutPath "$INSTDIR"
  File "${xbmc_root}\Xbmc\XBMC.exe"
  File "${xbmc_root}\Xbmc\copying.txt"
  File "${xbmc_root}\Xbmc\LICENSE.GPL"
  File "${xbmc_root}\Xbmc\*.dll"
  SetOutPath "$INSTDIR\media"
  File /r /x *.so "${xbmc_root}\Xbmc\media\*.*"
  SetOutPath "$INSTDIR\sounds"
  File /r /x *.so "${xbmc_root}\Xbmc\sounds\*.*"
  SetOutPath "$INSTDIR\system"
  
  ; delete system/python if its there
  IfFileExists $INSTDIR\system\python 0 +2
    RMDir /r $INSTDIR\system\python
  
  File /r /x *.so /x mplayer /x *_d.* /x tcl85g.dll /x tclpip85g.dll /x tk85g.dll "${xbmc_root}\Xbmc\system\*.*"
  
  ; delete  msvc?90.dll's in INSTDIR, we use the vcredist installer later
  Delete "$INSTDIR\msvcr90.dll"
  Delete "$INSTDIR\msvcp90.dll"
  
  ;Turn off overwrite to prevent files in xbmc\userdata\ from being overwritten
  SetOverwrite off
  
  SetOutPath "$INSTDIR\userdata"
  File /r /x *.so  "${xbmc_root}\Xbmc\userdata\*.*"
  
  ;Turn on overwrite for rest of install
  SetOverwrite on
  
  SetOutPath "$INSTDIR\addons"
  File /r "${xbmc_root}\Xbmc\addons\*.*"
  ;SetOutPath "$INSTDIR\web"
  ;File /r "${xbmc_root}\Xbmc\web\*.*"

  ;Store installation folder
  WriteRegStr HKCU "Software\XBMC" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  SetOutPath "$INSTDIR"
  
  ; delete old windowed link
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC (Windowed).lnk"
  
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\XBMC.lnk" "$INSTDIR\XBMC.exe" \
    "" "$INSTDIR\XBMC.exe" 0 SW_SHOWNORMAL \
    "" "Start XBMC."
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall XBMC.lnk" "$INSTDIR\Uninstall.exe" \
    "" "$INSTDIR\Uninstall.exe" 0 SW_SHOWNORMAL \
    "" "Uninstall XBMC."
  
  WriteINIStr "$SMPROGRAMS\$StartMenuFolder\Visit XBMC Online.url" "InternetShortcut" "URL" "http://xbmc.org"
  !insertmacro MUI_STARTMENU_WRITE_END  
  
  ;add entry to add/remove programs
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC" \
                 "DisplayName" "XBMC"
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
                 "HelpLink" "http://xbmc.org/support"
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
Section "Confluence" SecSkinConfluence
  SectionIn 1 2 #section is in installtype Full and Minimal
  SectionIn RO
  SetOutPath "$INSTDIR\addons\skin.confluence\"
  File /r "${xbmc_root}\Xbmc\addons\skin.confluence\*.*"
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
  LangString DESC_SecXBMC ${LANG_ENGLISH} "XBMC"

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
    !insertmacro MUI_HEADER_TEXT "Uninstall XBMC" "Remove XBMC's profile folder from your computer."
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

  SetShellVarContext current

  ;ADD YOUR OWN FILES HERE...
  Delete "$INSTDIR\XBMC.exe"
  Delete "$INSTDIR\copying.txt"
  Delete "$INSTDIR\known_issues.txt"
  Delete "$INSTDIR\LICENSE.GPL"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\xbmc.log"
  Delete "$INSTDIR\xbmc.old.log"
  Delete "$INSTDIR\python26.dll"
  Delete "$INSTDIR\libcdio-*.dll"
  Delete "$INSTDIR\libiconv-2.dll"
  RMDir /r "$INSTDIR\language"
  RMDir /r "$INSTDIR\media"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\scripts"
  RMDir /r "$INSTDIR\skin"
  RMDir /r "$INSTDIR\sounds"
  RMDir /r "$INSTDIR\system"
  RMDir /r "$INSTDIR\visualisations"
  RMDir /r "$INSTDIR\addons"
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
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC (Portable).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\XBMC (Windowed).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall XBMC.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Visit XBMC Online.url"
  RMDir "$SMPROGRAMS\$StartMenuFolder"  
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\XBMC"

  DeleteRegKey /ifempty HKCU "Software\XBMC"

SectionEnd

;--------------------------------
;vs redist installer Section

Section "Microsoft Visual C++ 2008/2010 Redistributable Package (x86)" SEC_VCREDIST

  SectionIn 1 2
  
  DetailPrint "Running VS Redist Setup..."

  ;vc90 for python
  SetOutPath "$TEMP\vc2008"
  File "${xbmc_root}\..\dependencies\vcredist\2008\vcredist_x86.exe"
  ExecWait '"$TEMP\vc2008\vcredist_x86.exe" /q' $VSRedistSetupError
  RMDir /r "$TEMP\vc2008"
  
  ;vc100
  SetOutPath "$TEMP\vc2010"
  File "${xbmc_root}\..\dependencies\vcredist\2010\vcredist_x86.exe"
  DetailPrint "Running VS Redist Setup..."
  ExecWait '"$TEMP\vc2010\vcredist_x86.exe" /q' $VSRedistSetupError
  RMDir /r "$TEMP\vc2010"
 
  DetailPrint "Finished VS Redist Setup"
  SetOutPath "$INSTDIR"
SectionEnd

;--------------------------------
;DirectX webinstaller Section

!if "${xbmc_target}" == "dx"
!define DXVERSIONDLL "$SYSDIR\D3DX9_43.dll"

Section "DirectX Install" SEC_DIRECTX
 
  SectionIn 1 2

  DetailPrint "Running DirectX Setup..."

  SetOutPath "$TEMP\dxsetup"
  File "${xbmc_root}\..\dependencies\dxsetup\dsetup32.dll"
  File "${xbmc_root}\..\dependencies\dxsetup\DSETUP.dll"
  File "${xbmc_root}\..\dependencies\dxsetup\dxdllreg_x86.cab"
  File "${xbmc_root}\..\dependencies\dxsetup\DXSETUP.exe"
  File "${xbmc_root}\..\dependencies\dxsetup\dxupdate.cab"
  File "${xbmc_root}\..\dependencies\dxsetup\Jun2010_D3DCompiler_43_x86.cab"
  File "${xbmc_root}\..\dependencies\dxsetup\Jun2010_d3dx9_43_x86.cab"
  ExecWait '"$TEMP\dxsetup\dxsetup.exe" /silent' $DirectXSetupError
  RMDir /r "$TEMP\dxsetup"
  SetOutPath "$INSTDIR"

  DetailPrint "Finished DirectX Setup"
  
SectionEnd

Section "-Check DirectX installation" SEC_DIRECTXCHECK

  IfFileExists ${DXVERSIONDLL} +2 0
    MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "DirectX9 wasn't installed properly.$\nPlease download the DirectX End-User Runtime from Microsoft and install it again."

SectionEnd

Function .onInit
  # set section 'SEC_DIRECTX' as selected and read-only if required dx version not found
  IfFileExists ${DXVERSIONDLL} +3 0
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${SEC_DIRECTX} $0
FunctionEnd
!endif