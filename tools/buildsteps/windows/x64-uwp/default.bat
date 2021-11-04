@echo OFF

pushd %~dp0\..
call .helpers\default.bat
call vswhere.bat x64 store
if ERRORLEVEL 1 (
	echo ERROR! Something went wrong when calling vswhere.bat
	popd
	exit /B 1
)
popd

if "%Configuration%"=="" (
	set BUILD_TYPE=Debug
) else (
	set BUILD_TYPE=%Configuration%
)

set TARGET_CMAKE_GENERATOR=Visual Studio %vsver%
set TARGET_CMAKE_GENERATOR_PLATFORM=x64
set TARGET_CMAKE_OPTIONS=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%
set TARGET_ARCHITECTURE=x64
set TARGET_PLATFORM=x86_64-windowsstore-%BUILD_TYPE%
