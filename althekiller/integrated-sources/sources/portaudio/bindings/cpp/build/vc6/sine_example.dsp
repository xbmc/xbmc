# Microsoft Developer Studio Project File - Name="sine_example" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sine_example - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sine_example.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sine_example.mak" CFG="sine_example - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sine_example - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "sine_example - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sine_example - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /O2 /I "../../include/" /I "../../../../include/" /I "../../../../src/common/" /I "../../../../../asiosdk2/common/" /I "../../../../../asiosdk2/host/" /I "../../../../../asiosdk2/host/pc/" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 portaudiocpp-vc6-r.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/sine_example.exe" /libpath:"../../lib"

!ELSEIF  "$(CFG)" == "sine_example - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include/" /I "../../../../include/" /I "../../../../src/common/" /I "../../../../../asiosdk2/common/" /I "../../../../../asiosdk2/host/" /I "../../../../../asiosdk2/host/pc/" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 portaudiocpp-vc6-d.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/sine_example.exe" /pdbtype:sept /libpath:"../../lib"

!ENDIF 

# Begin Target

# Name "sine_example - Win32 Release"
# Name "sine_example - Win32 Debug"
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\pa_asio\iasiothiscallresolver.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_allocation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_converters.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_cpuload.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_dither.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_endianness.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_hostapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_process.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_stream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_trace.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\hostapi\dsound\pa_win_ds_dynlink.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\os\win\pa_x86_plain_converters.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Example Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\example\sine.cxx
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\pa_asio\iasiothiscallresolver.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_allocation.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\pa_asio\pa_asio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_converters.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_cpuload.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_dither.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_front.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_process.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_skeleton.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_stream.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\common\pa_trace.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\hostapi\dsound\pa_win_ds.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\hostapi\dsound\pa_win_ds_dynlink.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\os\win\pa_win_hostapis.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\os\win\pa_win_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\hostapi\wasapi\pa_win_wasapi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\hostapi\wmme\pa_win_wmme.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\os\win\pa_x86_plain_converters.c
# End Source File
# End Group
# Begin Group "ASIO 2 SDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\..\asiosdk2\common\asio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\asiosdk2\host\asiodrivers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\asiosdk2\host\pc\asiolist.cpp
# End Source File
# End Group
# End Target
# End Project
