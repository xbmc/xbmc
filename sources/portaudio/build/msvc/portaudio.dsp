# Microsoft Developer Studio Project File - Name="portaudio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=portaudio - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "portaudio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "portaudio.mak" CFG="portaudio - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "portaudio - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "portaudio - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "portaudio - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_x86"
# PROP BASE Intermediate_Dir "Release_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\src\common" /I "..\..\include" /I ".\\" /I "..\..\src\os\win" /D "WIN32" /D "NDEBUG" /D "_USRDLL" /D "PA_ENABLE_DEBUG_OUTPUT" /D "_CRT_SECURE_NO_DEPRECATE" /D "PAWIN_USE_WDMKS_DEVICE_INFO" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib setupapi.lib /nologo /dll /machine:I386 /out:"./Release_x86/portaudio_x86.dll"

!ELSEIF  "$(CFG)" == "portaudio - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_x86"
# PROP BASE Intermediate_Dir "Debug_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\src\common" /I "..\..\include" /I ".\\" /I "..\..\src\os\win" /D "WIN32" /D "_DEBUG" /D "_USRDLL" /D "PA_ENABLE_DEBUG_OUTPUT" /D "_CRT_SECURE_NO_DEPRECATE" /D "PAWIN_USE_WDMKS_DEVICE_INFO" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib setupapi.lib /nologo /dll /debug /machine:I386 /out:"./Debug_x86/portaudio_x86.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "portaudio - Win32 Release"
# Name "portaudio - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\common\pa_allocation.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_converters.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_cpuload.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_debugprint.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_dither.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_front.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_process.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_skeleton.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\pa_stream.c
# End Source File
# End Group
# Begin Group "hostapi"

# PROP Default_Filter ""
# Begin Group "ASIO"

# PROP Default_Filter ""
# Begin Group "ASIOSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\common\asio.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\host\ASIOConvertSamples.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\host\asiodrivers.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\host\pc\asiolist.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\common\combase.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\common\debugmessage.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\asio\ASIOSDK\common\register.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\hostapi\asio\pa_asio.cpp
# ADD CPP /I "..\..\src\hostapi\asio\ASIOSDK\host" /I "..\..\src\hostapi\asio\ASIOSDK\host\pc" /I "..\..\src\hostapi\asio\ASIOSDK\common"
# End Source File
# End Group
# Begin Group "dsound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\hostapi\dsound\pa_win_ds.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hostapi\dsound\pa_win_ds_dynlink.c
# End Source File
# End Group
# Begin Group "wmme"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\hostapi\wmme\pa_win_wmme.c
# End Source File
# End Group
# Begin Group "wasapi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\hostapi\wasapi\pa_win_wasapi.cpp
# End Source File
# End Group
# Begin Group "wdm-ks"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\hostapi\wdmks\pa_win_wdmks.c
# End Source File
# End Group
# End Group
# Begin Group "os"

# PROP Default_Filter ""
# Begin Group "win"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\os\win\pa_win_hostapis.c
# End Source File
# Begin Source File

SOURCE=..\..\src\os\win\pa_win_util.c
# End Source File
# Begin Source File

SOURCE=..\..\src\os\win\pa_win_waveformat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\os\win\pa_win_wdmks_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\os\win\pa_x86_plain_converters.c
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\portaudio.def
# End Source File
# End Group
# End Target
# End Project
