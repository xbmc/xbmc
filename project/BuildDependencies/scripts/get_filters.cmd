@ECHO OFF

SETLOCAL

SET LAV_HTTP="https://github.com/Nevcairiel/LAVFilters/releases/download/0.65/"
SET LAV_FILE="LAVFilters-0.65-x86.zip"
SET LAV_PATH="%TMP_PATH%\LAVFilters-0.65-x86\system\players\dsplayer\LAVFilters"

SET SUB_HTTP="https://github.com/Cyberbeing/xy-VSFilter/releases/download/3.1.0.705/"
SET SUB_FILE="XySubFilter_3.1.0.705_x86_BETA2.zip"
SET SUB_PATH="%TMP_PATH%\XySubFilter_3.1.0.705_x86_BETA2\system\players\dsplayer\XySubFilter"

SET TSR_HTTP="https://github.com/Romank1/MediaPortal-1/releases/download/v4.1.0.4/"
SET TSR_FILE="TsReader_v4_1_0_4_for_DSPlayer.zip"
SET TSR_PATH="%TMP_PATH%\TsReader_v4_1_0_4_for_DSPlayer\system\players\dsplayer\TsReader"

REM Clear succeed flag
IF EXIST %FORMED_OK_FLAG% (
  del /F /Q "%FORMED_OK_FLAG%" || EXIT /B 4
)

CALL :setStageName Starting downloads of formed packages...
SET SCRIPT_PATH=%CD%
CD %DL_PATH% || EXIT /B 10

SET CUR_HTTP=%LAV_HTTP%
SET CUR_PATH=%LAV_PATH%
CALL :processFile %LAV_FILE%
IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

SET CUR_HTTP=%SUB_HTTP%
SET CUR_PATH=%SUB_PATH%
CALL :processFile %SUB_FILE%
IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

SET CUR_HTTP=%TSR_HTTP%
SET CUR_PATH=%TSR_PATH%
CALL :processFile %TSR_FILE%
IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%

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
  %WGET% --no-check-certificate --output-document="%DL_PATH%/%1" "%CUR_HTTP%/%1" || EXIT /B 7
  TITLE Getting  %i
)

IF NOT EXIST %CUR_PATH% md %CUR_PATH%

CALL :setSubStageName Extracting %1...
copy /b "%1" "%TMP_PATH%" || EXIT /B 5
PUSHD "%TMP_PATH%" || EXIT /B 10
%ZIP% x %1 -o%CUR_PATH% || (
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
