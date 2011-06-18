@ECHO ON

SET LOC_PATH=%CD%
SET FILES=%LOC_PATH%\boost_d.txt

CALL dlextract.bat boost %FILES%

cd %TMP_PATH%

xcopy boost_1_46_1-headers-win32\* "%CUR_PATH%\include\" /E /Q /I /Y

copy boost_1_46_1-debug-libs-win32\lib\libboost_thread-vc100-mt-gd.lib "%CUR_PATH%\lib\libboost_thread-vc100-mt-sgd-1_46_1.lib"/Y
copy boost_1_46_1-libs-win32\lib\libboost_thread-vc100-mt.lib "%CUR_PATH%\lib\libboost_thread-vc100-mt-1_46_1.lib" /Y

copy boost_1_46_1-debug-libs-win32\lib\libboost_date_time-vc100-mt-gd.lib "%CUR_PATH%\lib\libboost_date_time-vc100-mt-sgd-1_46_1.lib"/Y
copy boost_1_46_1-libs-win32\lib\libboost_date_time-vc100-mt.lib "%CUR_PATH%\lib\libboost_date_time-vc100-mt-1_46_1.lib" /Y

cd %LOC_PATH%