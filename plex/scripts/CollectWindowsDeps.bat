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
call %WORKSPACE%\plex\scripts\buildmingwlibs.bat

rd /s /q %WORKSPACE%\upload
md %WORKSPACE%\upload

for /r %WORKSPACE%\system %%d in (*.dll) do (
	rem %WORKSPACE%\plex\scripts\WindowsSign.cmd %%d
)

..\..\project\BuildDependencies\msys\bin\sh --login /xbmc/plex/scripts/packdeps.sh 