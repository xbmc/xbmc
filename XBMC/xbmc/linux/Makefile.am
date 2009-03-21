CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/guilib
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/utils -I$(top_srcdir)/xbmc/FileSystem
CPPFLAGS	+=	@SDL_CFLAGS@ @HAL_CFLAGS@
CFLAGS		+=	-fPIC # FIXME! Do we really need this?

noinst_LIBRARIES	=	liblinux.a
liblinux_a_SOURCES	=	ConvUtils.cpp \
				XEventUtils.cpp \
				XFileUtils.cpp \
				XHandle.cpp \
				XSyncUtils.cpp \
				XTimeUtils.cpp \
				XMemUtils.cpp \
				XThreadUtils.cpp \
				NetworkLinux.cpp \
				NetworkLinux.cpp \
				LinuxResourceCounter.cpp \
				LinuxTimezone.cpp \
				XRandR.cpp \
				LinuxFileSystem.cpp \
				XCriticalSection.cpp \
				XLCDproc.cpp \
				HalManager.cpp \
				DBusPowerSyscall.cpp

# vim: ts=8:sw=8:noexpandtab
