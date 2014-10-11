@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\get_mingw_env.txt

IF NOT EXIST %TMP_PATH% md %TMP_PATH%

CALL dlextract.bat mingw_env %FILES%

cd %TMP_PATH%

del lib\libexpat.dll.a
del lib\libexpat.la
del lib\libz.dll.a

xcopy bin\* "%MINGW_INSTALL_PATH%\bin" /E /Q /I /Y
xcopy doc\* "%MINGW_INSTALL_PATH%\doc" /E /Q /I /Y
xcopy include\* "%MINGW_INSTALL_PATH%\include" /E /Q /I /Y
xcopy lib\* "%MINGW_INSTALL_PATH%\lib" /E /Q /I /Y
xcopy share\* "%MINGW_INSTALL_PATH%\share" /E /Q /I /Y
xcopy libexec\* "%MINGW_INSTALL_PATH%\libexec" /E /Q /I /Y
xcopy mingw32\* "%MINGW_INSTALL_PATH%\mingw32" /E /Q /I /Y
copy yasm-1.2.0-win32.exe "%MINGW_INSTALL_PATH%\bin\yasm.exe" /Y
copy xasm.exe "%MINGW_INSTALL_PATH%\bin\xasm.exe" /Y
copy mads.exe "%MINGW_INSTALL_PATH%\bin\mads.exe" /Y
rem copy "%MINGW_INSTALL_PATH%\bin\mingw32-make.exe" "%MINGW_INSTALL_PATH%\bin\make.exe" /Y
rem xcopy curl-7.21.0-devel-mingw32\include\curl "%CUR_PATH%\include\curl" /E /Q /I /Y
rem copy curl-7.21.0-devel-mingw32\bin\*.dll "%APP_PATH%\system\" /Y

cd %LOC_PATH%
