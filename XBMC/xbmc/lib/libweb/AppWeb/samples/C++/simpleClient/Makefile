#
#	Makefile for the simpleClient sample
#
#	Copyright (c) Mbedthis LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
EXPORT_OBJECTS	:= 0
NAME			:= simpleClient

include 		make.dep

MAKE_IFLAGS		:= -I$(BLD_TOP)/appweb -I$(BLD_TOP)/http \
				   -I$(BLD_TOP)/http/modules -I$(BLD_TOP)/esp \
				   -I$(BLD_TOP)/ejs -I$(BLD_TOP)/mpr \
				   -I$(BLD_TOP)/http/modules/gaCompat

compileExtra: $(NAME)$(BLD_EXE)

$(NAME)$(BLD_EXE): $(BLD_LIB_DIR)/libappweb$(BLD_LIB) \
		$(BLD_OBJ_DIR)/$(NAME)$(BLD_OBJ)
	@bld --c++ --executable $(NAME)$(BLD_EXE) --libs "appwebStatic" \
		$(BLD_OBJ_DIR)/$(NAME)$(BLD_OBJ)

run:
	./$(NAME)$(BLD_EXE)

cleanExtra:
	@echo "    rm -f $(NAME)$(BLD_EXE)"
	@rm -f $(NAME)$(BLD_EXE) $(NAME)

projects:
	@MAKE_IFLAGS="-I../../../include" \
	makeProj --executable $(NAME)${BLD_EXE} --exeBaseDir "." \
		--sources "*.cpp" --libraryPath ../../../bin --libs "appwebStatic"

