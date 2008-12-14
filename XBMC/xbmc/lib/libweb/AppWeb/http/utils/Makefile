#
# Makefile for utilities for the http web server 
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

MAKE_IFLAGS		:= -I.. -I../../mpr -I../../esp

#
#	Explicitly get our configuration so we can set BUILD_CROSS if vxworks
#
include 		../../buildConfig.make

ifeq			($(BLD_OS),VXWORKS)
BUILD_CROSS		:= 0
endif

include 		make.dep

ifeq			($(BLD_FEATURE_STDCPP),0)
	STDCPP		:= $(BLD_OBJ_DIR)/stdcpp$(BLD_OBJ)
endif

compileExtra: \
	$(BLD_BIN_DIR)/charGen$(BLD_EXE) \
	$(BLD_BIN_DIR)/httpComp$(BLD_EXE) \
	$(BLD_BIN_DIR)/httpPassword$(BLD_EXE)

$(BLD_BIN_DIR)/charGen$(BLD_EXE): $(BLD_OBJ_DIR)/charGen$(BLD_OBJ)
	@bld --c++ --executable $(BLD_BIN_DIR)/charGen$(BLD_EXE) \
		--preferStatic --objects $(BLD_OBJ_DIR)/charGen$(BLD_OBJ) $(STDCPP)

$(BLD_BIN_DIR)/httpComp$(BLD_EXE): $(BLD_OBJ_DIR)/httpComp$(BLD_OBJ)
	@bld --c++ --executable $(BLD_BIN_DIR)/httpComp$(BLD_EXE) \
		--preferStatic --objects $(BLD_OBJ_DIR)/httpComp$(BLD_OBJ) $(STDCPP)

$(BLD_BIN_DIR)/httpPassword$(BLD_EXE): $(BLD_OBJ_DIR)/httpPassword$(BLD_OBJ)
	@bld --c++ --executable $(BLD_BIN_DIR)/httpPassword$(BLD_EXE) \
		--preferStatic --objects $(BLD_OBJ_DIR)/httpPassword$(BLD_OBJ) $(STDCPP)

cleanExtra:
	rm -f $(BLD_BIN_DIR)/charGen$(BLD_EXE) 
	rm -f $(BLD_BIN_DIR)/httpComp$(BLD_EXE) 
	rm -f $(BLD_BIN_DIR)/httpPassword$(BLD_EXE)

projects: charGen.dsp httpComp.dsp httpPassword.dsp

charGen.dsp: always
	@makeProj --executable charGen$(BLD_EXE) --libs "appweb" --sources "charGen.cpp"

httpComp.dsp: always
	@makeProj --executable httpComp$(BLD_EXE) --libs "appweb" --sources "httpComp.cpp"

httpPassword.dsp: always
	@makeProj --executable httpPassword$(BLD_EXE) --libs "appweb" --sources "httpPassword.cpp"

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
