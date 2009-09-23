; spyce.nsi
; Spyce Installer (NSIS script)

;#####################################
;VERSION

!define VERSION 1.3.13
!define RELEASE 1

;#####################################
;DEFINES

!define NAME Spyce
!define NAME_SMALL spyce
!define Desc "Spyce - Python Server Pages"
!define REG_PROG "SOFTWARE\${NAME}"
!define REG_UNINST "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
!define PYTHON "python.exe"
!define REG_PYTHONLOC "${REG_PROG}"

!define COMPILE 1

;#####################################
;OPTIONS

OutFile "${NAME_SMALL}-${VERSION}.exe"
InstallDir $PROGRAMFILES\${NAME_SMALL}
InstallDirRegKey HKLM ${REG_PROG} "location"

Name "${NAME}"
Caption "${NAME} Windows Installer"
UninstallCaption "${NAME} Windows Uninstaller"
DirText "${NAME} Windows Installer"
ComponentText  "${NAME} Windows Installer"
CompletedText "${NAME} Windows Installer is finished"
UninstallText "${NAME} Windows Uninstaller"
BrandingText " "

CRCCheck on
AutoCloseWindow true
EnabledBitmap  misc/one-check.bmp
DisabledBitmap  misc/one-nocheck.bmp
ShowInstDetails show
ShowUninstDetails show
;BGGradient 
SilentUnInstall silent
Icon misc\pics\spyce-border.ico ; MUST contain a 32x32x16 color icon
UninstallIcon misc\pics\spyce-border.ico
WindowIcon on
SetOverwrite on
SetCompress auto
SetDatablockOptimize on
SetDateSave off

;#####################################
;SECTIONS

Section "${NAME} engine"
  ReadRegStr $9 HKLM ${REG_PYTHONLOC} "python"
  SetOutPath $INSTDIR
  ; create and register uninstaller
  WriteUninstaller "uninstall.exe"
  WriteRegStr HKLM ${REG_PROG} "location" "$INSTDIR"
  WriteRegStr HKLM "${REG_UNINST}" "DisplayName" "${NAME}: ${DESC} (remove only)"
  WriteRegStr HKLM "${REG_UNINST}" "UninstallString" '"$INSTDIR\uninstall.exe"'

  ; copy spyce engine files
  File *.py
  File CHANGES LICENCE README THANKS spyceApache.conf spyce.conf.eg misc\pics\spyce.ico spyce.mime
  SetOutPath "$INSTDIR\modules"
  File modules\*.py
  SetOutPath "$INSTDIR\tags"
  File tags\*.py
  SetOutPath -
  ; pre-compile the sources
  !ifdef COMPILE
    DetailPrint "Compile Spyce sources."
    ExecWait `"$9" "$INSTDIR\spyceParser.py"`
    ExecWait `"$9" "$INSTDIR\installHelper.py" "--py=$INSTDIR"`
    ;ExecWait `"$9" -OO "$INSTDIR\installHelper.py" "--py=$INSTDIR"`
    ExecWait `"$9" "$INSTDIR\installHelper.py" "--py=$INSTDIR\modules"`
    ;ExecWait `"$9" -OO "$INSTDIR\installHelper.py" "--py=$INSTDIR\modules"`
    ExecWait `"$9" "$INSTDIR\installHelper.py" "--py=$INSTDIR\tags"`
    ;ExecWait `"$9" -OO "$INSTDIR\installHelper.py" "--py=$INSTDIR\tags"`
  !endif
SectionEnd

Section "${NAME} documentation"
  SectionIn RO
  ReadRegStr $9 HKLM ${REG_PYTHONLOC} "python"
  ; copy Spyce documentation files
  SetOutPath "$INSTDIR\docs"
  File docs\*.spy docs\*.gif
  SetOutPath "$INSTDIR\docs\examples"
  File docs\examples\*.spy docs\examples\*.spi docs\examples\*.tmpl docs\examples\*.py docs\examples\*.gif
  SetOutPath "$INSTDIR\docs\inc"
  File docs\inc\*.spi
  SetOutPath -
  ; compile documentation
  !ifdef COMPILE
    DetailPrint "Compile Spyce documentation."
    ExecWait `"$9" "$INSTDIR\run_spyceCmd.py" "-O" "$INSTDIR\docs\*.spy"`
  !endif
SectionEnd

SectionDivider "Options"

Section "Create start menu shortcuts"
  CreateDirectory "$SMPROGRAMS\${NAME}"
  CreateShortCut "$SMPROGRAMS\${NAME}\Spyce Documentation.lnk" "$INSTDIR\docs\index.html" "" "$INSTDIR\spyce.ico"
  CreateShortCut "$SMPROGRAMS\${NAME}\Spyce Documentation -- localhost.lnk" "http://localhost/spyce/" "" ""
  CreateShortCut "$SMPROGRAMS\${NAME}\Spyce Online.lnk" "http://spyce.sf.net/" "" ""
  CreateShortCut "$SMPROGRAMS\${NAME}\Spyce Examples.lnk" "$INSTDIR\docs\examples" "" "$INSTDIR\spyce.ico"
  CreateShortCut "$SMPROGRAMS\${NAME}\Uninstall Spyce.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Create shell extensions"
  WriteRegStr HKCR ".spy" "" "SpyceFile"
  WriteRegStr HKCR "SpyceFile" "" "Spyce dynamic HTML file"
  WriteRegStr HKCR "SpyceFile\DefaultIcon" "" $INSTDIR\spyce.ico,0
  WriteRegStr HKCR "SpyceFile\shell\open\command" "" 'notepad.exe "%1"'
  WriteRegStr HKCR "SpyceFile\shell\compile" "" "Compile Spyce"
  WriteRegStr HKCR "SpyceFile\shell\compile\command" "" '"$9" "$INSTDIR\run_spyceCmd.py" -O "%1"'
  WriteRegStr HKCR "SpyceFile\shell" "" "compile"
SectionEnd

Section "Configure Apache"
  DetailPrint "Configuring Apache..."
  ExecWait `"$9" "$INSTDIR\installHelper.py" "--apache=$INSTDIR"`
  ExecWait `"$9" "$INSTDIR\installHelper.py" "--apacheRestart"`
  MessageBox MB_OK|MB_ICONEXCLAMATION "$\nApache reconfigured and restarted.$\nIf everything is ok, you should be able to browse to: http://localhost/spyce/$\nIf not, please check your httpd.conf file and/or restart Apache."

SectionEnd


Section "Uninstall"
  ReadRegStr $9 HKLM ${REG_PYTHONLOC} "python"
  DetailPrint "Unconfiguring Apache..."
  ExecWait `"$9" "$INSTDIR\installHelper.py" "--apacheUN"`
  ExecWait `"$9" "$INSTDIR\installHelper.py" "--apacheRestart"`
  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\${NAME}"
  DeleteRegKey HKLM ${REG_UNINST}
  DeleteRegKey HKLM ${REG_PROG}
  DeleteRegKey HKCR ".spy"
  DeleteRegKey HKCR "SpyceFile"
SectionEnd

;#####################################
;FUNCTIONS

Function detectPython
  ; see if there is any python interpreter
  ClearErrors
  ExecShell "open" "${PYTHON}" `-c "print 'Python is alive!'"` SW_SHOWMINIMIZED
  IfErrors 0 NoAbort
    MessageBox MB_OK|MB_ICONEXCLAMATION "Unable to find Python interpreter. Please install Python first."
    Abort
  NoAbort:
  ; find out where it is
  GetTempFileName $9
  GetTempFileName $8
  FileOpen $7 $9 w
  FileWrite $7 'import sys$\n'
  FileWrite $7 "f=open(r'$8', 'w')$\n"
  FileWrite $7 'f.write(sys.executable)$\n'
  FileWrite $7 'f.close()$\n'
  FileClose $7
  ExecShell "open" "${PYTHON}" `"$9"` SW_SHOWMINIMIZED
  IntOp $0 0 + 0
  Loop:
    FileOpen $7 $8 r
    FileRead $7 $6
    FileClose $7
    StrCmp $6 "" 0 EndLoop
    Sleep 100
    IntOp $0 $0 + 1
    IntCmp $0 50 EndLoop
    Goto Loop
  EndLoop:
  Delete $9
  StrCpy $9 "$6"  ; put the python path in $9 -- GLOBAL
  StrCmp $9 "" 0 NoAbort2
    MessageBox MB_OK|MB_ICONEXCLAMATION "Mechanism for discovering Python path via sys.executable did not work.$\nSorry, but automatic installation is unable to proceed. Please contact the author."
    Abort
  NoAbort2:
  WriteRegStr HKLM ${REG_PYTHONLOC} "python" "$9"
FunctionEnd

Function .onInit
  Call detectPython
FunctionEnd

Function .onInstSuccess
  DetailPrint "Spyce successfully installed."
  MessageBox MB_OK "Spyce successfully installed."
  ExecShell "open" "$INSTDIR\docs\index.html" SW_SHOWMAXIMIZED
FunctionEnd

Function un.onInit
  MessageBox MB_YESNO|MB_ICONQUESTION "Are you sure that you want to uninstall Spyce?" IDYES NoAbort
    Abort
  NoAbort:
FunctionEnd

Function un.onUninstSuccess
  MessageBox MB_OK "Spyce successfully uninstalled."
FunctionEnd
