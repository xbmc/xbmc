#
#	Makefile for Embedded Javascript (EJS)
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		:= -I../mpr 

include 		make.dep

ifeq 			($(BLD_FEATURE_TEST),1)
POST_DIRS		:= test
endif

ifeq 			($(BLD_FEATURE_STATIC),1)
	TARGETS		+= $(BLD_LIB_DIR)/libejsStatic$(BLD_ARCH)
endif
ifeq 			($(BLD_FEATURE_SHARED),1)
	TARGETS		+= $(BLD_LIB_DIR)/libejs$(BLD_SHOBJ)
endif

compileExtra: $(TARGETS)

$(BLD_LIB_DIR)/libejsStatic$(BLD_ARCH): $(FILES)
	@bld --static --library $(BLD_LIB_DIR)/libejsStatic \
		--objectsDir $(BLD_OBJ_DIR) --objectList files

$(BLD_LIB_DIR)/libejs$(BLD_SHOBJ): $(FILES)
	@bld --shared --library $(BLD_LIB_DIR)/libejs \
		--objectsDir $(BLD_OBJ_DIR) --objectList files --libs mpr

cleanExtra:
	@rm -f $(TARGETS)

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
