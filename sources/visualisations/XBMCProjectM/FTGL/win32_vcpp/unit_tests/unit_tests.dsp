# Microsoft Developer Studio Project File - Name="unit_tests" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=unit_tests - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "unit_tests.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unit_tests.mak" CFG="unit_tests - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unit_tests - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "unit_tests - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unit_tests - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(MIC_COTS_CPPUNIT_DIR)/include" /I "../../include" /I "../../extras" /I "D:\cots\freetype-2.0.5\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "FTGL_LIBRARY_STATIC" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 freetype204MT.lib ftgl_static_MT.lib cppunit.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\Build" /libpath:"D:\cots\freetype-2.0.5\objs" /libpath:"D:\cots\cppunit-1.9.8\lib"

!ELSEIF  "$(CFG)" == "unit_tests - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(MIC_COTS_CPPUNIT_DIR)/include" /I "../../include" /I "../../extras" /I "D:\cots\freetype-2.0.5\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "FTGL_LIBRARY_STATIC" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 freetype204MT_D.lib ftgl_static_MT_d.lib cppunitd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\Build" /libpath:"D:\cots\freetype-2.0.5\objs" /libpath:"D:\cots\cppunit-1.9.8\lib"

!ENDIF 

# Begin Target

# Name "unit_tests - Win32 Release"
# Name "unit_tests - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\test\FTBBox-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTCharmap-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTContour-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTFace-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTFont-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTGlyphContainer-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTLibrary-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTList-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTMesh-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTPoint-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTSize-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTTesselation-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTVector-Test.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\test\FTVectoriser-Test.cpp"
# End Source File
# Begin Source File

SOURCE=..\..\test\HPGCalc_afm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\HPGCalc_pfb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\test\TestMain.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\test\Fontdefs.h
# End Source File
# End Group
# End Target
# End Project
