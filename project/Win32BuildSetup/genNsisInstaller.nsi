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

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${APP_NAME}"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\${APP_NAME}" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

  InstProgressFlags smooth
  
  ; Installer file properties
  VIProductVersion                   ${VERSION_NUMBER}
  VIAddVersionKey "ProductName"      "${APP_NAME}"
  VIAddVersionKey "Comments"         "This application and its source code are freely distributable."
  VIAddVersionKey "LegalCopyright"   "The trademark is owned by ${COMPANY_NAME}"
  VIAddVersionKey "CompanyName"      "${COMPANY_NAME}"
  VIAddVersionKey "FileDescription"  "${APP_NAME} ${VERSION_NUMBER} Setup"
  VIAddVersionKey "FileVersion"      "${VERSION_NUMBER}"
  VIAddVersionKey "ProductVersion"   "${VERSION_NUMBER}"
  VIAddVersionKey "LegalTrademarks"  "${APP_NAME}"
  ;VIAddVersionKey "OriginalFilename" "${APP_NAME}Setup-${app_revision}-${app_branch}.exe"

;--------------------------------
;Variables

  Var StartMenuFolder
  Var PageProfileState
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

Function CallbackDirLeave
  ;deinstall kodi if it is already there in destination folder
  Call HandleKodiInDestDir
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

  ;deinstall kodi in destination dir if $CleanDestDir == "1" - meaning user has confirmed it
  Call DeinstallKodiInDestDir

  ;Start copying files
  SetOutPath "$INSTDIR"
  File "${app_root}\application\*.*"
  SetOutPath "$INSTDIR\addons"
  File /r "${app_root}\application\addons\*.*"
  File /nonfatal /r "${app_root}\addons\peripheral.*"
  File /r "${app_root}\addons\skin.*"
  SetOutPath "$INSTDIR\media"
  File /r "${app_root}\application\media\*.*"
  SetOutPath "$INSTDIR\system"
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
                 "Publisher" "${COMPANY_NAME}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "HelpLink" "${WEBSITE}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
                 "URLInfoAbout" "${WEBSITE}"

SectionEnd

;*-addons.nsi are generated by genNsisIncludes.bat
!include /nonfatal "audiodecoder-addons.nsi"
!include /nonfatal "audioencoder-addons.nsi"
!include /nonfatal "audiodsp-addons.nsi"
!include /nonfatal "inputstream-addons.nsi"
!include /nonfatal "pvr-addons.nsi"
;!include /nonfatal "skin-addons.nsi"
!include /nonfatal "screensaver-addons.nsi"
!include /nonfatal "visualization-addons.nsi"

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecAPP ${LANG_ENGLISH} "${APP_NAME} ${VERSION_NUMBER}"

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

Section "VS2015 C++ re-distributable Package (x86)" SEC_VCREDIST1
DetailPrint "Running VS2015 re-distributable setup..."
  SectionIn 1 2 #section is in install type Full
  SetOutPath "$TEMP\vc2015"
  File "${app_root}\..\dependencies\vcredist\2015\vcredist_x86.exe"
  ExecWait '"$TEMP\vc2015\vcredist_x86.exe" /install /quiet /norestart' $VSRedistSetupError
  RMDir /r "$TEMP\vc2015"
  DetailPrint "Finished VS2015 re-distributable setup"
  SetOutPath "$INSTDIR"
SectionEnd

SectionGroupEnd

Function .onInit
  ${IfNot} ${AtLeastWinVista}
    MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "Windows Vista or above required.$\nThis program can not be run on Windows XP"
    Quit
  ${EndIf}
  StrCpy $CleanDestDir "-1"
FunctionEnd
