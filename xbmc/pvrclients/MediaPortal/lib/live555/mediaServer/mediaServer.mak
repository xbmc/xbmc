INCLUDES = -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include -I../BasicUsageEnvironment/include
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

MEDIA_SERVER = live555MediaServer$(EXE)

ALL = $(MEDIA_SERVER)
all: $(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

MEDIA_SERVER_OBJS = live555MediaServer.$(OBJ) DynamicRTSPServer.$(OBJ)

live555MediaServer,$(CPP):	DynamicRTSPServer.hh version.hh
DynamicRTSPServer.$(CPP):	DynamicRTSPServer.hh

USAGE_ENVIRONMENT_DIR = ../UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = ../BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(LIB_SUFFIX)
LIVEMEDIA_DIR = ../liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(LIB_SUFFIX)
GROUPSOCK_DIR = ../groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(LIB_SUFFIX)
LOCAL_LIBS =	$(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) \
		$(BASIC_USAGE_ENVIRONMENT_LIB) $(USAGE_ENVIRONMENT_LIB)
LIBS =			$(LOCAL_LIBS) $(LIBS_FOR_CONSOLE_APPLICATION)

live555MediaServer$(EXE):	$(MEDIA_SERVER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MEDIA_SERVER_OBJS) $(LIBS)

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

##### Any additional, platform-specific rules come here:
