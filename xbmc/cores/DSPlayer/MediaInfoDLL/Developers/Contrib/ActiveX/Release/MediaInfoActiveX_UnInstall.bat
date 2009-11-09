@echo off
set _WINSYS_=%windir%\SYSTEM
if exist %windir%\SYSTEM32\REGSVR32.EXE set _WINSYS_=%windir%\SYSTEM32
%_WINSYS_%\regsvr32 /u /s %_WINSYS_%\MediaInfoActiveX.dll
del %_WINSYS_%\MediaInfoActiveX.dll
rem do not delete MediaInfo.dll, it might be used elsewhere; del %_WINSYS_%\MediaInfo.dll
set _WINSYS_=
echo Done.
