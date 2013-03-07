@ECHO OFF

echo Downloading %1
echo --------------

cd %DL_PATH% || EXIT /B 1

FOR /F "eol=; tokens=1,2" %%f IN (%2) DO (
echo %%f %%g
  IF NOT EXIST %%f (
    %WGET% "%%g/%%f" || EXIT /B 10
  ) ELSE (
    echo Already have %%f
  )

  copy /b "%%f" "%TMP_PATH%" || EXIT /B 5
)

echo Extracting...
echo -------------

cd %TMP_PATH% || EXIT /B 1

FOR /F "eol=; tokens=1,2" %%f IN (%2) DO (
  %ZIP% x %%f || EXIT /B 4
)

FOR %%f IN (*.tar) DO (
  %ZIP% x -y %%f || EXIT /B 4
)