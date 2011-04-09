INCLUDES = -Iinclude -I../groupsock/include
##### Change the following for your environment: 
# Comment out the following line to produce Makefiles that generate debuggable code:
NODEBUG=1

# The following definition ensures that we are properly matching
# the WinSock2 library file with the correct header files.
# (will link with "ws2_32.lib" and include "winsock2.h" & "Ws2tcpip.h")
TARGETOS = WINNT

# If for some reason you wish to use WinSock1 instead, uncomment the
# following two definitions.
# (will link with "wsock32.lib" and include "winsock.h")
#TARGETOS = WIN95
#APPVER = 4.0

!include    <ntwin32.mak>

UI_OPTS =		$(guilflags) $(guilibsdll)
# Use the following to get a console (e.g., for debugging):
CONSOLE_UI_OPTS =		$(conlflags) $(conlibsdll)
CPU=i386

TOOLS32	=		E:\Program Files\Microsoft Visual Studio 9.0\VC
COMPILE_OPTS =		$(INCLUDES) $(cdebug) $(cflags) $(cvarsdll) -I. -I"$(TOOLS32)\include"
C =			c
C_COMPILER =		"$(TOOLS32)\bin\cl"
C_FLAGS =		$(COMPILE_OPTS)
CPP =			cpp
CPLUSPLUS_COMPILER =	$(C_COMPILER)
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS)
OBJ =			obj
LINK =			$(link) -out:
LIBRARY_LINK =		lib -out:
LINK_OPTS_0 =		$(linkdebug) msvcrt.lib
LIBRARY_LINK_OPTS =	
LINK_OPTS =		$(LINK_OPTS_0) $(UI_OPTS)
CONSOLE_LINK_OPTS =	$(LINK_OPTS_0) $(CONSOLE_UI_OPTS)
SERVICE_LINK_OPTS =     kernel32.lib advapi32.lib shell32.lib -subsystem:console,$(APPVER)
LIB_SUFFIX =		lib
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
MULTIMEDIA_LIBS =	winmm.lib
EXE =			.exe
PLATFORM = Windows

rc32 = "$(TOOLS32)\bin\rc"
.rc.res:
	$(rc32) $<
##### End of variables to change

USAGE_ENVIRONMENT_LIB = libUsageEnvironment.$(LIB_SUFFIX)
ALL = $(USAGE_ENVIRONMENT_LIB)
all:	$(ALL)

OBJS = UsageEnvironment.$(OBJ) HashTable.$(OBJ) strDup.$(OBJ)

$(USAGE_ENVIRONMENT_LIB): $(OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) $(OBJS)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

UsageEnvironment.$(CPP):	include/UsageEnvironment.hh
include/UsageEnvironment.hh:	include/UsageEnvironment_version.hh include/strDup.hh
HashTable.$(CPP):		include/HashTable.hh
include/HashTable.hh:		include/Boolean.hh
strDup.$(CPP):			include/strDup.hh

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

##### Any additional, platform-specific rules come here:
