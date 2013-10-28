@ECHO OFF

ECHO Workspace is %WORKSPACE%

rem git clean the untracked files but not the directories
rem to keep the downloaded dependencies
rem we assume git in path as this is a requirement

cd %WORKSPACE%
ECHO running git clean -xf
git clean -xf

rem cleaning additional directories
ECHO delete build directories
IF EXIST %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 rmdir %WORKSPACE%\project\Win32BuildSetup\BUILD_WIN32 /S /Q
IF EXIST %WORKSPACE%\project\Win32BuildSetup\dependencies rmdir %WORKSPACE%\project\Win32BuildSetup\dependencies /S /Q

IF EXIST %WORKSPACE%\project\BuildDependencies\include rmdir %WORKSPACE%\project\BuildDependencies\include /S /Q
IF EXIST %WORKSPACE%\project\BuildDependencies\lib rmdir %WORKSPACE%\project\BuildDependencies\lib /S /Q
IF EXIST %WORKSPACE%\project\BuildDependencies\msys rmdir %WORKSPACE%\project\BuildDependencies\msys /S /Q

IF EXIST %WORKSPACE%\project\VS2010Express\XBMC rmdir %WORKSPACE%\project\VS2010Express\XBMC /S /Q
IF EXIST %WORKSPACE%\project\VS2010Express\objs rmdir %WORKSPACE%\project\VS2010Express\objs /S /Q
IF EXIST %WORKSPACE%\project\VS2010Express\libs rmdir %WORKSPACE%\project\VS2010Express\libs /S /Q