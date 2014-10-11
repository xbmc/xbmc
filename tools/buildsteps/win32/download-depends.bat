@ECHO OFF

SETLOCAL

SET CUR_PATH=%WORKSPACE%\project\BuildDependencies
SET TMP_PATH=%WORKSPACE%\project\BuildDependencies\scripts\tmp
SET APP_PATH=%WORKSPACE%

cd %CUR_PATH%

rem can't run rmdir and md back to back. access denied error otherwise.
IF EXIST lib rmdir lib /S /Q
IF EXIST include rmdir include /S /Q
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q

IF $%1$ == $$ (
  SET DL_PATH="%CD%\downloads"
) ELSE (
  SET DL_PATH="%1"
)

SET WGET=%CUR_PATH%\bin\wget
SET ZIP=%CUR_PATH%\..\Win32BuildSetup\tools\7z\7za

IF NOT EXIST %DL_PATH% md %DL_PATH%

md lib
md include
md %TMP_PATH%

cd scripts

FOR /F "tokens=*" %%S IN ('dir /B "*_d.bat"') DO (
  echo running %%S ...
  CALL %%S
)

cd %CUR_PATH%

rmdir %TMP_PATH% /S /Q