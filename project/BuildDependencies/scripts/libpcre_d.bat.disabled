@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libpcre_d.txt

CALL dlextract.bat libpcre %FILES%

cd %TMP_PATH%

xcopy include\* "%CUR_PATH%\include\" /E /Q /I /Y
copy lib\pcre.lib "%CUR_PATH%\lib\" /Y
copy lib\pcrecpp.lib "%CUR_PATH%\lib\" /Y
copy bin\pcre.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\" /Y
copy bin\pcrecpp.dll "%XBMC_PATH%\project\Win32BuildSetup\dependencies\" /Y

rem for debugging
copy bin\pcre.dll "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)\" /Y
copy bin\pcrecpp.dll "%XBMC_PATH%\project\VS2010Express\XBMC\Debug (DirectX)\" /Y

cd %LOC_PATH%
