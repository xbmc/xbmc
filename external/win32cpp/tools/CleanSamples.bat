
REM:A batch file to remove unnecessary files from
REM: each Visual Studio project

REM: Change the directory to the parent
pushd ..\Samples

::Remove directories
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\Borland"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\Debug"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\Debug_Build"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\src\Dev-C++"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\GNU"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\Microsoft"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\Release"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\Release_Build"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\x64"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\_UpgradeReport_Files"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\__history"
FOR /D %%f IN ("*.") DO RMDIR /S /Q "%%f\ProjectFiles\ipch


::Remove files
FOR /D %%f IN ("*.") DO DEL /Q /AH "%%f\ProjectFiles\*.suo"
FOR /D %%f IN ("*.") DO DEL /Q /AH "%%f\ProjectFiles\*.old"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.ncb"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.plg"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\err*.*"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\tmp*.*"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.pdb"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.aps"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.cbTemp"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.opt"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.user"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.depend"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.XML"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.o"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.old"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.layout"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.local"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.log"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.dat"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.bak"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.sdf"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.vcb"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.vcl"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\ProjectFiles\*.vco"

FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\*private.*"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\Makefile.win"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\*.aps"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\*.bak"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\*.bml"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\*.layout"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\RibbonUI.h"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\src\RibbonUI.rc"

REM: Clean the Networking directory
pushd Networking

::Remove directories
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Borland
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Debug
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Debug_Build
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Dev-C++
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\GNU
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Microsoft
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Release
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\Release_Build
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\x64
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\_UpgradeReport_Files
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\__history
FOR /D %%f IN ("*.") DO DEL /Q /AH "%%f\*.suo"
FOR /D %%f IN ("*.") DO DEL /Q /AH "%%f\*.old"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.ncb"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.plg"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\err*.*"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\tmp*.*"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.pdb"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.aps"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.cbTemp"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.opt"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.user"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.depend"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.XML"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.o"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.old"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.layout"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.local"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.log"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.dat"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.bak"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.sdf"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.vcb"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.vcl"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.vco"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*private.*"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\Makefile.win"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.aps"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.bak"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.bml"
FOR /D %%f IN ("*.") DO DEL /Q "%%f\*.layout"


popd
popd


