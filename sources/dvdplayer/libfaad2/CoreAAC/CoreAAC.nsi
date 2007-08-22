; ---------------------------------------------------------------------------
; CoreAAC install script for NSIS
; ---------------------------------------------------------------------------

!define NAME "CoreAAC Audio Decoder"
!define OUTFILE "Redist\CoreAAC.exe"
!define INPUT_PATH "Release\"
!define FILTER_FILE1 "CoreAAC.ax"
!define UNINST_NAME "CoreAAC-uninstall.exe"

; ---------------------------------------------------------------------------
; NOTE: this .NSI script is designed for NSIS v1.8+
; ---------------------------------------------------------------------------

Name "${NAME}"
OutFile "${OUTFILE}"

SetOverwrite ifnewer
SetCompress auto ; (can be off or force)
SetDatablockOptimize on ; (can be off)
CRCCheck on ; (can be off)
AutoCloseWindow false ; (can be true for the window go away automatically at end)
ShowInstDetails hide ; (can be show to have them shown, or nevershow to disable)
SetDateSave off ; (can be on to have files restored to their orginal date)

InstallColors /windows
InstProgressFlags smooth

; ---------------------------------------------------------------------------

Function .onInit
  MessageBox MB_YESNO "This will install ${NAME}. Do you wish to continue?" IDYES gogogo
    Abort
  gogogo:
FunctionEnd

; ---------------------------------------------------------------------------

Section "" ; (default section)
	SetOutPath "$SYSDIR"
	; add files / whatever that need to be installed here.
	File "${INPUT_PATH}${FILTER_FILE1}"

	; write out uninstaller
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME} (remove only)"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$SYSDIR\${UNINST_NAME}"'
	WriteUninstaller "$SYSDIR\${UNINST_NAME}"

	RegDLL "$SYSDIR\${FILTER_FILE1}"
SectionEnd ; end of default section

; ---------------------------------------------------------------------------

; begin uninstall settings/section
UninstallText "This will uninstall ${NAME} from your system"

Section Uninstall
	UnRegDLL "$SYSDIR\${FILTER_FILE1}"
	
	; add delete commands to delete whatever files/registry keys/etc you installed here.
	Delete /REBOOTOK "$SYSDIR\${FILTER_FILE1}"   
	Delete "$SYSDIR\${UNINST_NAME}"
   
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
SectionEnd ; end of uninstall section

; ---------------------------------------------------------------------------

Function un.onUninstSuccess
	IfRebootFlag 0 NoReboot
		MessageBox MB_OK \ 
			"A file couldn't be deleted. It will be deleted at next reboot."
	NoReboot:
FunctionEnd

; ---------------------------------------------------------------------------
; eof
; ---------------------------------------------------------------------------
