@ECHO OFF
rem compiles a bunch of mingw libs and not more
rem might be a nice addition if we check for success?
rem still not working right

..\BuildDependencies\msys\bin\rxvt -backspacekey  -sl 2500 -sr -fn Courier-12 -tn msys -geometry 120x25 -e /bin/sh --login /xbmc/lib/ffmpeg/build_xbmc_win32.sh
..\BuildDependencies\msys\bin\rxvt -backspacekey  -sl 2500 -sr -fn Courier-12 -tn msys -geometry 120x25 -e /bin/sh --login /xbmc/lib/libdvd/build-xbmc-win32.sh

