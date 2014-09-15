@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcdio_d.txt

CALL dlextract.bat libcdio %FILES%

cd %TMP_PATH%

xcopy libcdio-0.83-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy libcdio-0.83-win32\lib\libcdio.dll.lib "%CUR_PATH%\lib\" /Y
copy libcdio-0.83-win32\bin\libcdio-13.dll "%APP_PATH%\project\Win32BuildSetup\dependencies\" /Y
copy libcdio-0.83-win32\bin\libiconv-2.dll "%APP_PATH%\project\Win32BuildSetup\dependencies\" /Y

cd %LOC_PATH%
