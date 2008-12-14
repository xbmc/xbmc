#
# Makefile for the GoAhead WebServer Compatibility module
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
NAME			:= compatModule

include 		make.dep

MODULES			:= $(BLD_MOD_DIR)/lib$(NAME)$(BLD_SHOBJ)
MAKE_IFLAGS		:= -I.. -I$(BLD_TOP)/http -I$(BLD_TOP)/mpr \
				   -I$(BLD_TOP)/mprServices -I$(BLD_TOP)/ejs -I$(BLD_TOP)/esp

#
#	Determine which modules need to be explicitly linked with when creating
#	the test module
#
ifeq	($(BLD_FEATURE_ESP_MODULE),1)
ifeq	($(BLD_FEATURE_ESP_MODULE_BUILTIN),0)
	ESP_LIB	+= espModule
endif
endif
ifeq	($(BLD_FEATURE_EGI_MODULE),1)
ifeq	($(BLD_FEATURE_EGI_MODULE_BUILTIN),0)
	EGI_LIB	+= egiModule
endif
endif

modules: $(MODULES) dummy

$(BLD_MOD_DIR)/lib$(NAME)$(BLD_SHOBJ): \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB) \
		$(BLD_OBJ_DIR)/gaCompatModule$(BLD_OBJ) \
		$(FILES)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/lib$(NAME) --soname \
		--libraryPath "$(BLD_MOD_DIR)" \
		--preferShared --libs "appweb $(ESP_LIB) $(EGI_LIB)" \
		$(BLD_OBJ_DIR)/gaCompatModule$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/emfdb$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/um$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsCompat$(BLD_OBJ)

cleanExtra:
	rm -f $(MODULES)

#
#	Create the MS Visual Studio project
#
projects: always
	@makeProj --shared --library lib$(NAME) \
		--libraryPath "$(BLD_TOP)/$(BLD_PHP4_DIR)/libs" \
		--libs "appweb $(BLD_PHP4_LIBS)" \
		--sources "$(NAME).cpp" --headers "compatApi.h $(NAME).h"

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
