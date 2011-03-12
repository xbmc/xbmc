@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\python2.6_d.txt

CALL dlextract.bat python2.6 %FILES%

cd %TMP_PATH%

set DEBUG=false

echo \test\ > py_exclude.txt

if "%DEBUG%" == "false" (
  echo _d.dll >> py_exclude.txt
  echo _d.pyd >> py_exclude.txt
  echo _d.lib >> py_exclude.txt
  echo .pdb >> py_exclude.txt
  echo tcl85g. >> py_exclude.txt
  echo tclpip85g. >> py_exclude.txt
  echo tk85g. >> py_exclude.txt
)

xcopy python2.6.6\include\* "%CUR_PATH%\include\python" /E /Q /I /Y /EXCLUDE:py_exclude.txt
xcopy python2.6.6\python\DLLs "%XBMC_PATH%\system\python\DLLs" /E /Q /I /Y /EXCLUDE:py_exclude.txt
xcopy python2.6.6\python\Lib "%XBMC_PATH%\system\python\Lib" /E /Q /I /Y /EXCLUDE:py_exclude.txt
xcopy python2.6.6\python26.dll "%XBMC_PATH%\system\python\" /Q /I /Y /EXCLUDE:py_exclude.txt
xcopy python2.6.6\python26_d.dll "%XBMC_PATH%\system\python\" /Q /I /Y /EXCLUDE:py_exclude.txt
xcopy python2.6.6\libs\*.lib "%CUR_PATH%\lib\" /E /Q /I /Y /EXCLUDE:py_exclude.txt
xcopy python2.6.6\libs\*.pdb "%CUR_PATH%\lib\" /E /Q /I /Y /EXCLUDE:py_exclude.txt

cd %LOC_PATH%
