@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libcdio_d.txt

CALL dlextract.bat libcdio %FILES% || EXIT /B 2

cd %TMP_PATH% || EXIT /B 1

xcopy libcdio-0.83-win32\include\* "%CUR_PATH%\include\" /E /Q /I /Y || EXIT /B 5
copy libcdio-0.83-win32\lib\libcdio.dll.lib "%CUR_PATH%\lib\" /Y || EXIT /B 5
copy libcdio-0.83-win32\bin\libcdio-13.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\" /Y || EXIT /B 5
copy libcdio-0.83-win32\bin\libiconv-2.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\" /Y || EXIT /B 5

rem for debugging
copy libcdio-0.83-win32\bin\libcdio-13.dll "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)\" /Y || EXIT /B 5
copy libcdio-0.83-win32\bin\libiconv-2.dll "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)\" /Y || EXIT /B 5

cd %LOC_PATH% || EXIT /B 1
