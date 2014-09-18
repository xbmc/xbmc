@ECHO OFF

ECHO Workspace is %WORKSPACE%

cd %WORKSPACE%
rem clean the BUILD_WIN32 at first to avoid problems with possible git files in there
IF EXIST %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 rmdir %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 /S /Q

rem we assume git in path as this is a requirement
rem git clean the untracked files and directories
rem but keep the downloaded dependencies
ECHO running git clean -xfd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2"
git clean -xfd -e "project/BuildDependencies/downloads" -e "project/BuildDependencies/downloads2"

rem cleaning additional directories
ECHO delete build directories
IF EXIST %WORKSPACE%\project\Win32BuildSetup\dependencies rmdir %WORKSPACE%\project\Win32BuildSetup\dependencies /S /Q

IF EXIST %WORKSPACE%\project\BuildDependencies\include rmdir %WORKSPACE%\project\BuildDependencies\include /S /Q
IF EXIST %WORKSPACE%\project\BuildDependencies\lib rmdir %WORKSPACE%\project\BuildDependencies\lib /S /Q
IF EXIST %WORKSPACE%\project\BuildDependencies\msys rmdir %WORKSPACE%\project\BuildDependencies\msys /S /Q

IF EXIST %WORKSPACE%\project\VS2010Express\XBMC rmdir %WORKSPACE%\project\VS2010Express\XBMC /S /Q
IF EXIST %WORKSPACE%\project\VS2010Express\objs rmdir %WORKSPACE%\project\VS2010Express\objs /S /Q
IF EXIST %WORKSPACE%\project\VS2010Express\libs rmdir %WORKSPACE%\project\VS2010Express\libs /S /Q

rem fetch submodules
IF EXIST "%WORKSPACE%\addons\skin.re-touched" rmdir "%WORKSPACE%\addons\skin.re-touched" /S /Q || exit /b 10
git submodule update --init "%WORKSPACE%\addons\skin.re-touched" || exit /b 30
