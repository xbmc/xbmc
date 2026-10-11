@ECHO OFF

SETLOCAL

PUSHD %~dp0\..\..\..
SET WORKSPACE=%CD%
POPD

SET TARGETPLATFORM=%1
SET NATIVEPLATFORM=%2

REM Build tools location. We may want to add an extra folder for HOST ARCH
REM if we want end up having build tools other than win32 arch
SET HOST_BUILDTOOLS=tools

IF "%TARGETPLATFORM%" == "" SET TARGETPLATFORM=win32
IF "%NATIVEPLATFORM%" == "" SET NATIVEPLATFORM=win32

ECHO TARGETPLATFORM: %TARGETPLATFORM%
ECHO NATIVEPLATFORM: %NATIVEPLATFORM%

REM If KODI_MIRROR is not set externally to this script, set it to the default mirror URL
IF "%KODI_MIRROR%" == "" SET KODI_MIRROR=https://mirrors.kodi.tv
echo Downloading from mirror %KODI_MIRROR%

REM Locate the BuildDependencies directory, based on the path of this script
SET BUILD_DEPS_PATH=%WORKSPACE%\project\BuildDependencies
SET APP_PATH=%WORKSPACE%\project\BuildDependencies\%TARGETPLATFORM%
SET NATIVE_PATH=%WORKSPACE%\project\BuildDependencies\%HOST_BUILDTOOLS%
SET TMP_PATH=%BUILD_DEPS_PATH%\scripts\tmp

REM Clean dependencies path (install path) to avoid Debug vs Release conflicts
IF EXIST %APP_PATH% rmdir %APP_PATH% /S /Q

REM Restore the dependencies a successful build left for the same inputs, see prepare-env.bat.
REM The bundled libraries are built with /GL, so the Visual Studio version is part of the hash.
SET BUILD_CACHE_DIR=%WORKSPACE%\.build-cache
IF "%BUILD_CACHE_ENTRIES%" == "" SET BUILD_CACHE_ENTRIES=2
SET FORMED_TARGET_RESTORED=NO
CALL :getBuildHash
IF NOT "%BUILD_CACHE_ENTRIES%" == "0" IF EXIST "%BUILD_CACHE_DIR%\%BUILD_HASH%\%TARGETPLATFORM%" CALL :restoreCachedBuild
IF EXIST "%BUILD_CACHE_DIR%" PowerShell -NoProfile -Command "Get-ChildItem -LiteralPath '%BUILD_CACHE_DIR%' -Directory | Sort-Object CreationTime -Descending | Select-Object -Skip %BUILD_CACHE_ENTRIES% | ForEach-Object { cmd /c rmdir /S /Q $_.FullName }"

REM Change to the BuildDependencies directory, if we're not there already
PUSHD %BUILD_DEPS_PATH%

REM Can't run rmdir and md back to back. access denied error otherwise.
IF EXIST %TMP_PATH% rmdir %TMP_PATH% /S /Q

SET DL_PATH="%BUILD_DEPS_PATH%\downloads"
SET ZIP=%BUILD_DEPS_PATH%\..\Win32BuildSetup\tools\7z\7za

IF NOT EXIST %DL_PATH% md %DL_PATH%

md %TMP_PATH%

cd scripts

SET FORMED_OK_FLAG=%TMP_PATH%\got-all-formed-packages
REM Trick to preserve console title
start /b /wait cmd.exe /c get_formed.cmd
IF NOT EXIST %FORMED_OK_FLAG% (
  ECHO ERROR: Not all formed packages are ready!
  ECHO.
  ECHO I tried to get the packages from %KODI_MIRROR%;
  ECHO if this download mirror seems to be having problems, try choosing another from
  ECHO the list on https://mirrors.kodi.tv/timestamp.txt?mirrorlist, and setting %%KODI_MIRROR%% to
  ECHO point to it, like so:
  ECHO   C:\^> SET KODI_MIRROR=https://example.com/pub/xbmc/
  ECHO.
  ECHO Then, rerun this script.
  
  REM Restore the previous current directory
  POPD

  ENDLOCAL
  
  EXIT /B 101
)

rmdir %TMP_PATH% /S /Q

REM BuildSetup.bat marks it complete once the build has installed the bundled libraries
IF "%FORMED_TARGET_RESTORED%" == "NO" ECHO %BUILD_HASH%> "%APP_PATH%\.build-hash.pending"

REM Restore the previous current directory
POPD

ENDLOCAL

EXIT /B 0

:getBuildHash
SET VSWHERE_ARGS=-latest -property installationVersion
IF "%prerelease%" == "true" SET VSWHERE_ARGS=%VSWHERE_ARGS% -prerelease
FOR /F "usebackq delims=" %%v IN (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" %VSWHERE_ARGS%`) DO SET VS_VERSION=%%v
FOR /F %%r IN ('git -C "%WORKSPACE%" rev-list HEAD --max-count=1 -- CMakeLists.txt cmake/modules cmake/platform cmake/scripts project/BuildDependencies/scripts tools/buildsteps/windows tools/depends') DO SET DEPENDS_REVISION=%%r
REM BuildSetup.bat builds Release when buildconfig is not set
IF NOT DEFINED buildconfig SET buildconfig=Release
FOR /F %%h IN ('ECHO %TARGETPLATFORM% %DEPENDS_REVISION% %VS_VERSION% %buildconfig%^| git hash-object --stdin') DO SET BUILD_HASH=%%h
EXIT /B 0

:restoreCachedBuild
move "%BUILD_CACHE_DIR%\%BUILD_HASH%\%TARGETPLATFORM%" "%APP_PATH%" >NUL || EXIT /B 0
rmdir "%BUILD_CACHE_DIR%\%BUILD_HASH%"
move /Y "%APP_PATH%\.build-hash" "%APP_PATH%\.build-hash.pending" >NUL
SET FORMED_TARGET_RESTORED=YES
ECHO Restored %APP_PATH% from the build cache
EXIT /B 0
