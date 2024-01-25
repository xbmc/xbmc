@ECHO OFF

PUSHD %~dp0\..\..\..
SET WORKSPACE=%CD%
POPD

ECHO Workspace is %WORKSPACE%

cd %WORKSPACE%
rem clean the BUILD_WIN32 at first to avoid problems with possible git files in there
IF EXIST %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 rmdir %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 /S /Q

rem also clean 'build' dir used to build ffmpeg as git clean has trouble to remove some times
IF EXIST %WORKSPACE%\project\BuildDependencies\build rmdir %WORKSPACE%\project\BuildDependencies\build /S /Q

rem daemonized executables block mingw from being cleaned with git
TASKKILL /IM "dirmngr.exe" /F >nul 2>&1
TASKKILL /IM "gpg-agent.exe" /F >nul 2>&1

rem we assume git in path as this is a requirement
rem git clean the untracked files and directories
rem but keep the downloaded dependencies
rem also keeps MSYS2 installation
SET GIT_CLEAN_CMD=git clean -xffd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2" -e "project/BuildDependencies/mingwlibs" -e "project/BuildDependencies/msys64" -e "project/BuildDependencies/tools" -e "cmake/addons/build/download/msys2-base-*.tar.xz"

ECHO running %GIT_CLEAN_CMD%
%GIT_CLEAN_CMD%
