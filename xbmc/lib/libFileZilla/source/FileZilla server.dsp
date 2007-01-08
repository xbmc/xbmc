# Microsoft Developer Studio Project File - Name="Service" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Service - Win32 Compat Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "FileZilla server.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "FileZilla server.mak" CFG="Service - Win32 Compat Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "Service - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "Service - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE "Service - Win32 Compat Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Service - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FAcs /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "Service - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FAcs /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Debug
SOURCE="$(InputPath)"
PreLink_Desc=Terminate service...
PreLink_Cmds=if EXIST "$(OutDir)\FileZilla Server.exe" "$(OutDir)\FileZilla Server.exe" /stop	if EXIST "$(OutDir)\FileZilla Server.exe" "$(OutDir)\FileZilla Server.exe" /uninstall
PostBuild_Desc=Install Service...
PostBuild_Cmds="$(OutDir)\FileZilla Server.exe" /stop	"$(OutDir)\FileZilla Server.exe" /uninstall	"$(OutDir)\FileZilla Server.exe" /install auto	"$(OutDir)\FileZilla Server.exe" /start
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Service - Win32 Compat Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Service___Win32_Compat_Debug"
# PROP BASE Intermediate_Dir "Service___Win32_Compat_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Compat_Debug"
# PROP Intermediate_Dir "Compat_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /nodefaultlib:"libcmt.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
OutDir=.\Compat_Debug
SOURCE="$(InputPath)"
PreLink_Desc=Terminate service...
PreLink_Cmds=if EXIST "$(OutDir)\FileZilla Server.exe" "$(OutDir)\FileZilla Server.exe" /stop	if EXIST "$(OutDir)\FileZilla Server.exe" "$(OutDir)\FileZilla Server.exe" /compat /stop	if EXIST "$(OutDir)\FileZilla Server.exe" "$(OutDir)\FileZilla Server.exe" /uninstall
PostBuild_Cmds="$(OutDir)\FileZilla Server.exe" /stop	"$(OutDir)\FileZilla Server.exe" /uninstall
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Service - Win32 Release"
# Name "Service - Win32 Debug"
# Name "Service - Win32 Compat Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Accounts.cpp
# End Source File
# Begin Source File

SOURCE=.\AdminInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\AdminListenSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\AdminSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\AsyncGssSocketLayer.cpp
# End Source File
# Begin Source File

SOURCE=.\AsyncSocketEx.cpp
# End Source File
# Begin Source File

SOURCE=.\AsyncSocketExLayer.cpp
# End Source File
# Begin Source File

SOURCE=.\ControlSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\ExternalIpCheck.cpp
# End Source File
# Begin Source File

SOURCE=.\FileLogger.cpp
# End Source File
# Begin Source File

SOURCE=".\FileZilla server.rc"
# End Source File
# Begin Source File

SOURCE=.\ListenSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\MailMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\MarkupSTL.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\MFC64bitFix.cpp
# End Source File
# Begin Source File

SOURCE=.\Options.cpp
# End Source File
# Begin Source File

SOURCE=.\Permissions.cpp
# End Source File
# Begin Source File

SOURCE=.\Server.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerThread.cpp
# End Source File
# Begin Source File

SOURCE=.\Service.cpp
# End Source File
# Begin Source File

SOURCE=.\SpeedLimit.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Thread.cpp
# End Source File
# Begin Source File

SOURCE=.\TransferSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\version.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\WheatyExceptionReport.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Accounts.h
# End Source File
# Begin Source File

SOURCE=.\AdminInterface.h
# End Source File
# Begin Source File

SOURCE=.\AdminListenSocket.h
# End Source File
# Begin Source File

SOURCE=.\AdminSocket.h
# End Source File
# Begin Source File

SOURCE=.\AsyncGssSocketLayer.h
# End Source File
# Begin Source File

SOURCE=.\AsyncSocketEx.h
# End Source File
# Begin Source File

SOURCE=.\AsyncSocketExLayer.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\ControlSocket.h
# End Source File
# Begin Source File

SOURCE=.\ExternalIpCheck.h
# End Source File
# Begin Source File

SOURCE=.\FileLogger.h
# End Source File
# Begin Source File

SOURCE=.\ListenSocket.h
# End Source File
# Begin Source File

SOURCE=.\MFC64bitFix.h
# End Source File
# Begin Source File

SOURCE=.\Options.h
# End Source File
# Begin Source File

SOURCE=.\OptionTypes.h
# End Source File
# Begin Source File

SOURCE=.\Permissions.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Server.h
# End Source File
# Begin Source File

SOURCE=.\ServerThread.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Thread.h
# End Source File
# Begin Source File

SOURCE=.\TransferSocket.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\empty.ico
# End Source File
# Begin Source File

SOURCE=".\res\FileZilla server.ico"
# End Source File
# Begin Source File

SOURCE=.\res\green.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\red.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\yellow.ico
# End Source File
# End Group
# End Target
# End Project
