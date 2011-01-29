@ECHO OFF
rem compiles a bunch of mingw libs and not more
rem might be a nice addition if we check for success?
rem still not working right

..\BuildDependencies\msys\bin\sh --login -i /xbmc/lib/ffmpeg/build_xbmc_win32.sh
..\BuildDependencies\msys\bin\sh --login -i /xbmc/lib/libdvd/build-xbmc-win32.sh

