
ECHO mkdir install ...
rmdir /s install\include
rmdir /s install\lib
mkdir install\include\dvdnav
mkdir install\lib

ECHO includes ...
xcopy /Y ..\src\dvdnav.h install\include\dvdnav
xcopy /Y ..\src\dvdnav_events.h install\include\dvdnav
xcopy /Y ..\src\dvd_types.h install\include\dvdnav
xcopy /Y ..\src\dvdread\dvd_reader.h install\include\dvdnav
xcopy /Y ..\src\dvdread\nav_read.h install\include\dvdnav
xcopy /Y ..\src\dvdread\ifo_read.h install\include\dvdnav
xcopy /Y ..\src\dvdread\nav_print.h install\include\dvdnav
xcopy /Y ..\src\dvdread\ifo_print.h install\include\dvdnav
xcopy /Y ..\src\dvdread\ifo_types.h install\include\dvdnav
xcopy /Y ..\src\dvdread\nav_types.h install\include\dvdnav

ECHO lib ...
xcopy /Y %1\libdvdnav\libdvdnav.lib install\lib
