@ECHO OFF

SETLOCAL

SET CUR_PATH=%CD%
SET XBMC_PATH=%CD%\..\..
SET TMP_PATH=%CD%\scripts\tmp

rem can't run rmdir and md back to back. access denied error otherwise.
IF EXIST lib rmdir lib /S /Q || EXIT /B 6
IF EXIST include rmdir include /S /Q || EXIT /B 6
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q || EXIT /B 6

IF $%1$ == $$ (
  SET DL_PATH="%CD%\downloads"
) ELSE (
  SET DL_PATH="%1"
)

SET WGET=%CUR_PATH%\bin\wget
SET ZIP=%CUR_PATH%\..\Win32BuildSetup\tools\7z\7za

IF NOT EXIST %DL_PATH% md %DL_PATH% || EXIT /B 3

md lib || EXIT /B 3
md include || EXIT /B 3
md %TMP_PATH% || EXIT /B 3

cd scripts || EXIT /B 1

FOR /F "tokens=*" %%S IN ('dir /B "*_d.bat"') DO (
  echo running %%S ...
  CALL %%S || ( ECHO Error running %%S & EXIT /B 1 )
)

cd %CUR_PATH% || EXIT /B 1

rmdir %TMP_PATH% /S /Q || EXIT /B 6
