@echo off

if not exist dependencies\vcredist\2008 mkdir dependencies\vcredist\2008
if not exist dependencies\vcredist\2010 mkdir dependencies\vcredist\2010
if not exist dependencies\vcredist\2008\vcredist_x86.exe (
  echo Downloading vc90 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2008\vcredist_x86.exe http://mirrors.xbmc.org/build-deps/win32/vcredist/2008/vcredist_x86.exe
)
if not exist dependencies\vcredist\2010\vcredist_x86.exe (
  echo Downloading vc100 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2010\vcredist_x86.exe http://mirrors.xbmc.org/build-deps/win32/vcredist/2010/vcredist_x86.exe
)

