@ECHO OFF

rmdir tmp /S /Q
md tmp
cd tmp

echo Downloading fontconfig and freetype6.dll ...
echo ------------------------------------------
%WGET% "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/fontconfig-dev_2.8.0-2_win32.zip"
%WGET% "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/fontconfig_2.8.0-2_win32.zip"
%WGET% "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/freetype_2.3.12-1_win32.zip"

%ZIP% x fontconfig-dev_2.8.0-2_win32.zip
%ZIP% x fontconfig_2.8.0-2_win32.zip
%ZIP% x freetype_2.3.12-1_win32.zip

xcopy include\fontconfig "%CUR_PATH%\include\fontconfig" /E /Q /I /Y
copy lib\fontconfig.lib "%CUR_PATH%\lib\" /Y
rem libfontconfig-1.dll requires libexpat-1.dll which is copied by libexpat_d.bat
copy bin\libfontconfig-1.dll "%XBMC_PATH%\system\players\dvdplayer\"
copy bin\freetype6.dll "%XBMC_PATH%\system\players\dvdplayer\"

cd ..
