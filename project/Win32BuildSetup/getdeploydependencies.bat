@echo off

if not exist dependencies\vcredist\2008 mkdir dependencies\vcredist\2008
if not exist dependencies\vcredist\2010 mkdir dependencies\vcredist\2010
if not exist dependencies\vcredist\2013 mkdir dependencies\vcredist\2013
if not exist dependencies\vcredist\2008\vcredist_x86.exe (
  echo Downloading vc90 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2008\vcredist_x86.exe http://mirrors.xbmc.org/build-deps/win32/vcredist/2008/vcredist_x86.exe
)
if not exist dependencies\vcredist\2010\vcredist_x86.exe (
  echo Downloading vc100 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2010\vcredist_x86.exe http://mirrors.xbmc.org/build-deps/win32/vcredist/2010/vcredist_x86.exe
)
if not exist dependencies\vcredist\2013\vcredist_x86.exe (
  echo Downloading vc120 redist...
  ..\BuildDependencies\bin\wget -nv -O dependencies\vcredist\2013\vcredist_x86.exe http://mirrors.xbmc.org/build-deps/win32/vcredist/2013/vcredist_x86.exe
)

if not exist dependencies\dxsetup mkdir dependencies\dxsetup
for %%f in (DSETUP.dll dsetup32.dll dxdllreg_x86.cab DXSETUP.exe dxupdate.cab Jun2010_D3DCompiler_43_x86.cab Jun2010_d3dx9_43_x86.cab) do (
  if not exist dependencies\dxsetup\%%f (
    ..\BuildDependencies\bin\wget -nv -O dependencies\dxsetup\%%f http://mirrors.xbmc.org/build-deps/win32/dxsetup/%%f
  )
)
