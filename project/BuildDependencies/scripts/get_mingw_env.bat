@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\get_mingw_env.txt

IF NOT EXIST %TMP_PATH% md %TMP_PATH% || EXIT /B 3

CALL dlextract.bat mingw_env %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

del lib\libexpat.dll.a || EXIT /B 6
del lib\libexpat.la || EXIT /B 6
del lib\libz.dll.a || EXIT /B 6

xcopy bin\* "%MINGW_INSTALL_PATH%\bin" /E /Q /I /Y || EXIT /B 5
xcopy doc\* "%MINGW_INSTALL_PATH%\doc" /E /Q /I /Y || EXIT /B 5
xcopy include\* "%MINGW_INSTALL_PATH%\include" /E /Q /I /Y || EXIT /B 5
xcopy lib\* "%MINGW_INSTALL_PATH%\lib" /E /Q /I /Y || EXIT /B 5
xcopy share\* "%MINGW_INSTALL_PATH%\share" /E /Q /I /Y || EXIT /B 5
xcopy libexec\* "%MINGW_INSTALL_PATH%\libexec" /E /Q /I /Y || EXIT /B 5
xcopy mingw32\* "%MINGW_INSTALL_PATH%\mingw32" /E /Q /I /Y || EXIT /B 5
copy yasm-1.2.0-win32.exe "%MINGW_INSTALL_PATH%\bin\yasm.exe" /Y || EXIT /B 5
copy xasm.exe "%MINGW_INSTALL_PATH%\bin\xasm.exe" /Y || EXIT /B 5
copy mads.exe "%MINGW_INSTALL_PATH%\bin\mads.exe" /Y || EXIT /B 5
rem xcopy curl-7.21.0-devel-mingw32\include\curl "%CUR_PATH%\include\curl" /E /Q /I /Y
rem copy curl-7.21.0-devel-mingw32\bin\*.dll "%XBMC_PATH%\system\" /Y

cd %LOC_PATH% || EXIT /B 1
