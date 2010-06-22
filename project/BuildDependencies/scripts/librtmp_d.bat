@ECHO OFF

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\librtmp_d.txt

CALL dlextract.bat librtmp %FILES%

cd %TMP_PATH%
%ZIP% x rtmpdump-2.2e.tar

xcopy rtmpdump-2.2e\librtmp\*.h "%CUR_PATH%\include\librtmp\" /E /Q /I /Y

cd %LOC_PATH%
