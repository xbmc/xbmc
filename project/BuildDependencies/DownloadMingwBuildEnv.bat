@ECHO OFF

SETLOCAL

REM If KODI_MIRROR is not set externally to this script, set it to the default mirror URL
IF "%KODI_MIRROR%" == "" SET KODI_MIRROR=http://mirrors.kodi.tv
echo Downloading from mirror %KODI_MIRROR%

REM Locate the BuildDependencies directory, based on the path of this script
SET BUILD_DEPS_PATH=%~dp0
SET APP_PATH=%BUILD_DEPS_PATH%\..\..
SET TMP_PATH=%BUILD_DEPS_PATH%\scripts\tmp

SET MSYS_INSTALL_PATH="%BUILD_DEPS_PATH%\msys"
SET MINGW_INSTALL_PATH="%BUILD_DEPS_PATH%\msys\mingw"


REM can't run rmdir and md back to back. access denied error otherwise.
IF EXIST %MSYS_INSTALL_PATH% rmdir %MSYS_INSTALL_PATH% /S /Q
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q

IF $%1$ == $$ (
  SET DL_PATH="%BUILD_DEPS_PATH%\downloads2"
) ELSE (
  SET DL_PATH="%1"
)

SET WGET=%BUILD_DEPS_PATH%\bin\wget
SET ZIP=%BUILD_DEPS_PATH%\..\Win32BuildSetup\tools\7z\7za

IF NOT EXIST %DL_PATH% md %DL_PATH%

IF NOT EXIST %MSYS_INSTALL_PATH% md %MSYS_INSTALL_PATH%
IF NOT EXIST %MINGW_INSTALL_PATH% md %MINGW_INSTALL_PATH%
IF NOT EXIST %TMP_PATH% md %TMP_PATH%

PUSHD %BUILD_DEPS_PATH%\scripts

CALL get_msys_env.bat
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q
CALL get_mingw_env.bat

POPD

REM update fstab to install path
SET FSTAB=%MINGW_INSTALL_PATH%
SET FSTAB=%FSTAB:\=/%
SET FSTAB=%FSTAB:"=%
ECHO %FSTAB% /mingw>>"%MSYS_INSTALL_PATH%\etc\fstab"
SET FSTAB=%APP_PATH%
SET FSTAB=%FSTAB:\=/%
SET FSTAB=%FSTAB:"=%
ECHO %FSTAB% /xbmc>>"%MSYS_INSTALL_PATH%\etc\fstab"

REM insert call to vsvars32.bat in msys.bat
PUSHD %MSYS_INSTALL_PATH%
Move msys.bat msys.bat_dist
ECHO CALL "%VS120COMNTOOLS%vsvars32.bat">>msys.bat
TYPE msys.bat_dist>>msys.bat

POPD

IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q
