#
#	Makefile for the bootstrap directory
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#
#	Note: we do not include make.dep here so we can bootstrap ourselves
#
BLD_TOP 		:= ../..
BUILD_CROSS		:= 0
EXPORT_OBJECTS	:= 0
MAKE_IFLAGS		:= -I..

#
#	Makefile.top always ensures the bootstrap is built. Prevent recursive calling.
#
IN_BOOTSTRAP	:= 1

all: compile

#
#	Read the build configuration settings and make variable definitions.
#
include $(BLD_TOP)/buildConfig.make

FILES = \
	$(BLD_OBJ_DIR)/dsi$(BLD_OBJ) 	\
	$(BLD_OBJ_DIR)/genDepend$(BLD_OBJ) 	\
	$(BLD_OBJ_DIR)/getopt$(BLD_OBJ) 	\
	$(BLD_OBJ_DIR)/incPatch$(BLD_OBJ) 	\
	$(BLD_OBJ_DIR)/pp$(BLD_OBJ)

ifeq ($BLD_BUILD_OS),WIN)
FILES += \
	$(BLD_OBJ_DIR)/dumpext$(BLD_OBJ) \
	$(BLD_OBJ_DIR)/removeFiles$(BLD_OBJ)
endif

#
#	Read the Makefile rules
#
include $(BLD_TOP)/build/make/make.rules
include $(BLD_TOP)/build/make/make.os.$(BLD_BUILD_OS)

BLD_FEATURE_STATIC	:= 1
BLD_FEATURE_SHARED	:= 0

TARGETS		:= 	$(BLD_TOOLS_DIR)/dsi$(BLD_EXE) \
				$(BLD_TOOLS_DIR)/genDepend$(BLD_EXE) \
				$(BLD_TOOLS_DIR)/incPatch$(BLD_EXE) \
				$(BLD_TOOLS_DIR)/pp$(BLD_EXE)

ifeq	($(BLD_HOST_OS),WIN)
TARGETS		+= 	$(BLD_TOOLS_DIR)/dumpext$(BLD_EXE)
TARGETS		+= 	$(BLD_TOOLS_DIR)/removeFiles$(BLD_EXE)
endif

compileExtra: $(TARGETS)

$(BLD_TOOLS_DIR)/dsi$(BLD_EXE): $(BLD_OBJ_DIR)/dsi$(BLD_OBJ) $(BLD_OBJ_DIR)/getopt$(BLD_OBJ)
	@bld --executable $(BLD_TOOLS_DIR)/dsi$(BLD_EXE) \
		dsi$(BLD_OBJ) getopt$(BLD_OBJ)

$(BLD_TOOLS_DIR)/dumpext$(BLD_EXE): $(BLD_OBJ_DIR)/dumpext$(BLD_OBJ) $(BLD_OBJ_DIR)/getopt$(BLD_OBJ)
	@bld --executable $(BLD_TOOLS_DIR)/dumpext$(BLD_EXE) \
		dumpext$(BLD_OBJ) getopt$(BLD_OBJ)

$(BLD_TOOLS_DIR)/genDepend$(BLD_EXE): $(BLD_OBJ_DIR)/genDepend$(BLD_OBJ) $(BLD_OBJ_DIR)/getopt$(BLD_OBJ)
	@bld --executable $(BLD_TOOLS_DIR)/genDepend$(BLD_EXE) \
		genDepend$(BLD_OBJ) getopt$(BLD_OBJ)

$(BLD_TOOLS_DIR)/incPatch$(BLD_EXE): $(BLD_OBJ_DIR)/incPatch$(BLD_OBJ) $(BLD_OBJ_DIR)/getopt$(BLD_OBJ)
	@bld --executable $(BLD_TOOLS_DIR)/incPatch$(BLD_EXE) \
		incPatch$(BLD_OBJ) getopt$(BLD_OBJ)

$(BLD_TOOLS_DIR)/removeFiles$(BLD_EXE): $(BLD_OBJ_DIR)/removeFiles$(BLD_OBJ)
	@bld --graphical --executable $(BLD_TOOLS_DIR)/removeFiles$(BLD_EXE) "removeFiles$(BLD_OBJ)"

$(BLD_TOOLS_DIR)/pp$(BLD_EXE): pp$(BLD_OBJ)
	@bld --executable $(BLD_TOOLS_DIR)/pp$(BLD_EXE) pp$(BLD_OBJ)

cleanExtra:
	rm -f *.manifest *.exe $(TARGETS) *.o *.pdb *.obj 2>/dev/null ; true


## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
