# Microsoft Developer Studio Project File - Name="static_library" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=static_library - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "static_library.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "static_library.mak" CFG="static_library - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "static_library - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "static_library - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "static_library - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /O2 /I "../../include/" /I "../../../../include/" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../lib/portaudiocpp-vc6-r.lib"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include/" /I "../../../../include/" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ  /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../lib/portaudiocpp-vc6-d.lib"

!ENDIF 

# Begin Target

# Name "static_library - Win32 Release"
# Name "static_library - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\source\portaudiocpp\AsioDeviceAdapter.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\BlockingStream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\CallbackInterface.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\CallbackStream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\CFunCallbackStream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\CppFunCallbackStream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\Device.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\DirectionSpecificStreamParameters.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\Exception.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\HostApi.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\InterfaceCallbackStream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\MemFunCallbackStream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\Stream.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\StreamParameters.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\System.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\SystemDeviceIterator.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\source\portaudiocpp\SystemHostApiIterator.cxx

!IF  "$(CFG)" == "static_library - Win32 Release"

!ELSEIF  "$(CFG)" == "static_library - Win32 Debug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\portaudiocpp\AsioDeviceAdapter.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\AutoSystem.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\BlockingStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\CallbackInterface.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\CallbackStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\CFunCallbackStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\CppFunCallbackStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\Device.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\DirectionSpecificStreamParameters.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\Exception.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\HostApi.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\InterfaceCallbackStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\MemFunCallbackStream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\PortAudioCpp.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\SampleDataFormat.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\Stream.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\StreamParameters.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\System.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\SystemDeviceIterator.hxx
# End Source File
# Begin Source File

SOURCE=..\..\include\portaudiocpp\SystemHostApiIterator.hxx
# End Source File
# End Group
# End Target
# End Project
