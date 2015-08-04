@ECHO OFF

REM Turn on delayed variable expansion so that we can interpolate environment variables in the URLs
SETLOCAL enableDelayedExpansion

echo Downloading %1
echo --------------

cd %DL_PATH%

FOR /F "eol=; tokens=1,2" %%f IN (%2) DO (
  set MIRROR=%%g
  set MIRROR=!MIRROR:$KODI_MIRROR=%KODI_MIRROR%!
  echo !MIRROR! %%f
  IF NOT EXIST %%f (
    %WGET% "!MIRROR!%%f"
    IF NOT EXIST %%f (
      echo Failed to download file %%f from !MIRROR!
      EXIT /B 1
    )
  ) ELSE (
    echo Already have %%f
  )

  copy /b "%%f" "%TMP_PATH%"
)

echo Extracting...
echo -------------

cd %TMP_PATH%

FOR /F "eol=; tokens=1,2" %%f IN (%2) DO (
  %ZIP% x -y %%f
  IF %ERRORLEVEL% NEQ 0 (
    echo Failed to unpack archive %%f
    EXIT /B 2
  )
)

FOR /F "tokens=*" %%f IN ('dir /B "*.tar"') DO (
  %ZIP% x -y %%f
  IF %ERRORLEVEL% NEQ 0 (
    echo Failed to unpack archive %%f
    EXIT /B 2
  )
)

ENDLOCAL
