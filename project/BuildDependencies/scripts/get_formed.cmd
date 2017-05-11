@ECHO OFF

SETLOCAL

REM So that we can expand variables inside of IF and FOR
SETLOCAL enableDelayedExpansion

REM Check presence of required file
dir 0_package.native-%NATIVEPLATFORM%.list >NUL 2>NUL && dir 0_package.target-%TARGETPLATFORM%.list >NUL 2>NUL || (
  ECHO 0_package.native-%NATIVEPLATFORM%.list or 0_package.target-%TARGETPLATFORM%.list not found!
  ECHO Aborting...
  EXIT /B 20
)

REM Clear succeed flag
IF EXIST %FORMED_OK_FLAG% (
  del /F /Q "%FORMED_OK_FLAG%" || EXIT /B 4
)

REM Clear the "failures" list
SET FORMED_FAILED_LIST=%TMP_PATH%\failed-formed-packages
IF EXIST %FORMED_FAILED_LIST% (
  del /F /Q "%FORMED_FAILED_LIST%" || EXIT /B 4
)

REM If KODI_MIRROR is not set externally to this script, set it to the default mirror URL
IF "%KODI_MIRROR%" == "" SET KODI_MIRROR=http://mirrors.kodi.tv
echo Downloading from mirror %KODI_MIRROR%


CALL :setStageName Starting downloads of formed packages...
SET SCRIPT_PATH=%CD%
CD %DL_PATH% || EXIT /B 10
FOR /F "eol=; tokens=1" %%f IN (%SCRIPT_PATH%\0_package.native-%NATIVEPLATFORM%.list %SCRIPT_PATH%\0_package.target-%TARGETPLATFORM%.list) DO (
  CALL :processFile %%f
)

REM Report any errors
IF EXIST %FORMED_FAILED_LIST% (
  CALL :setStageName Some formed packages had errors
  ECHO.
  FOR /F "tokens=1,2 delims=|" %%x IN (%FORMED_FAILED_LIST%) DO @ECHO %%x: - %%y
  ECHO.
  EXIT /B 101
) ELSE (
  CALL :setStageName All formed packages ready.
  ECHO %DATE% %TIME% > "%FORMED_OK_FLAG%"
  EXIT /B 0
)
REM End of main body


:processFile
CALL :setStageName Getting %1...
SET RetryDownload=YES
:startDownloadingFile
IF EXIST %1 (
  ECHO Using downloaded %1
) ELSE (
  CALL :setSubStageName Downloading %1...
  SET DOWNLOAD_URL=%KODI_MIRROR%/build-deps/win32/%1
  %WGET% --tries=5 --retry-connrefused --waitretry=2 "!DOWNLOAD_URL!" 2>&1 || ECHO %1^|Download of !DOWNLOAD_URL! failed >> %FORMED_FAILED_LIST% && EXIT /B 7
  TITLE Getting %1
)

CALL :setSubStageName Extracting %1...
copy /b "%1" "%TMP_PATH%" || EXIT /B 5
PUSHD "%TMP_PATH%" || EXIT /B 10
%ZIP% x %1 || (
  IF %RetryDownload%==YES (
    POPD || EXIT /B 5
    ECHO WARNING! Can't extract files from archive %1!
    ECHO WARNING! Deleting %1 and will retry downloading.
    del /f "%1"
    SET RetryDownload=NO
    GOTO startDownloadingFile
  ) ELSE (
    ECHO %1^|Can't extract files from archive %1 >> %FORMED_FAILED_LIST%
  )
  exit /B 6
)

dir /A:-D "%~n1\*.*" >NUL 2>NUL && (
CALL :setSubStageName Pre-Cleaning %1...
REM Remove any non-dir files in extracted ".\packagename\"
FOR /F %%f IN ('dir /B /A:-D "%~n1\*.*"') DO (del "%~n1\%%f" /F /Q || (ECHO %1^|Failed to pre-clean %~n1\%%f >> %FORMED_FAILED_LIST% && EXIT /B 4))
)

CALL :setSubStageName Copying %1 to build tree...
REM Copy only content of extracted ".\packagename\"
XCOPY "%~n1\*" "%APP_PATH%\" /E /I /Y /F /R /H /K || (ECHO %1^|Failed to copy package contents to build tree >> %FORMED_FAILED_LIST% && EXIT /B 5)

dir /A:-D * >NUL 2>NUL && (
CALL :setSubStageName Post-Cleaning %1...
REM Delete package archive and possible garbage
FOR /F %%f IN ('dir /B /A:-D') DO (del %%f /F /Q || (ECHO %1^|Failed to post-clean %%f >> %FORMED_FAILED_LIST% && EXIT /B 4))
)

ECHO.
ECHO Done %1.
POPD || EXIT /B 10

EXIT /B 0
REM end of :processFile

:setStageName
ECHO.
ECHO ==================================================
ECHO %*
ECHO.
TITLE %*
EXIT /B 0

:setSubStageName
ECHO.
ECHO --------------------------------------------------
ECHO %*
ECHO.
EXIT /B 0
