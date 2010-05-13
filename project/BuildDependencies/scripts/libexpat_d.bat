@ECHO OFF

rmdir tmp /S /Q
md tmp
cd tmp

echo Downloading libexpat ...
echo ------------------------
%WGET% "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/expat_2.0.1-1_win32.zip"
%WGET% "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/expat-dev_2.0.1-1_win32.zip"

%ZIP% x expat_2.0.1-1_win32.zip
%ZIP% x expat-dev_2.0.1-1_win32.zip

copy include\* "%CUR_PATH%\include\" /Y
copy lib\expat.lib "%CUR_PATH%\lib\libexpat.lib" /Y
copy bin\libexpat-1.dll "%XBMC_PATH%\system\libexpat.dll"
rem libexpat-1.dll for libfontconfig-1.dll which is needed for libass.dll
copy bin\libexpat-1.dll "%XBMC_PATH%\system\players\dvdplayer\"

cd ..
