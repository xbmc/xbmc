;XBMC for Windows install script
;Copyright (C) 2005-2013 Team XBMC
;http://xbmc.org

;Script by chadoe

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "nsDialogs.nsh"
  !include "LogicLib.nsh"
  !include "WinVer.nsh"
  
;--------------------------------
;define global used name
  !define APP_NAME "XBMC"

;--------------------------------
;General

  ;Name and file
  Name "${APP_NAME}"
  OutFile "${APP_NAME}Setup-${xbmc_revision}-${xbmc_branch}.exe"

  XPStyle on
  
  ;Default installation folder
  InstallDir "$PROGRAMFILES\${APP_NAME}"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\${APP_NAME}" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Variables

  Var StartMenuFolder
  Var PageProfileState
  Var DirectXSetupError
  Var VSRedistSetupError
  
;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  !define MUI_ICON "..\..\tools\windows\packaging\media\xbmc.ico"
  !define MUI_HEADERIMAGE_BITMAP "..\..\tools\windows\packaging\media\installer\header.bmp"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "..\..\tools\windows\packaging\media\installer\welcome-left.bmp"
  !define MUI_COMPONENTSPAGE_SMALLDESC
  !define MUI_FINISHPAGE_LINK "Please visit http://xbmc.org for more information."
  !define MUI_FINISHPAGE_LINK_LOCATION "http://xbmc.org"
  !define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_NAME}.exe"
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
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${APP_NAME}" 
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
;Install levels

InstType "Full"    ; 1.
InstType "Normal"  ; 2.
InstType "Minimal" ; 3.

;--------------------------------
;Installer Sections

Section "XBMC" SecXBMC
  SetShellVarContext current
  SectionIn RO
  SectionIn 1 2 3 #section is in install type Normal/Full/Minimal
  ;ADD YOUR OWN FILES HERE...
  SetOutPath "$INSTDIR"
  File "${xbmc_root}\Xbmc\XBMC.exe"
  File "${xbmc_root}\Xbmc\copying.txt"
  File "${xbmc_root}\Xbmc\LICENSE.GPL"
  File "${xbmc_root}\Xbmc\*.dll"
  SetOutPath "$INSTDIR\language"
  File /r /x *.so "${xbmc_root}\Xbmc\language\*.*"
  SetOutPath "$INSTDIR\media"
  File /r /x *.so "${xbmc_root}\Xbmc\media\*.*"
  SetOutPath "$INSTDIR\sounds"
  File /r /x *.so "${xbmc_root}\Xbmc\sounds\*.*"

  RMDir /r $INSTDIR\addons
  SetOutPath "$INSTDIR\addons"
  File /r /x skin.touched ${xbmc_root}\Xbmc\addons\*.*

  ; delete system/python if its there
  SetOutPath "$INSTDIR\system"
  IfFileExists $INSTDIR\system\python 0 +2
    RMDir /r $INSTDIR\system\python
  
  File /r /x *.so /x *_d.* /x tcl85g.dll /x tclpip85g.dll /x tk85g.dll "${xbmc_root}\Xbmc\system\*.*"
  
  ; delete  msvc?90.dll's in INSTDIR, we use the vcredist installer later
  Delete "$INSTDIR\msvcr90.dll"
  Delete "$INSTDIR\msvcp90.dll"
  
  ;Turn off overwrite to prevent files in xbmc\userdata\ from being overwritten
  SetOverwrite off
  
  SetOutPath "$INSTDIR\userdata"
  File /r /x *.so  "${xbmc_root}\Xbmc\userdata\*.*"
  
  ;Turn on overwrite for rest of install
  SetOverwrite on

  ;Store installation folder
  WriteRegStr HKCU "Software\${APP_NAME}" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  SetOutPath "$INSTDIR"
  
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk" "$INSTDIR\${APP_NAME}.exe" \
    "" "$INSTDIR\${APP_NAME}.exe" 0 SW_SHOWNORMAL \
    "" "Start ${APP_NAME}."
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall ${APP_NAME}.lnk" "$INSTDIR\Uninstall.exe" \
    "" "$INSTDIR\Uninstall.exe" 0 SW_SHOWNORMAL \
    "" "Uninstall ${APP_NAME}."
  
  WriteINIStr "$SMPROGRAMS\$StartMenuFolder\Visit ${APP_NAME} Online.url" "InternetShortcut" "URL" "http://xbmc.org"
  !insertmacro MUI_STARTMENU_WRITE_END  
  
  ;add entry to add/remove programs
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "DisplayName" "${APP_NAME}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "NoRepair" 1
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "DisplayIcon" "$INSTDIR\${APP_NAME}.exe,0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "Publisher" "Team XBMC"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "HelpLink" "http://xbmc.org/support"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "URLInfoAbout" "http://xbmc.org"
                 
SectionEnd

SectionGroup "Skins" SecSkins
Section "Confluence" SecSkinConfluence
  SectionIn 1 2 3 #section is in install type Full/Normal/Minimal
  SectionIn RO
  SetOutPath "$INSTDIR\addons\skin.confluence\"
  File /r "${xbmc_root}\Xbmc\addons\skin.confluence\*.*"
SectionEnd
;skins.nsi is generated by genNsisIncludes.bat
!include /nonfatal "skins.nsi"
SectionGroupEnd

SectionGroup "PVR Addons" SecPvrAddons
;xbmc-pvr-addons.nsi is generated by genNsisIncludes.bat
!include /nonfatal "xbmc-pvr-addons.nsi"
SectionGroupEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecXBMC ${LANG_ENGLISH} "${APP_NAME}"

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
    !insertmacro MUI_HEADER_TEXT "Uninstall ${APP_NAME}" "Remove ${APP_NAME}'s profile folder from your computer."
  nsDialogs::Create /NOUNLOAD 1018
  Pop $UnPageProfileDialog

  ${If} $UnPageProfileDialog == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 12u "Do you want to delete the profile folder?"
  Pop $0

  ${NSD_CreateText} 0 13u 100% 12u "$APPDATA\${APP_NAME}\"
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
  Delete "$INSTDIR\${APP_NAME}.exe"
  Delete "$INSTDIR\copying.txt"
  Delete "$INSTDIR\known_issues.txt"
  Delete "$INSTDIR\LICENSE.GPL"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\xbmc.log"
  Delete "$INSTDIR\xbmc.old.log"
  Delete "$INSTDIR\python26.dll"
  Delete "$INSTDIR\python27.dll"
  Delete "$INSTDIR\libcdio-*.dll"
  Delete "$INSTDIR\libiconv-2.dll"
  Delete "$INSTDIR\libxml2.dll"
  Delete "$INSTDIR\iconv.dll"
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
    RMDir /r "$APPDATA\${APP_NAME}\"
  ${Else}
;Even if userdata is kept in %appdata%\xbmc\userdata, the $INSTDIR\userdata should be cleaned up on uninstall if not used
;If guisettings.xml exists in the XBMC\userdata directory, do not delete XBMC\userdata directory
;If that file does not exists, then delete that folder and $INSTDIR
    IfFileExists $INSTDIR\userdata\guisettings.xml +3
      RMDir /r "$INSTDIR\userdata"  
      RMDir "$INSTDIR"
  ${EndIf}

  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall ${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Visit ${APP_NAME} Online.url"
  RMDir "$SMPROGRAMS\$StartMenuFolder"  
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

  DeleteRegKey /ifempty HKCU "Software\${APP_NAME}"

SectionEnd

;--------------------------------
;vs redist installer Section

Section "Microsoft Visual C++ 2008/2010 Redistributable Package (x86)" SEC_VCREDIST

  SectionIn 1 2 #section is in install type Full/Normal and when not installed
  
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
 
  SectionIn 1 2 #section is in install type Full/Normal and when not installed

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
  ${IfNot} ${AtLeastWinVista}
    MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Windows Vista or above required.$\nThis program can not be run on Windows XP"
    Quit
  ${EndIf}
  # set section 'SEC_DIRECTX' as selected and read-only if required dx version not found
  IfFileExists ${DXVERSIONDLL} +3 0
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${SEC_DIRECTX} $0
FunctionEnd
!endif