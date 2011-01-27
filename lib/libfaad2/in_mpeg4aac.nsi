Name "AudioCoding.com MP4 Winamp plugin"
OutFile in_mp4.exe
CRCCheck on
LicenseText "You must read the following license before installing."
LicenseData COPYING
ComponentText "This will install the AudioCoding.com MP4 Winamp plugin on your computer."
InstType Normal
AutoCloseWindow true
SetOverwrite on
SetDateSave on

InstallDir $PROGRAMFILES\Winamp
InstallDirRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" "UninstallString"
;DirShow
DirText "The installer has detected the path to Winamp. If it is not correct, please change."

Section "AudioCoding.com MP4 Winamp plugin"
SectionIn 1
SetOutPath $INSTDIR\Plugins
File plugins\in_mp4\Release\in_mp4.dll
SectionEnd
