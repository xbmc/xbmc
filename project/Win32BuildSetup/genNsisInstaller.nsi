;Application for Windows install script
;Copyright (C) 2005-2013 Team XBMC
;http://xbmc.org

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "nsDialogs.nsh"
  !include "LogicLib.nsh"
  !include "WinVer.nsh"
  
;--------------------------------
;General

  ;Name and file
  Name "${APP_NAME}"
  OutFile "${APP_NAME}Setup-${app_revision}-${app_branch}.exe"

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
  Var /GLOBAL CleanDestDir
  
;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  !define MUI_ICON "..\..\tools\windows\packaging\media\application.ico"
  !define MUI_UNICON "..\..\tools\windows\packaging\media\application.ico"
  !define MUI_HEADERIMAGE_BITMAP "..\..\tools\windows\packaging\media\installer\header.bmp"
  !define MUI_HEADERIMAGE_UNBITMAP "..\..\tools\windows\packaging\media\installer\header.bmp"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "..\..\tools\windows\packaging\media\installer\welcome-left.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "..\..\tools\windows\packaging\media\installer\welcome-left.bmp"
  !define MUI_COMPONENTSPAGE_SMALLDESC
  !define MUI_FINISHPAGE_LINK "Please visit ${WEBSITE} for more information."
  !define MUI_FINISHPAGE_LINK_LOCATION "${WEBSITE}"
  !define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_NAME}.exe"
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_ABORTWARNING  
;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "..\..\LICENSE.GPL"
  !insertmacro MUI_PAGE_COMPONENTS
  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE CallbackDirLeave
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${APP_NAME}" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder  

  !insertmacro MUI_PAGE_INSTFILES
  !define MUI_PAGE_CUSTOMFUNCTION_PRE CallbackPreFinish
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
;HelperFunction
Function CallbackPreFinish 
  Var /GLOBAL ShouldMigrateUserData
  StrCpy $ShouldMigrateUserData "0"
  Var /GLOBAL OldXBMCInstallationFound
  StrCpy $OldXBMCInstallationFound "0"

  Call HandleOldXBMCInstallation
  ;Migrate userdata from XBMC to Kodi
  Call HandleUserdataMigration
FunctionEnd

Function CallbackDirLeave
  ;deinstall kodi if it is already there in destination folder
  Call HandleKodiInDestDir
FunctionEnd

Function HandleUserdataMigration
  Var /GLOBAL INSTDIR_XBMC
  ReadRegStr $INSTDIR_XBMC HKCU "Software\XBMC" ""

  ;Migration from XBMC to Kodi
  ;Move XBMC portable_data and appdata folder if exists to new location
  ${If} $ShouldMigrateUserData == "1"
      ${If} ${FileExists} "$APPDATA\XBMC\*.*"
      ${AndIfNot} ${FileExists} "$APPDATA\${APP_NAME}\*.*"
          Rename "$APPDATA\XBMC\" "$APPDATA\${APP_NAME}\"
          MessageBox MB_OK|MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND "Your current XBMC settings and library data were moved to the new ${APP_NAME} userdata location.$\nThis is to make the transition without any user interaction needed."
          ;mark that it was migrated in the filesystem - kodi will show another info message during first Kodi startup
          ;for really making sure that the user has read that message.
          FileOpen $0 "$APPDATA\${APP_NAME}\.kodi_data_was_migrated" w
          FileClose $0
      ${EndIf}
  ${Else}
    ; old installation was found but not uninstalled - inform the user
    ; that his userdata is not automatically migrted
    ${If} $OldXBMCInstallationFound == "1"
      MessageBox MB_OK|MB_ICONEXCLAMATION|MB_TOPMOST|MB_SETFOREGROUND "There was a former XBMC Installation detected but you didn't uninstall it. The older settings and library data will not be moved to the ${APP_NAME} userdata location. ${APP_NAME} will use the default settings."
    ${EndIf}
  ${EndIf}
FunctionEnd

Function HandleOldXBMCInstallation
  Var /GLOBAL INSTDIR_XBMC_OLD
  ReadRegStr $INSTDIR_XBMC_OLD HKCU "Software\XBMC" ""
  
  ;ask if a former XBMC installation should be uninstalled if detected
  ${IfNot} $INSTDIR_XBMC_OLD == ""
    StrCpy $OldXBMCInstallationFound "1"
    MessageBox MB_YESNO|MB_ICONQUESTION "You are upgrading from XBMC to ${APP_NAME}. Would you like to copy your settings and library data from XBMC to ${APP_NAME}?$\nWARNING: If you do so, XBMC will be completely un-installed and removed. It is recommended that you back up your library before continuing.$\nFor more information visit http://kodi.wiki/view/Userdata " IDYES true IDNO false
    true:
      DetailPrint "Uninstalling $INSTDIR_XBMC"
      SetDetailsPrint none
      ExecWait '"$INSTDIR_XBMC_OLD\uninstall.exe" /S _?=$INSTDIR_XBMC_OLD'
      SetDetailsPrint both
      ;this also removes the uninstall.exe which doesn't remove it self...
      Delete "$INSTDIR_XBMC_OLD\uninstall.exe"
      ;if the directory is now empty we can safely remove it (rmdir won't remove non-empty dirs!)
      RmDir "$INSTDIR_XBMC_OLD"
      StrCpy $ShouldMigrateUserData "1"
    false:
  ${EndIf}
FunctionEnd

Function HandleOldKodiInstallation
  Var /GLOBAL INSTDIR_KODI
  ReadRegStr $INSTDIR_KODI HKCU "Software\${APP_NAME}" ""

  ;if former Kodi installation was detected in a different directory then the destination dir
  ;ask for uninstallation
  ;only ask about the other installation if user didn't already
  ;decide to not overwrite the installation in his originally selected destination dir
  ${IfNot}    $CleanDestDir == "0"
  ${AndIfNot} $INSTDIR_KODI == ""
  ${AndIfNot} $INSTDIR_KODI == $INSTDIR
    MessageBox MB_YESNO|MB_ICONQUESTION  "A previous ${APP_NAME} installation in a different folder was detected. Would you like to uninstall it?$\nYour current settings and library data will be kept intact." IDYES true IDNO false
    true:
      DetailPrint "Uninstalling $INSTDIR_KODI"
      SetDetailsPrint none
      ExecWait '"$INSTDIR_KODI\uninstall.exe" /S _?=$INSTDIR_KODI'
      SetDetailsPrint both
      ;this also removes the uninstall.exe which doesn't remove it self...
      Delete "$INSTDIR_KODI\uninstall.exe"
      ;if the directory is now empty we can safely remove it (rmdir won't remove non-empty dirs!)
      RmDir "$INSTDIR_KODI"
    false:
  ${EndIf}
FunctionEnd

Function HandleKodiInDestDir
  ;if former Kodi installation was detected in the destination directory - uninstall it first
  ${IfNot} $INSTDIR == ""
  ${AndIf} ${FileExists} "$INSTDIR\uninstall.exe"
    MessageBox MB_YESNO|MB_ICONQUESTION  "A previous installation was detected in the selected destination folder. Do you really want to overwrite it?$\nYour settings and library data will be kept intact." IDYES true IDNO false
    true:
      StrCpy $CleanDestDir "1"
      Goto done
    false:
      StrCpy $CleanDestDir "0"
      Abort
    done:
  ${EndIf}
FunctionEnd

Function DeinstallKodiInDestDir
  ${If} $CleanDestDir == "1"
    DetailPrint "Uninstalling former ${APP_NAME} Installation in $INSTDIR"
    SetDetailsPrint none
    ExecWait '"$INSTDIR\uninstall.exe" /S _?=$INSTDIR'
    SetDetailsPrint both
    ;this also removes the uninstall.exe which doesn't remove it self...
    Delete "$INSTDIR\uninstall.exe"
  ${EndIf}
FunctionEnd

;--------------------------------
;Install levels

InstType "Full"    ; 1.
InstType "Normal"  ; 2.
InstType "Minimal" ; 3.

;--------------------------------
;Installer Sections

Section "${APP_NAME}" SecAPP
  SetShellVarContext current
  SectionIn RO
  SectionIn 1 2 3 #section is in install type Normal/Full/Minimal

  ;handle an old kodi installation in a folder different from the destination folder
  Call HandleOldKodiInstallation
  ;deinstall kodi in destination dir if $CleanDestDir == "1" - meaning user has confirmed it
  Call DeinstallKodiInDestDir

  ;Start copying files
  SetOutPath "$INSTDIR"
  File "${app_root}\application\*.*"
  SetOutPath "$INSTDIR\addons"
  File /r "${app_root}\application\addons\*.*"
  SetOutPath "$INSTDIR\media"
  File /r "${app_root}\application\media\*.*"
  SetOutPath "$INSTDIR\system"
  ; remove leftover from old Kodi installation
  ${If} ${FileExists} "$INSTDIR\system\webserver"
    RMDir /r "$INSTDIR\system\webserver"
  ${EndIf}
  File /r "${app_root}\application\system\*.*"
  SetOutPath "$INSTDIR\userdata"
  File /r "${app_root}\application\userdata\*.*"

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
  
  WriteINIStr "$SMPROGRAMS\$StartMenuFolder\Visit ${APP_NAME} Online.url" "InternetShortcut" "URL" "${WEBSITE}"
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
                 "Publisher" "${COMPANY}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "HelpLink" "${WEBSITE}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "URLInfoAbout" "${WEBSITE}"

SectionEnd

;*-addons.nsi are generated by genNsisIncludes.bat
!include /nonfatal "audiodecoder-addons.nsi"
!include /nonfatal "audioencoder-addons.nsi"
!include /nonfatal "pvr-addons.nsi"
!include /nonfatal "skin-addons.nsi"

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecAPP ${LANG_ENGLISH} "${APP_NAME}"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAPP} $(DESC_SecAPP)
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

  ${NSD_CreateLabel} 0 0 100% 12u "Do you want to delete the profile folder which contains your ${APP_NAME} settings and library data?"
  Pop $0

  ${NSD_CreateText} 0 13u 100% 12u "$APPDATA\${APP_NAME}\"
  Pop $UnPageProfileEditBox
    SendMessage $UnPageProfileEditBox ${EM_SETREADONLY} 1 0

  ${NSD_CreateLabel} 0 30u 100% 24u "Leave the option box below unchecked to keep the profile folder which contains ${APP_NAME}'s settings and library data for later use. If you are sure you want to delete the profile folder you may check the option box.$\nWARNING: Deletion of the profile folder cannot be undone and you will lose all settings and library data."
  Pop $0

  ${NSD_CreateCheckbox} 0 71u 100% 8u "Yes, I am sure and grant permission to also delete the profile folder."
  Pop $UnPageProfileCheckbox

  nsDialogs::Show
FunctionEnd

Function un.UnPageProfileLeave
${NSD_GetState} $UnPageProfileCheckbox $UnPageProfileCheckbox_State
FunctionEnd

Section "Uninstall"

  SetShellVarContext current

  ;ADD YOUR OWN FILES HERE...
  RMDir /r "$INSTDIR\addons"
  RMDir /r "$INSTDIR\language"
  RMDir /r "$INSTDIR\media"
  RMDir /r "$INSTDIR\system"
  RMDir /r "$INSTDIR\userdata"
  Delete "$INSTDIR\*.*"
  
  ;Un-install User Data if option is checked, otherwise skip
  ${If} $UnPageProfileCheckbox_State == ${BST_CHECKED}
    RMDir /r "$APPDATA\${APP_NAME}\"
    RMDir /r "$INSTDIR\portable_data\"
  ${EndIf}
  RMDir "$INSTDIR"

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
SectionGroup "Microsoft Visual C++ packages" SEC_VCREDIST
Section "VS2008 C++ re-distributable Package (x86)" SEC_VCREDIST1
  ;vc90 for python
  DetailPrint "Running VS2008 re-distributable setup..."
  SectionIn 1 2 #section is in install type Full 
  SetOutPath "$TEMP\vc2008"
  File "${app_root}\..\dependencies\vcredist\2008\vcredist_x86.exe"
  ExecWait '"$TEMP\vc2008\vcredist_x86.exe" /q' $VSRedistSetupError
  RMDir /r "$TEMP\vc2008"
  DetailPrint "Finished VS2008 re-distributable setup"
SectionEnd

Section "VS2010 C++ re-distributable Package (x86)" SEC_VCREDIST2
  DetailPrint "Running VS2010 re-distributable setup..."
  SectionIn 1 2 #section is in install type Full 
  SetOutPath "$TEMP\vc2010"
  File "${app_root}\..\dependencies\vcredist\2010\vcredist_x86.exe"
  ExecWait '"$TEMP\vc2010\vcredist_x86.exe" /q' $VSRedistSetupError
  RMDir /r "$TEMP\vc2010"
  DetailPrint "Finished VS2010 re-distributable setup"
SectionEnd

Section "VS2013 C++ re-distributable Package (x86)" SEC_VCREDIST3
DetailPrint "Running VS2013 re-distributable setup..."
  SectionIn 1 2 #section is in install type Full
  SetOutPath "$TEMP\vc2013"
  File "${app_root}\..\dependencies\vcredist\2013\vcredist_x86.exe"
  ExecWait '"$TEMP\vc2013\vcredist_x86.exe" /q' $VSRedistSetupError
  RMDir /r "$TEMP\vc2013"
  DetailPrint "Finished VS2013 re-distributable setup"
  SetOutPath "$INSTDIR"
SectionEnd

SectionGroupEnd

;--------------------------------
;DirectX web-installer Section
!define DXVERSIONDLL "$SYSDIR\D3DX9_43.dll"
Section "DirectX Install" SEC_DIRECTX
  SectionIn 1 2 #section is in install type Full/Normal and when not installed
  DetailPrint "Running DirectX Setup..."
  SetOutPath "$TEMP\dxsetup"
  File "${app_root}\..\dependencies\dxsetup\*.*"
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
  StrCpy $CleanDestDir "-1"
FunctionEnd
