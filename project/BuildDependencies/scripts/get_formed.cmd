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
IF "%KODI_MIRROR%" == "" SET KODI_MIRROR=https://mirrors.kodi.tv
echo Downloading from mirror %KODI_MIRROR%

REM If USE_LESSMSI is set to YES externally, disable msiexec. Primary usage is CI where
REM the service is running without TRUSTED_INSTALLER permissions to allow execution of msiexec
REM Usage: SET MSIEXEC=YES
IF "%USE_LESSMSI%" == "YES" (
  echo Using lessmsi for MSI extraction
)

CALL :setStageName Starting downloads of Host (%NATIVEPLATFORM%) formed packages...
SET SCRIPT_PATH=%CD%
CD %DL_PATH% || EXIT /B 10
FOR /F "eol=; tokens=1,2" %%f IN (%SCRIPT_PATH%\0_package.native-%NATIVEPLATFORM%.list) DO (
  IF "%%g" == "" (
    set "BASE_URL=%KODI_MIRROR%/build-deps/win32"
  ) ELSE (
    set "BASE_URL=%%g"
  )
  CALL :processFile %%f %NATIVE_PATH% !BASE_URL!
  REM Apparently there's a quirk in cmd so this means if error level => 1
  IF ERRORLEVEL 1 (
    ECHO One or more packages failed to download
    EXIT /B 7
  )
)

CALL :setStageName Starting downloads of Target (%TARGETPLATFORM%) formed packages...
CD %DL_PATH% || EXIT /B 10
FOR /F "eol=; tokens=1,2" %%f IN (%SCRIPT_PATH%\0_package.target-%TARGETPLATFORM%.list) DO (
  IF "%%g" == "" (
    set "BASE_URL=%KODI_MIRROR%/build-deps/win32"
  ) ELSE (
    set "BASE_URL=%%g"
  )
  CALL :processFile %%f %APP_PATH% !BASE_URL!
  REM Apparently there's a quirk in cmd so this means if error level => 1
  IF ERRORLEVEL 1 (
    ECHO One or more packages failed to download
    EXIT /B 7
  )
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
  CALL :setSubStageName Downloading %1 from %3...
  SET DOWNLOAD_URL=%3/%1
  curl --retry 5 --retry-all-errors --retry-connrefused --retry-delay 5 --location --remote-name "!DOWNLOAD_URL!" 2>&1
  REM Apparently there's a quirk in cmd so this means if error level => 1
  IF ERRORLEVEL 1 (
    ECHO %1^|Download of !DOWNLOAD_URL! failed >> %FORMED_FAILED_LIST%
    ECHO %1^|Download of !DOWNLOAD_URL! failed
    EXIT /B 7
  )
  TITLE Getting %1
)

CALL :setSubStageName Extracting %1...
copy /b "%1" "%TMP_PATH%" >NUL 2>NUL || EXIT /B 5
PUSHD "%TMP_PATH%" || EXIT /B 10
FOR /F  %%X IN ("%1") DO (
  if "%%~xX" == ".msi" (
    mkdir "%cd%\%%~nX"
    if "!USE_LESSMSI!" == "YES" (
      %NATIVE_PATH%\bin\lessmsi.exe x %1 %cd%\%%~nX\ >NUL 2>NUL || (
        IF %RetryDownload%==YES (
          POPD || EXIT /B 5
          ECHO WARNING! Can't extract files from archive %1!
          ECHO WARNING! Deleting %1 and will retry downloading.
          del /f "%1"
          rmdir /S /Q "%cd%\%%~nX"
          SET RetryDownload=NO
          GOTO startDownloadingFile
        ) ELSE (
          ECHO %1^|Can't extract files from archive %1 >> %FORMED_FAILED_LIST%
        )
        exit /B 6
      )
    ) else (
      msiexec /a "%cd%\%1" /qn TARGETDIR="%cd%\%%~nX" >NUL 2>NUL || (
        IF %RetryDownload%==YES (
          POPD || EXIT /B 5
          ECHO WARNING! Can't extract files from archive %1!
          ECHO WARNING! Deleting %1 and will retry downloading.
          del /f "%1"
          rmdir /S /Q "%cd%\%%~nX"
          SET RetryDownload=NO
          GOTO startDownloadingFile
        ) ELSE (
          ECHO %1^|Can't extract files from archive %1 >> %FORMED_FAILED_LIST%
        )
        exit /B 6
      )
    )
  ) ELSE (
    %ZIP% x %1 >NUL 2>NUL || (
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
  )
)

REM This only prepares lessmsi package into a folder structure that fits the rest of this system
REM relocating the data from the lessmsi zip into a bin folder root
if NOT exist %~n1\ (
  FOR /F %%H IN ('dir /B /S *.zip ^| findstr /I /R "lessmsi.*zip"') do (
    CALL :setSubStageName Arrange package data for package %1...
    REM Relocate files in extracted ".\packagename\" to bin folder for lessmsi
    ROBOCOPY "%TMP_PATH%" "%TMP_PATH%\%~n1\bin" /E /MOV /E /njh /njs /ndl /nc /ns /nfl /xf *.zip /xd %~n1 >NUL 2>NUL
    :: This is a dumb cleanup, which is also the reason lessmsi should be first package in 0_package.native-win32.list
    FOR /F %%f IN ('dir /B /A:D') DO (
      if NOT "%~n1" == "%%f" (
        rmdir /S /Q "%TMP_PATH%\%%f"
      )
    )
  )

  REM nasm doesnt extract into the same name as the archive. Move the extracted folder to match the same name
  REM to allow the rest of the script to function
  FOR /F %%I IN ('dir /B /A:D nasm*') do (
    mkdir "%~n1\bin"
    ROBOCOPY "%%I" "%~n1\bin" *.* /MOV /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
    rmdir /S /Q %%I
  )
)

dir /A:-D "%~n1\*.*" >NUL 2>NUL && (
CALL :setSubStageName Pre-Cleaning %1...
REM Remove any non-dir files in extracted ".\packagename\"
FOR /F %%f IN ('dir /B /A:-D "%~n1\*.*"') DO (del "%~n1\%%f" /F /Q || (ECHO %1^|Failed to pre-clean %~n1\%%f >> %FORMED_FAILED_LIST% && EXIT /B 4))
)

CALL :setSubStageName Re-arrange old-formed package %1 if necessary...
:: move project\BuildDependencies\*.* to root
dir /A:D "%~n1\project" >NUL 2>NUL && (
ROBOCOPY "%~n1\project\BuildDependencies\\" "%~n1\\" *.* /E /MOVE /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
dir /:D "%~n1\project\BuildDependencies" >NUL 2>NUL && (ECHO %1^|Failed to re-arrange package contents >> %FORMED_FAILED_LIST% && EXIT /B 5)
RD "%~n1\project" /S /Q
)

:: move system\*.* to bin
dir /A:D "%~n1\system" >NUL 2>NUL && (
ROBOCOPY "%~n1\system\\" "%~n1\bin" *.* /E /MOVE /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
dir /A:D "%~n1\system" >NUL 2>NUL && (ECHO %1^|Failed to re-arrange package contents >> %FORMED_FAILED_LIST% && EXIT /B 5)
)

:: move Win32\*.* to root
dir /A:D "%~n1\Win32" >NUL 2>NUL && (
ROBOCOPY "%~n1\Win32\\" "%~n1\\" *.* /E /MOVE /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
dir /A:D "%~n1\Win32" >NUL 2>NUL && (ECHO %1^|Failed to re-arrange package contents >> %FORMED_FAILED_LIST% && EXIT /B 5)
)

:: move PFiles64\*.* to bin (Meson msi extraction explicitly)
:: Not common structure for all msi extractions
:: limit to meson msi explicitly due to non standard folder structure extracted from msi
dir /B /S *.msi >NUL 2>NUL && (
  FOR /F %%H IN ('dir /B /S *.msi ^| findstr /I /R "meson.*msi"') do (
    if "!USE_LESSMSI!" == "YES" (
      :: lessmsi folder structure extracts to SourceDir in output folder
      dir /A:D "%~n1\SourceDir\PFiles64" >NUL 2>NUL && (
      ROBOCOPY "%~n1\SourceDir\PFiles64\\" "%~n1\bin" *.* /E /MOVE /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
      dir /A:D "%~n1\SourceDir\PFiles64" >NUL 2>NUL && (ECHO %1^|Failed to re-arrange msi package contents >> %FORMED_FAILED_LIST% && EXIT /B 5)
      rmdir /S /Q "%~n1\SourceDir"
      )
    ) else (
      :: msiexec extracts directly to output folder
      dir /A:D "%~n1\PFiles64" >NUL 2>NUL && (
      ROBOCOPY "%~n1\PFiles64\\" "%~n1\bin" *.* /E /MOVE /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
      dir /A:D "%~n1\PFiles64" >NUL 2>NUL && (ECHO %1^|Failed to re-arrange msi package contents >> %FORMED_FAILED_LIST% && EXIT /B 5)
      )
    )
  )
)

:: move x64\*.* to root
dir /A:D "%~n1\x64" >NUL 2>NUL && (
ROBOCOPY "%~n1\x64\\" "%~n1\\" *.* /E /MOVE /njh /njs /ndl /nc /ns /nfl >NUL 2>NUL
dir /A:D "%~n1\x64" >NUL 2>NUL && (ECHO %1^|Failed to re-arrange package contents >> %FORMED_FAILED_LIST% && EXIT /B 5)
)

CALL :setSubStageName Copying %1 to path %2...
REM Copy only content of extracted ".\packagename\"
XCOPY "%~n1\*" "%2\" /E /I /Y /F /R /H /K  >NUL 2>NUL|| (ECHO %1^|Failed to copy package contents to build tree >> %FORMED_FAILED_LIST% && EXIT /B 5)

dir /A:-D * >NUL 2>NUL && (
CALL :setSubStageName Post-Cleaning %1...
REM Delete package archive and possible garbage
FOR /F %%f IN ('dir /B /A:-D') DO (del %%f /F /Q  >NUL 2>NUL|| (ECHO %1^|Failed to post-clean %%f >> %FORMED_FAILED_LIST% && EXIT /B 4))
)

ECHO Done %1.
POPD || EXIT /B 10

EXIT /B 0
REM end of :processFile

:setStageName
ECHO.
ECHO ==================================================
ECHO %*
TITLE %*
EXIT /B 0

:setSubStageName
ECHO %*
EXIT /B 0
