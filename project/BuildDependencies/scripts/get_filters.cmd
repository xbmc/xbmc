@ECHO OFF

SETLOCAL

REM Check presence of required file
dir 0_package_filters.list >NUL 2>NUL || (
ECHO 0_package_filters.list not found!
ECHO Aborting...
EXIT /B 20
)

REM Clear succeed flag
IF EXIST %FORMED_OK_FLAG% (
  del /F /Q "%FORMED_OK_FLAG%" || EXIT /B 4
)

SET SCRIPT_PATH=%CD%
CD %DL_PATH% || EXIT /B 10
FOR /F "eol=; tokens=1* delims==" %%a IN (%SCRIPT_PATH%\0_package_filters.list) DO (
CALL :processFile %%b %%a
IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
)

CALL :setStageName All formed packages ready.
ECHO %DATE% %TIME% > "%FORMED_OK_FLAG%"
EXIT /B 0
REM End of main body

:processFile
CALL :setStageName Getting %~nx1...
SET RetryDownload=YES
:startDownloadingFile
IF EXIST %~nx1 (
  ECHO Using downloaded %~nx1
) ELSE (
  CALL :setSubStageName Downloading %~nx1...
  %WGET% --no-check-certificate --output-document="%DL_PATH%/%~nx1" "%1" || EXIT /B 7
  TITLE Getting  %i
)

SET FILTER_PATH=%TMP_PATH%\%~n1\system\players\dsplayer\%2
IF NOT EXIST %FILTER_PATH% md %FILTER_PATH%

CALL :setSubStageName Extracting %~nx1...
copy /b "%~nx1" "%TMP_PATH%" || EXIT /B 5
PUSHD "%TMP_PATH%" || EXIT /B 10
%ZIP% x %~nx1 -o%FILTER_PATH% || (
  IF %RetryDownload%==YES (
    POPD || EXIT /B 5
    ECHO WARNNING! Can't extract files from archive %~nx1!
    ECHO WARNNING! Deleting %~nx1 and will retry downloading.
    del /f "%~nx1"
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
CALL :setSubStageName Post-Cleaning %1.zip...
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
