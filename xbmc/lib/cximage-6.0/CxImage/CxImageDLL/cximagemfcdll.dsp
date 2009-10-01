# Microsoft Developer Studio Project File - Name="CxImageMfcDll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CxImageMfcDll - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cximagemfcdll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cximagemfcdll.mak" CFG="CxImageMfcDll - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CxImageMfcDll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CxImageMfcDll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CxImageMfcDll - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CxImageMfcDll - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CxImageMfcDll - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../zlib" /I "../../jasper/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "JAS_WIN_MSVC_BUILD" /D "_CRT_SECURE_NO_DEPRECATE" /D _WIN32_IE=0x0400 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../bin/cximaged.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "CxImageMfcDll - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../zlib" /I "../../jasper/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "JAS_WIN_MSVC_BUILD" /D "_CRT_SECURE_NO_DEPRECATE" /D _WIN32_IE=0x0400 /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"../../bin/cximage.dll"

!ELSEIF  "$(CFG)" == "CxImageMfcDll - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CxImageMfcDll___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "CxImageMfcDll___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Unicode_Debug"
# PROP Intermediate_Dir "Unicode_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../zlib" /I "../../jasper/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "JAS_WIN_MSVC_BUILD" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../zlib" /I "../../jasper/include" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "JAS_WIN_MSVC_BUILD" /D "WIN32" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /D _WIN32_IE=0x0400 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../bin/cximaged.dll" /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../bin/cximagedu.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "CxImageMfcDll - Win32 Unicode Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CxImageMfcDll___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "CxImageMfcDll___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Unicode_Release"
# PROP Intermediate_Dir "Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "../../zlib" /I "../../jasper/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "JAS_WIN_MSVC_BUILD" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../zlib" /I "../../jasper/include" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "JAS_WIN_MSVC_BUILD" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /D _WIN32_IE=0x0400 /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"../../bin/cximage.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"../../bin/cximageu.dll"

!ENDIF 

# Begin Target

# Name "CxImageMfcDll - Win32 Debug"
# Name "CxImageMfcDll - Win32 Release"
# Name "CxImageMfcDll - Win32 Unicode Debug"
# Name "CxImageMfcDll - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cximagemfcdll.cpp
# End Source File
# Begin Source File

SOURCE=.\cximagemfcdll.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\tif_xfile.cpp
# End Source File
# Begin Source File

SOURCE=..\ximabmp.cpp
# End Source File
# Begin Source File

SOURCE=..\ximadsp.cpp
# End Source File
# Begin Source File

SOURCE=..\ximaenc.cpp
# End Source File
# Begin Source File

SOURCE=..\ximaexif.cpp
# End Source File
# Begin Source File

SOURCE=..\ximage.cpp
# End Source File
# Begin Source File

SOURCE=..\ximagif.cpp
# End Source File
# Begin Source File

SOURCE=..\ximahist.cpp
# End Source File
# Begin Source File

SOURCE=..\ximaico.cpp
# End Source File
# Begin Source File

SOURCE=..\ximainfo.cpp
# End Source File
# Begin Source File

SOURCE=..\ximaint.cpp
# End Source File
# Begin Source File

SOURCE=..\ximajas.cpp
# End Source File
# Begin Source File

SOURCE=..\ximajbg.cpp
# End Source File
# Begin Source File

SOURCE=..\ximajpg.cpp
# End Source File
# Begin Source File

SOURCE=..\ximalpha.cpp
# End Source File
# Begin Source File

SOURCE=..\ximalyr.cpp
# End Source File
# Begin Source File

SOURCE=..\ximamng.cpp
# End Source File
# Begin Source File

SOURCE=..\ximapal.cpp
# End Source File
# Begin Source File

SOURCE=..\ximapcx.cpp
# End Source File
# Begin Source File

SOURCE=..\ximapng.cpp
# End Source File
# Begin Source File

SOURCE=..\ximaraw.cpp
# End Source File
# Begin Source File

SOURCE=..\ximasel.cpp
# End Source File
# Begin Source File

SOURCE=..\ximaska.cpp
# End Source File
# Begin Source File

SOURCE=..\ximatga.cpp
# End Source File
# Begin Source File

SOURCE=..\ximath.cpp
# End Source File
# Begin Source File

SOURCE=..\ximatif.cpp
# End Source File
# Begin Source File

SOURCE=..\ximatran.cpp
# End Source File
# Begin Source File

SOURCE=..\ximawbmp.cpp
# End Source File
# Begin Source File

SOURCE=..\ximawmf.cpp
# End Source File
# Begin Source File

SOURCE=..\ximawnd.cpp
# End Source File
# Begin Source File

SOURCE=..\xmemfile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cximagemfcdll.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\xfile.h
# End Source File
# Begin Source File

SOURCE=..\ximabmp.h
# End Source File
# Begin Source File

SOURCE=..\ximadefs.h
# End Source File
# Begin Source File

SOURCE=..\ximage.h
# End Source File
# Begin Source File

SOURCE=..\ximagif.h
# End Source File
# Begin Source File

SOURCE=..\ximaico.h
# End Source File
# Begin Source File

SOURCE=..\ximaiter.h
# End Source File
# Begin Source File

SOURCE=..\ximajas.h
# End Source File
# Begin Source File

SOURCE=..\ximajpg.h
# End Source File
# Begin Source File

SOURCE=..\ximamng.h
# End Source File
# Begin Source File

SOURCE=..\ximapcx.h
# End Source File
# Begin Source File

SOURCE=..\ximapng.h
# End Source File
# Begin Source File

SOURCE=..\ximaska.h
# End Source File
# Begin Source File

SOURCE=..\ximatga.h
# End Source File
# Begin Source File

SOURCE=..\ximath.h
# End Source File
# Begin Source File

SOURCE=..\ximatif.h
# End Source File
# Begin Source File

SOURCE=..\ximawbmp.h
# End Source File
# Begin Source File

SOURCE=..\ximawmf.h
# End Source File
# Begin Source File

SOURCE=..\xiofile.h
# End Source File
# Begin Source File

SOURCE=..\xmemfile.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
