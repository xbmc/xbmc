# Microsoft Developer Studio Project File - Name="FZS Interface" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=FZS Interface - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "FileZilla Server Interface.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "FileZilla Server Interface.mak" CFG="FZS Interface - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "FZS Interface - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "FZS Interface - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FZS Interface - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FAcs /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Shlwapi.lib version.lib ws2_32.lib /nologo /subsystem:windows /map /debug /machine:I386

!ELSEIF  "$(CFG)" == "FZS Interface - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FAcs /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Shlwapi.lib version.lib ws2_32.lib /nologo /subsystem:windows /map /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "FZS Interface - Win32 Release"
# Name "FZS Interface - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Accounts.cpp
# End Source File
# Begin Source File

SOURCE=.\AdminSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\AsyncSocketEx.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\BrowseForFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\ConnectDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\EnterSomething.cpp
# End Source File
# Begin Source File

SOURCE=".\FileZilla server.cpp"
# End Source File
# Begin Source File

SOURCE=".\FileZilla server.rc"
# End Source File
# Begin Source File

SOURCE=.\GroupsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupsDlgGeneral.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupsDlgSpeedLimit.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\HyperLink.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\Led.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\MailMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\MarkupSTL.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\NewUserDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\OfflineAskDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Options.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsAdminInterfacePage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsGeneralPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsGeneralWelcomemessagePage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsGSSPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsLoggingPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsMiscPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsPasvPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSecurityPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSpeedLimitPage.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\SAPrefsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\SAPrefsStatic.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\SAPrefsSubDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\SBDestination.cpp
# End Source File
# Begin Source File

SOURCE=..\SpeedLimit.cpp
# End Source File
# Begin Source File

SOURCE=.\SpeedLimitRuleDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\splitex.cpp
# End Source File
# Begin Source File

SOURCE=.\StatusCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\StatusView.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\misc\SystemTray.cpp
# End Source File
# Begin Source File

SOURCE=.\UsersDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\UsersDlgGeneral.cpp
# End Source File
# Begin Source File

SOURCE=.\UsersDlgSpeedLimit.cpp
# End Source File
# Begin Source File

SOURCE=.\UsersListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\UsersView.cpp
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

SOURCE=..\Accounts.h
# End Source File
# Begin Source File

SOURCE=.\AdminSocket.h
# End Source File
# Begin Source File

SOURCE=.\AsyncSocketEx.h
# End Source File
# Begin Source File

SOURCE=.\misc\BrowseForFolder.h
# End Source File
# Begin Source File

SOURCE=.\ConnectDialog.h
# End Source File
# Begin Source File

SOURCE=.\EnterSomething.h
# End Source File
# Begin Source File

SOURCE=".\FileZilla server.h"
# End Source File
# Begin Source File

SOURCE=.\GroupsDlg.h
# End Source File
# Begin Source File

SOURCE=.\GroupsDlgGeneral.h
# End Source File
# Begin Source File

SOURCE=.\GroupsDlgSpeedLimit.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\misc\md5.h
# End Source File
# Begin Source File

SOURCE=.\NewUserDlg.h
# End Source File
# Begin Source File

SOURCE=.\OfflineAskDlg.h
# End Source File
# Begin Source File

SOURCE=.\Options.h
# End Source File
# Begin Source File

SOURCE=.\OptionsAdminInterfacePage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\OptionsGeneralPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsGeneralWelcomemessagePage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsGSSPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsLoggingPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsMiscPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsPasvPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSecurityPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSpeedLimitPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\misc\SBDestination.h
# End Source File
# Begin Source File

SOURCE=..\SpeedLimit.h
# End Source File
# Begin Source File

SOURCE=.\SpeedLimitRuleDlg.h
# End Source File
# Begin Source File

SOURCE=.\splitex.h
# End Source File
# Begin Source File

SOURCE=.\StatusCtrl.h
# End Source File
# Begin Source File

SOURCE=.\StatusView.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\UsersDlg.h
# End Source File
# Begin Source File

SOURCE=.\UsersDlgGeneral.h
# End Source File
# Begin Source File

SOURCE=.\UsersDlgSpeedLimit.h
# End Source File
# Begin Source File

SOURCE=.\UsersListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\UsersView.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\misc\WheatyExceptionReport.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\donate.bmp
# End Source File
# Begin Source File

SOURCE=.\res\empty.ico
# End Source File
# Begin Source File

SOURCE=".\res\FileZilla server.ico"
# End Source File
# Begin Source File

SOURCE=".\res\FileZilla server.rc2"
# End Source File
# Begin Source File

SOURCE=.\res\green.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\leds.bmp
# End Source File
# Begin Source File

SOURCE=.\res\red.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\unknown.ico
# End Source File
# Begin Source File

SOURCE=.\res\yellow.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
