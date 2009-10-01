ROOT_PATH  = ../../../..
include Common.mak

NPT_INCLUDE = $(NEPTUNE_PATH)/Source/Config -I$(NEPTUNE_PATH)/Source/Core -I$(NEPTUNE_PATH)/Source/Neptune
PLT_INCLUDE = $(PLATINUM_PATH)/Source/Core -I$(PLATINUM_PATH)/Source/Devices/MediaServer
INCDIR		= $(DEVKIT_TOP)/include -I$(NPT_INCLUDE) -I$(PLT_INCLUDE) -I./
SRC_DIR		= ${PLATINUM_PATH}/Source

DBGFLAG		= -g
# OPTFLAG		= -O2
OPTFLAG		= 
CFLAGS		= -W -Wall $(DBGFLAG) $(OPTFLAG) -I$(INCDIR) 
CPPFLAGS	= -W -Wall $(DBGFLAG) $(OPTFLAG) -I$(INCDIR) -DPSP -DNPT_DEBUG -MMD
ASFLAGS		= $(DBGFLAG)
LDFLAGS		= $(DBGFLAG) -Wl,-export-dynamic -shared -mimpure-text
LDLIBS		= 
LOADLIBES	=

VPATH += ${SRC_DIR}/Core
VPATH += ${SRC_DIR}/Devices/MediaServer
PLT_SOURCES = \
			PltAction.cpp	\
			PltArgument.cpp	\
			PltCtrlPoint.cpp \
			PltCtrlPointTask.cpp \
			PltDatagramStream.cpp \
			PltDeviceData.cpp \
			PltDeviceHost.cpp \
			PltDownloader.cpp \
			PltEvent.cpp \
			PltHttpClientTask.cpp \
			PltHttpServer.cpp \
			PltHttpServerTask.cpp \
			PltMetadataHandler.cpp \
			PltScheduler.cpp \
			PltService.cpp \
			PltSsdp.cpp \
			PltStateVariable.cpp \
			PltTask.cpp \
			PltUPnP.cpp \
			PltXML.cpp

PLT_MS_SOURCES = \
			PltDidl.cpp \
			PltMediaBrowser.cpp \
			PltMediaItem.cpp \
			PltMediaPlaylist.cpp \
			PltMediaServer.cpp \
			PltMediaServerSCPDs.cpp

PLT_MS_TARGET = $(PLATINUM_MS)
PLT_MS_OBJS   = $(PLT_MS_SOURCES:.cpp=.o)

TARGET	 = $(PLATINUM)
OBJS	 = $(PLT_SOURCES:.cpp=.o) $(PLT_MS_SOURCES:.cpp=.o)

all:	deps $(TARGET).a

$(PLATINUM):	 $(TARGET).a

deps:
	make -C $(NEPTUNE_PATH)/Build/Neptune/Targets/mipsel-psp-linux $(NEPTUNE)
	
clean:
	rm -f *.o $(TARGET).a *~ *.bak PathDefs

$(TARGET).a: $(OBJS)
	$(RM) $@
	$(AR) r $@ $^
	$(RANLIB) $@

$(PLT_MS_TARGET).a: $(PLT_MS_OBJS)
	$(RM) $@
	$(AR) r $@ $^
	$(RANLIB) $@

#----------- rules --------------
-include $(OBJS:.o=.d)
-include PathDefs
PathDefs:
	psp-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
