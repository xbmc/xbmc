@ECHO OFF

SETLOCAL

REM Check presence of required file
dir 0_package.list >NUL 2>NUL || (
ECHO 0_package.list not found!
ECHO Aborting...
EXIT /B 20
)

REM Clear succeed flag
IF EXIST %FORMED_OK_FLAG% (
  del /F /Q "%FORMED_OK_FLAG%" || EXIT /B 4
)

CALL :setStageName Starting downloads of formed packages...
SET SCRIPT_PATH=%CD%
CD %DL_PATH% || EXIT /B 10
FOR /F "eol=; tokens=1" %%f IN (%SCRIPT_PATH%\0_package.list) DO (
CALL :processFile %%f
IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
)

CALL :setStageName All formed packages ready.
ECHO %DATE% %TIME% > "%FORMED_OK_FLAG%"
EXIT /B 0
REM End of main body


:processFile
CALL :setStageName Getting %1...
SET RetryDownload=YES
:startDownloadingFile
IF EXIST %1 (
  ECHO Using downloaded %1
) ELSE (
  CALL :setSubStageName Downloading %1...
  %WGET% "http://mirrors.xbmc.org/build-deps/win32/%1" || EXIT /B 7
  TITLE Getting %1
)

CALL :setSubStageName Extracting %1...
copy /b "%1" "%TMP_PATH%" || EXIT /B 5
PUSHD "%TMP_PATH%" || EXIT /B 10
%ZIP% x %1 || (
  IF %RetryDownload%==YES (
    POPD || EXIT /B 5
    ECHO WARNNING! Can't extract files from archive %1!
    ECHO WARNNING! Deleting %1 and will retry downloading.
    del /f "%1"
    SET RetryDownload=NO
    GOTO startDownloadingFile
  )
  exit /B 6
)

dir /A:-D "%~n1\*.*" >NUL 2>NUL && (
CALL :setSubStageName Pre-Cleaning %1...
REM Remove any non-dir files in extracted ".\packagename\"
FOR /F %%f IN ('dir /B /A:-D "%~n1\*.*"') DO (del "%~n1\%%f" /F /Q || EXIT /B 4)
)

CALL :setSubStageName Copying %1 to build tree...
REM Copy only content of extracted ".\packagename\"
XCOPY "%~n1\*" "%APP_PATH%\" /E /I /Y /F /R /H /K || EXIT /B 5

dir /A:-D * >NUL 2>NUL && (
CALL :setSubStageName Post-Cleaning %1...
REM Delete package archive and possible garbage
FOR /F %%f IN ('dir /B /A:-D') DO (del %%f /F /Q || EXIT /B 4)
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
