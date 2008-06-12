# Microsoft Developer Studio Project File - Name="cnv_FAAD" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cnv_FAAD - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cnv_FAAD.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cnv_FAAD.mak" CFG="cnv_FAAD - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cnv_FAAD - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cnv_FAAD - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cnv_FAAD - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cnv_FAAD___Win32_Release"
# PROP BASE Intermediate_Dir "cnv_FAAD___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CNV_FAAD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../common/faad" /I "../../common/mp4v2" /I "../../include" /I "SDK" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CNV_FAAD_EXPORTS" /D "WIN32_LEAN_AND_MEAN" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "NDEBUG"
# ADD RSC /l 0x410 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /machine:I386 /out:"Release/cnv_FAAD.wac"

!ELSEIF  "$(CFG)" == "cnv_FAAD - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cnv_FAAD___Win32_Debug"
# PROP BASE Intermediate_Dir "cnv_FAAD___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CNV_FAAD_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../faad2/common/faad" /I "../../../faad2/common/mp4v2" /I "../../common/faad" /I "../../include" /I "../../common/mp4v2" /I "SDK" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CNV_FAAD_EXPORTS" /D "WIN32_LEAN_AND_MEAN" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x410 /d "_DEBUG"
# ADD RSC /l 0x410 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Programmi\Winamp3\Wacs\cnv_FAAD.wac" /pdbtype:sept
# SUBTRACT LINK32 /force

!ENDIF 

# Begin Target

# Name "cnv_FAAD - Win32 Release"
# Name "cnv_FAAD - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "wa3sdk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SDK\studio\assert.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\attribs\attribute.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\attribs\attrstr.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\attribs\cfgitemi.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\studio\corecb.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\depend.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\encodedstr.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\foreach.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\memblock.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\common\nsGUID.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\util\pathparse.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\ptrlist.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\studio\services\servicei.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\std.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\string.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\bfc\svc_enum.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\studio\services\svc_mediaconverter.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\studio\services\svc_textfeed.cpp
# End Source File
# Begin Source File

SOURCE=.\SDK\studio\waclient.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\cnv_FAAD.cpp
# End Source File
# Begin Source File

SOURCE=.\CRegistry.cpp
# End Source File
# Begin Source File

SOURCE=.\FAAD.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cnv_FAAD.h
# End Source File
# Begin Source File

SOURCE=.\CRegistry.h
# End Source File
# Begin Source File

SOURCE=.\Defines.h
# End Source File
# Begin Source File

SOURCE=.\faadwa3.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
