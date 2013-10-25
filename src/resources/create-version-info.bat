set VERSION="1.0"
set FILEDESCR=/s desc "Software update utility."
set BUILDINFO=/s pb "Built by Plex"
set COMPINFO=/s company "Plex Inc." /s (c) "(C) Plex Inc. 2011"
set PRODINFO=/s product "Plex Software Updater" /pv "1.0"

verpatch /vo /xi updater.exe %VERSION% %FILEDESCR% %COMPINFO% %PRODINFO% %BUILDINFO%
