@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\libvpx_d.txt
SET LIBVPX="%XBMC_PATH%\xbmc\cores\dvdplayer\Codecs\libvpx"

CALL dlextract.bat libvpx %FILES%

IF EXIST %LIBVPX% rmdir %LIBVPX% /S /Q

cd %TMP_PATH%

xcopy libvpx-0.9.1 %LIBVPX% /E /Q /I /Y

cd %LOC_PATH%
