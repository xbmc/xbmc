@echo off

if "%WORKSPACE%"=="" (
	rem if we don't have a workspace let's just assume we are being executed from plex\scripts
	set WORKSPACE="%~dp0..\.."
)

echo Using %WORKSPACE%

rem start by downloading what we need.
cd %WORKSPACE%\Project\BuildDependencies
call DownloadBuildDeps.bat
call DownloadMingwBuildEnv.bat
cd %WORKSPACE%\Project\Win32BuildSetup
set NOPROMPT=1
call buildmingwlibs.bat

rem alright, done already, let's build the actual Plex binary
cd %WORKSPACE%\plex\build
rd /s /q windows-release-build
mkdir windows-release-build
cd windows-release-build
echo "running CMake"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=output -GNinja %WORKSPACE%
echo "running ninja"
ninja package