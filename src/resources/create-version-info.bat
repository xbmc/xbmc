set VERSION="1.0"
set FILEDESCR=/s desc "Software update utility."
set BUILDINFO=/s pb "Built by Robert Knight"
set COMPINFO=/s company "Mendeley Ltd." /s (c) "(C) Mendeley Ltd. 2011"
set PRODINFO=/s product "Mendeley Software Updater" /pv "1.0"

verpatch /vo /xi updater.exe %VERSION% %FILEDESCR% %COMPINFO% %PRODINFO% %BUILDINFO%