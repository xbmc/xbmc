@echo OFF

set NATIVE_CMAKE_GENERATOR=Visual Studio 15
set NATIVE_CMAKE_GENERATOR_PLATFORM=
set NATIVE_PLATFORM=

for /f %%i in ('wmic CPU get Architecture ^| findstr "^[0-9]"') do set architecture=%%i

if "%architecture%"=="9" (
	set NATIVE_CMAKE_GENERATOR_PLATFORM=x64
	set NATIVE_PLATFORM=x86_64-windows-native
) else (
	echo unsupported %architecture%
	exit 1
)

if "%Configuration%"=="Default" (
	set Configuration=RelWithDebInfo
)
