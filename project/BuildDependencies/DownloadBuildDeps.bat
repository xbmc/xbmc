@ECHO OFF

SET CUR_PATH=%CD%
SET XBMC_PATH=%CD%\..\..
SET WGET=..\..\bin\wget
SET ZIP=..\..\..\Win32BuildSetup\tools\7z\7za

rmdir lib /S /Q
rmdir include /S /Q
md lib
md include

cd scripts

FOR /F "tokens=*" %%S IN ('dir /B "*_d.bat"') DO (
  echo running %%S ...
  CALL %%S
)

cd %CUR_PATH%