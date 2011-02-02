@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\get_msys_env.txt

CALL dlextract.bat msys_env %FILES%

cd %TMP_PATH%

xcopy bin\* "%MSYS_INSTALL_PATH%\bin" /E /Q /I /Y
xcopy lib\* "%MSYS_INSTALL_PATH%\lib" /E /Q /I /Y
xcopy etc\* "%MSYS_INSTALL_PATH%\etc" /E /Q /I /Y
xcopy share\* "%MSYS_INSTALL_PATH%\share" /E /Q /I /Y
copy *.ico "%MSYS_INSTALL_PATH%" /Y
copy *.bat "%MSYS_INSTALL_PATH%" /Y
copy coreutils-5.97\bin\pr.exe "%MSYS_INSTALL_PATH%\bin\" /Y

cd %LOC_PATH%
