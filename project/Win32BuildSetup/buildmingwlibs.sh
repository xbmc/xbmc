# compile our mingw dlls
echo "##### building ffmpeg dlls #####"
sh /xbmc/lib/ffmpeg/build_xbmc_win32.sh
echo "##### building of ffmpeg dlls done #####"

echo "##### building libdvd dlls #####"
sh /xbmc/lib/libdvd/build-xbmc-win32.sh
echo "##### building of libdvd dlls done #####"

# wait for key press
echo press a key to close the window
read