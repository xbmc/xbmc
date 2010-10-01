# Microsoft Developer Studio Project File - Name="msvc6" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=msvc6 - Win32 Debug with Release lib
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msvc6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msvc6.mak" CFG="msvc6 - Win32 Debug with Release lib"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msvc6 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "msvc6 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "msvc6 - Win32 Debug with Release lib" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "msvc6 - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../../angelscript/include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x416 /d "NDEBUG"
# ADD RSC /l 0x416 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../../angelscript/lib/angelscript.lib /nologo /subsystem:console /machine:I386 /out:"../../bin/msvc6.exe"

!ELSEIF  "$(CFG)" == "msvc6 - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../../angelscript/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x416 /d "_DEBUG"
# ADD RSC /l 0x416 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../../angelscript/lib/angelscriptd.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/msvc6.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "msvc6 - Win32 Debug with Release lib"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "msvc6___Win32_Debug_with_Release_lib"
# PROP BASE Intermediate_Dir "msvc6___Win32_Debug_with_Release_lib"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "msvc6___Win32_Debug_with_Release_lib"
# PROP Intermediate_Dir "msvc6___Win32_Debug_with_Release_lib"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../angelscript/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../../angelscript/include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x416 /d "_DEBUG"
# ADD RSC /l 0x416 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../angelscript/lib/angelscriptd.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/msvc6.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../../angelscript/lib/angelscript.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../bin/msvc6.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "msvc6 - Win32 Release"
# Name "msvc6 - Win32 Debug"
# Name "msvc6 - Win32 Debug with Release lib"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\source\main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test2modules.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_2func.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_any.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_argref.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_array.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_arrayhandle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_arrayintf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_arrayobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_autohandle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_circularimport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_condition.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_config.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_configaccess.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_constobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_constproperty.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_constructor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_constructor2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_conversion.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_custommem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_debug.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_dict.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_discard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_dynamicconfig.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_exception.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_exceptionmemory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_float.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_funcoverload.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_generic.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_import.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_import2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_multiassign.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_nested.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_nevervisited.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_notinitialized.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_object.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_object2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_object3.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_objhandle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_objhandle2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_objzerosize.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_optimize.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_pointer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_refargument.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_return_with_cdecl_objfirst.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_returnstring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_saveload.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_scriptstring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_scriptstruct.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_stack2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_stdvector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_stream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_structintf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_suspend.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_unsaferef.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_vector3.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\test_vector3_2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testbstr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testbstr2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testcdecl_class.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testcdecl_class_a.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testcdecl_class_c.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testcdecl_class_d.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testcreateengine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testenumglobvar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute1arg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute2args.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute32args.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute32mixedargs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute4args.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecute4argsf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecutemixedargs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecutescript.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecutestring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testexecutethis32mixedargs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testglobalvar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testint64.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testlongtoken.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testmoduleref.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testmultipleinheritance.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testnegateoperator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testnotcomplexstdcall.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testnotcomplexthiscall.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testoutput.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testreturn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testreturnd.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testreturnf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\teststack.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\teststdcall4args.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\teststdstring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testswitch.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testtempvar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testvirtualinheritance.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\testvirtualmethod.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\angelscript\include\angelscript.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\source\bstr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\bstr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\add_on\scriptstring\scriptstring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\add_on\scriptstring\scriptstring.h
# End Source File
# Begin Source File

SOURCE=..\..\source\stdstring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\stdstring.h
# End Source File
# Begin Source File

SOURCE=..\..\source\stdvector.h
# End Source File
# Begin Source File

SOURCE=..\..\source\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\source\utils.h
# End Source File
# End Group
# End Target
# End Project
