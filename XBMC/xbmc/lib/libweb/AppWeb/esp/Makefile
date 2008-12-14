#
#	Makefile for Embedded Server Pages (ESP)
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		:= -I../mpr -I../ejs

include 		make.dep

ifeq 			($(BLD_FEATURE_TEST),1)
POST_DIRS		:= test
endif

ifeq 			($(BLD_FEATURE_STATIC),1)
	TARGETS		+= $(BLD_LIB_DIR)/libespStatic$(BLD_ARCH)
endif

ifeq			($(BLD_FEATURE_ESP_MODULE),1)
compileExtra: 	$(TARGETS)
endif

$(BLD_LIB_DIR)/libespStatic$(BLD_ARCH): $(FILES)
	@bld --static --library $(BLD_LIB_DIR)/libespStatic \
		--objectsDir $(BLD_OBJ_DIR) --objectList files

cleanExtra:
	@rm -f $(TARGETS)

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
