INCLUDES=-I. -I.. -I../../guilib -I../linux -I../utils -I../FileSystem -I../lib/libportaudio

.SUFFIXES : .m .mm

# do not include eprintf.cpp as make links to libgcc.a where as xcode does not.
SRCS = \
	SDLMain.mm \
	XBMCHelper.m \
	CocoaInterface.mm \
	CoreAudio.cpp \
	CoreAudioSoundManager.cpp \
	CocoaPowerSyscall.cpp \
	OSXGNUReplacements.c \
	ZeroconfOSX.cpp \
	ZeroconfBrowserOSX.cpp \
	WinSystemOSX.mm \
	WinSystemOSXGL.mm \
	
LIB=osx.a

include ../../Makefile.include
