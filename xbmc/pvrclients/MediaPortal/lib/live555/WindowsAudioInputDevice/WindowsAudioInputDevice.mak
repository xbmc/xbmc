INCLUDES = -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include
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

TOOLS32	=		c:\Program Files\DevStudio\Vc
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
LINK_OPTS_0 =		$(linkdebug) msvcirt.lib
LIBRARY_LINK_OPTS =	
LINK_OPTS =		$(LINK_OPTS_0) $(UI_OPTS)
CONSOLE_LINK_OPTS =	$(LINK_OPTS_0) $(CONSOLE_UI_OPTS)
SERVICE_LINK_OPTS =     kernel32.lib advapi32.lib shell32.lib -subsystem:console,$(APPVER)
LIB_SUFFIX =		lib
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
MULTIMEDIA_LIBS =	winmm.lib
EXE =			.exe

rc32 = "$(TOOLS32)\bin\rc"
.rc.res:
	$(rc32) $<
##### End of variables to change

WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB = libWindowsAudioInputDevice_noMixer.$(LIB_SUFFIX)
WINDOWSAUDIOINPUTDEVICE_MIXER_LIB = libWindowsAudioInputDevice_mixer.$(LIB_SUFFIX)
ALL = $(WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB) $(WINDOWSAUDIOINPUTDEVICE_MIXER_LIB) \
	showAudioInputPorts_noMixer$(EXE) showAudioInputPorts_mixer$(EXE)
all::	$(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB_OBJS = WindowsAudioInputDevice_common.$(OBJ) WindowsAudioInputDevice_noMixer.$(OBJ)
WINDOWSAUDIOINPUTDEVICE_MIXER_LIB_OBJS = WindowsAudioInputDevice_common.$(OBJ) WindowsAudioInputDevice_mixer.$(OBJ)

WindowsAudioInputDevice_common.$(CPP):	WindowsAudioInputDevice_common.hh
WindowsAudioInputDevice_noMixer.$(CPP):	WindowsAudioInputDevice_noMixer.hh
WindowsAudioInputDevice_noMixer.hh:		WindowsAudioInputDevice_common.hh
WindowsAudioInputDevice_mixer.$(CPP):	WindowsAudioInputDevice_mixer.hh
WindowsAudioInputDevice_mixer.hh:		WindowsAudioInputDevice_common.hh

$(WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB): $(WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB_OBJS) \
	$(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
	$(WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB_OBJS)
$(WINDOWSAUDIOINPUTDEVICE_MIXER_LIB): $(WINDOWSAUDIOINPUTDEVICE_MIXER_LIB_OBJS) \
	$(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
	$(WINDOWSAUDIOINPUTDEVICE_MIXER_LIB_OBJS)

USAGE_ENVIRONMENT_DIR = ../UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = ../BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(LIB_SUFFIX)
LIVEMEDIA_DIR = ../liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(LIB_SUFFIX)
GROUPSOCK_DIR = ../groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(LIB_SUFFIX)
LOCAL_LIBS =    $(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) \
                $(USAGE_ENVIRONMENT_LIB) $(BASIC_USAGE_ENVIRONMENT_LIB)
LOCAL_LIBS_NOMIXER =    $(WINDOWSAUDIOINPUTDEVICE_NOMIXER_LIB) $(LOCAL_LIBS)
LOCAL_LIBS_MIXER =    $(WINDOWSAUDIOINPUTDEVICE_MIXER_LIB) $(LOCAL_LIBS)
MULTIMEDIA_LIBS =       winmm.lib
LIBS_NOMIXER = $(LOCAL_LIBS_NOMIXER) $(LIBS_FOR_CONSOLE_APPLICATION) $(MULTIMEDIA_LIBS)
LIBS_MIXER = $(LOCAL_LIBS_MIXER) $(LIBS_FOR_CONSOLE_APPLICATION) $(MULTIMEDIA_LIBS)

SHOW_AUDIO_INPUT_PORTS_OBJS = showAudioInputPorts.$(OBJ)

showAudioInputPorts_noMixer$(EXE):	$(SHOW_AUDIO_INPUT_PORTS_OBJS) $(LOCAL_LIBS_NOMIXER)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(SHOW_AUDIO_INPUT_PORTS_OBJS) $(LIBS_NOMIXER)
showAudioInputPorts_mixer$(EXE):	$(SHOW_AUDIO_INPUT_PORTS_OBJS) $(LOCAL_LIBS_MIXER)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(SHOW_AUDIO_INPUT_PORTS_OBJS) $(LIBS_MIXER)

clean:
	-rm -rf *.$(OBJ) $(ALL) tcl2array$(EXE) core *.core *~
	-rm -rf $(TCL_EMBEDDED_CPLUSPLUS_FILES) $(TK_EMBEDDED_CPLUSPLUS_FILES) $(MISC_EMBEDDED_CPLUSPLUS_FILES)

##### Any additional, platform-specific rules come here:
