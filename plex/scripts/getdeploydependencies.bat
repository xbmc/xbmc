@echo off

if "%WORKSPACE%"=="" (
	set WORKSPACE="%~dp0..\.."
)

set DEPENDDIR="%WORKSPACE%\plex\build\dependencies"

if not exist %DEPENDDIR%\vcredist\2010 mkdir %DEPENDDIR%\vcredist\2010
if not exist %DEPENDDIR%\vcredist\2010\vcredist_x86.exe (
  echo Downloading vc100 redist...
  %WORKSPACE%\Project\BuildDependencies\bin\wget -nv -O %DEPENDDIR%\vcredist\2010\vcredist_x86.exe http://mirrors.xbmc.org/build-deps/win32/vcredist/2010/vcredist_x86.exe
)

if not exist %DEPENDDIR%\dxsetup mkdir %DEPENDDIR%\dxsetup
for %%f in (DSETUP.dll dsetup32.dll dxdllreg_x86.cab DXSETUP.exe dxupdate.cab Jun2010_D3DCompiler_43_x86.cab Jun2010_d3dx9_43_x86.cab) do (
  if not exist %DEPENDDIR%\dxsetup\%%f (
    %WORKSPACE%\Project\BuildDependencies\bin\wget -nv -O %DEPENDDIR%\dxsetup\%%f http://mirrors.xbmc.org/build-deps/win32/dxsetup/%%f
  )
)
