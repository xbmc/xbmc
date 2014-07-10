@ECHO OFF

SETLOCAL

SET EXITCODE=0

SET getdepends=true
FOR %%b in (%1) DO (
	IF %%b==nodepends SET getdepends=false
)

rem set Visual C++ build environment
call "%VS120COMNTOOLS%..\..\VC\bin\vcvars32.bat"

SET WORKDIR=%WORKSPACE%

IF "%WORKDIR%"=="" (
  SET WORKDIR=%CD%\..\..\..
)

rem setup some paths that we need later
SET CUR_PATH=%CD%
SET BASE_PATH=%WORKDIR%\project\cmake
SET ADDONS_PATH=%BASE_PATH%\addons
SET ADDON_DEPENDS_PATH=%ADDONS_PATH%\output
SET ADDONS_BUILD_PATH=%ADDONS_PATH%\build

SET ERRORFILE=%BASE_PATH%\make-addons.error

SET XBMC_INCLUDE_PATH=%ADDON_DEPENDS_PATH%\include\xbmc
SET XBMC_LIB_PATH=%ADDON_DEPENDS_PATH%\lib\xbmc

IF %getdepends%==true (
  CALL make-addon-depends.bat
  IF ERRORLEVEL 1 (
    ECHO make-addon-depends error level: %ERRORLEVEL% > %ERRORFILE%
    GOTO ERROR
  )
)

rem make sure the xbmc include and library paths exist
IF EXIST "%XBMC_INCLUDE_PATH%" (
  RMDIR "%XBMC_INCLUDE_PATH%" /S /Q > NUL
)
IF EXIST "%XBMC_LIB_PATH%" (
  RMDIR "%XBMC_LIB_PATH%" /S /Q > NUL
)
MKDIR "%XBMC_INCLUDE_PATH%"
MKDIR "%XBMC_LIB_PATH%"

rem go into the addons directory
CD %ADDONS_PATH%

rem remove the build directory if it exists
IF EXIST "%ADDONS_BUILD_PATH%" (
  RMDIR "%ADDONS_BUILD_PATH%" /S /Q > NUL
)

rem create the build directory
MKDIR "%ADDONS_BUILD_PATH%"

rem go into the build directory
CD "%ADDONS_BUILD_PATH%"

rem execute cmake to generate makefiles processable by nmake
cmake "%ADDONS_PATH%" -G "NMake Makefiles" ^
      -DXBMCROOT=%WORKDIR% ^
      -DDEPENDS_PATH=%ADDON_DEPENDS_PATH% ^
      -DCMAKE_INSTALL_PREFIX=%WORKDIR%\project\Win32BuildSetup\BUILD_WIN32\Xbmc\xbmc-addons ^
      -DPACKAGE_ZIP=1 ^
      -DARCH_DEFINES="-DTARGET_WINDOWS -DNOMINMAX"
IF ERRORLEVEL 1 (
  ECHO cmake error level: %ERRORLEVEL% > %ERRORFILE%
  GOTO ERROR
)

rem execute nmake to build the addons
nmake
IF ERRORLEVEL 1 (
  ECHO nmake error level: %ERRORLEVEL% > %ERRORFILE%
  GOTO ERROR
)

rem everything was fine
GOTO END

:ERROR
rem something went wrong
ECHO Failed to build addons
ECHO See %ERRORFILE% for more details
SET EXITCODE=1

:END
rem go back to the original directory
cd %CUR_PATH%

rem exit the script with the defined exitcode
EXIT /B %EXITCODE%
