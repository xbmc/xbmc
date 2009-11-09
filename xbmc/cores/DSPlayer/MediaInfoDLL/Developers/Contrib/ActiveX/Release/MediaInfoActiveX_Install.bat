@echo off
set _WINSYS_=%windir%\SYSTEM
if exist %windir%\SYSTEM32\REGSVR32.EXE set _WINSYS_=%windir%\SYSTEM32
copy /y ..\..\MediaInfo.dll %_WINSYS_% >nul
copy /y MediaInfoActiveX.dll %_WINSYS_% >nul
%_WINSYS_%\regsvr32 /s %_WINSYS_%\MediaInfoActiveX.dll
set _WINSYS_=
echo Done.
