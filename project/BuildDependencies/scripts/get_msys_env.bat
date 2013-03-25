@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\get_msys_env.txt

CALL dlextract.bat msys_env %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy bin\* "%MSYS_INSTALL_PATH%\bin" /E /Q /I /Y || EXIT /B 5
xcopy lib\* "%MSYS_INSTALL_PATH%\lib" /E /Q /I /Y || EXIT /B 5
xcopy etc\* "%MSYS_INSTALL_PATH%\etc" /E /Q /I /Y || EXIT /B 5
xcopy share\* "%MSYS_INSTALL_PATH%\share" /E /Q /I /Y || EXIT /B 5
copy *.ico "%MSYS_INSTALL_PATH%" /Y || EXIT /B 5
copy *.bat "%MSYS_INSTALL_PATH%" /Y || EXIT /B 5
copy coreutils-5.97\bin\pr.exe "%MSYS_INSTALL_PATH%\bin\" /Y || EXIT /B 5

cd %LOC_PATH%
