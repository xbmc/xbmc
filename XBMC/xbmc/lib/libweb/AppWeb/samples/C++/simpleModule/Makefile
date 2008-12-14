#
#	Makefile for the simpleModule sample
#
#	Copyright (c) Mbedthis LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
EXPORT_OBJECTS	:= 0
NAME			:= simpleModule

include 		make.dep

MAKE_IFLAGS		:= -I$(BLD_TOP)/appweb -I$(BLD_TOP)/http \
				   -I$(BLD_TOP)/http/modules -I$(BLD_TOP)/esp \
				   -I$(BLD_TOP)/http/modules/gaCompat \
				   -I$(BLD_TOP)/ejs -I$(BLD_TOP)/mpr

#
#	Must link with any builtin package libraries
#
ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE_BUILTIN),1)
	LIBS	+= $(BLD_MATRIXSSL_LIBS)
	LIBPATH += $(BLD_MATRIXSSL_DIR)
endif
ifeq	($(BLD_FEATURE_OPENSSL_MODULE_BUILTIN),1)
	LIBS	+= $(BLD_OPENSSL_LIBS)
	LIBPATH += $(BLD_OPENSSL_DIR)
endif
ifeq	($(BLD_FEATURE_PHP5_MODULE_BUILTIN),1)
	LIBS	+= $(BLD_PHP5_LIBS)
	LIBPATH += $(BLD_PHP5_DIR)
endif

compileExtra: $(NAME)$(BLD_LIB)

$(NAME)$(BLD_LIB): $(BLD_OBJ_DIR)/$(NAME)$(BLD_OBJ)
	@bld --c++ --shared --library $(NAME)  --libraryPath "$(LIBPATH)" \
		--libs "appweb $(LIBS)" $(BLD_OBJ_DIR)/$(NAME)$(BLD_OBJ)

cleanExtra:
	rm -f $(NAME)$(BLD_EXE) $(NAME)$(BLD_LIB)
