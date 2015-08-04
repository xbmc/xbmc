@echo off

REM If KODI_MIRROR is not set externally to this script, set it to the default mirror URL
IF "%KODI_MIRROR%" == "" SET KODI_MIRROR=http://mirrors.kodi.tv
echo Downloading from mirror %KODI_MIRROR%


:: Following commands expect this script's parent directory to be the current directory, so make sure that's so
PUSHD %~dp0

if not exist dependencies\vcredist\2008 mkdir dependencies\vcredist\2008
if not exist dependencies\vcredist\2010 mkdir dependencies\vcredist\2010
if not exist dependencies\vcredist\2013 mkdir dependencies\vcredist\2013
if not exist dependencies\vcredist\2008\vcredist_x86.exe (
  echo Downloading vc90 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2008\vcredist_x86.exe %KODI_MIRROR%/build-deps/win32/vcredist/2008/vcredist_x86.exe
)
if not exist dependencies\vcredist\2010\vcredist_x86.exe (
  echo Downloading vc100 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2010\vcredist_x86.exe %KODI_MIRROR%/build-deps/win32/vcredist/2010/vcredist_x86.exe
)
if not exist dependencies\vcredist\2013\vcredist_x86.exe (
  echo Downloading vc120 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2013\vcredist_x86.exe %KODI_MIRROR%/build-deps/win32/vcredist/2013/vcredist_x86.exe
)

:: Restore the previous current directory
POPD
