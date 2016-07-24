@ECHO OFF

REM setup all paths
SET cur_dir=%CD%
SET base_dir=%cur_dir%\..\..
SET builddeps_dir=%cur_dir%\..\..\project\BuildDependencies
SET bin_dir=%builddeps_dir%\bin
SET msys_bin_dir=%builddeps_dir%\msys64
IF NOT EXIST %msys_bin_dir% SET msys_bin_dir=%builddeps_dir%\msys32
SET awk_exe=%msys_bin_dir%\usr\bin\awk.exe
SET sed_exe=%msys_bin_dir%\usr\bin\sed.exe

REM read the version values from version.txt
FOR /f %%i IN ('%awk_exe% "/APP_NAME/ {print $2}" %base_dir%\version.txt') DO SET app_name=%%i
FOR /f %%i IN ('%awk_exe% "/COMPANY_NAME/ {print $2}" %base_dir%\version.txt') DO SET company_name=%%i
FOR /f %%i IN ('%awk_exe% "/VERSION_MAJOR/ {print $2}" %base_dir%\version.txt') DO SET major=%%i
FOR /f %%i IN ('%awk_exe% "/VERSION_MINOR/ {print $2}" %base_dir%\version.txt') DO SET minor=%%i
FOR /f %%i IN ('%awk_exe% "/VERSION_TAG/ {print $2}" %base_dir%\version.txt') DO SET tag=%%i
FOR /f %%i IN ('%awk_exe% "/ADDON_API/ {print $2}" %base_dir%\version.txt') DO SET addon_api=%%i

SET app_version=%major%.%minor%
IF NOT [%tag%] == [] (
  SET app_version=%app_version%-%tag%
)

REM XBMC_PC.rc.in requires a comma-separated version of addon_api
SET separator=,
CALL SET file_version=%%addon_api:.=%separator%%%%separator%0

REM create the files with the proper version information
"%sed_exe%" -e s/@APP_NAME@/%app_name%/g -e s/@APP_VERSION_MAJOR@/%major%/g -e s/@APP_VERSION_MINOR@/%minor%/g -e s/@APP_VERSION_TAG@/%tag%/g "%base_dir%\xbmc\CompileInfo.cpp.in" > "%base_dir%\xbmc\CompileInfo.cpp"
"%sed_exe%" s/@APP_ADDON_API@/%addon_api%/g "%base_dir%\addons\xbmc.addon\addon.xml.in" > "%base_dir%\addons\xbmc.addon\addon.xml"
"%sed_exe%" -e s/@APP_NAME@/%app_name%/g -e s/@COMPANY_NAME@/%company_name%/g -e s/@APP_VERSION_MAJOR@/%major%/g -e s/@APP_VERSION_MINOR@/%minor%/g -e s/@APP_VERSION_TAG@/%tag%/g -e s/@FILE_VERSION@/%file_version%/g -e s/@APP_VERSION@/%app_version%/g "%base_dir%\xbmc\platform\win32\XBMC_PC.rc.in" > "%base_dir%\xbmc\platform\win32\XBMC_PC.rc"

