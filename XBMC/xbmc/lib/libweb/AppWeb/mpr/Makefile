#
#	Makefile for the Mbedthis Portable Runtime (MPR) library
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

include 		make.dep

POST_DIRS		= package
ifeq 			($(BLD_FEATURE_TEST),1)
POST_DIRS		+= test
endif
ifeq 			($(BLD_FEATURE_LICENSE),1)
POST_DIRS		+= license
endif


ifeq 			($(BLD_UNIX),1)
PRE_DIRS		 = UNIX
else
PRE_DIRS		 = $(BLD_HOST_OS)
endif

ifeq 			($(BLD_FEATURE_STATIC),1)
	TARGETS		+= $(BLD_LIB_DIR)/libmprStatic$(BLD_ARCH)
	TARGETS		+= $(BLD_LIB_DIR)/libmprTestStatic$(BLD_ARCH)
endif
ifeq 			($(BLD_FEATURE_SHARED),1)
	TARGETS		+= $(BLD_LIB_DIR)/libmpr$(BLD_SHOBJ)
	TARGETS		+= $(BLD_LIB_DIR)/libmprTest$(BLD_SHOBJ)
endif

compileExtra: $(TARGETS)

#
#	Build a static MPR library
#
$(BLD_LIB_DIR)/libmprStatic$(BLD_ARCH): $(FILES) files \
		$(shell BLD_OBJ=$(BLD_OBJ) BLD_OBJ_DIR=$(BLD_OBJ_DIR) \
		eval echo `cat files`)
	@bld --c++ --static --library $(BLD_LIB_DIR)/libmprStatic \
		--objectsDir $(BLD_OBJ_DIR) --objectList files --resources mpr.rc

#
#	Build a shared MPR library
#
$(BLD_LIB_DIR)/libmpr$(BLD_SHOBJ): $(FILES) files
	@bld --c++ --shared --library $(BLD_LIB_DIR)/libmpr \
		--objectsDir $(BLD_OBJ_DIR) --objectList files --resources mpr.rc

#
#	Build a static MPR test library
#
$(BLD_LIB_DIR)/libmprTestStatic$(BLD_ARCH): $(BLD_OBJ_DIR)/test$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libmprStatic$(BLD_ARCH)
	@bld --static --library $(BLD_LIB_DIR)/libmprTestStatic \
		--objectsDir $(BLD_OBJ_DIR) "$(BLD_OBJ_DIR)/test$(BLD_OBJ)"

#
#	Build a shared MPR test library
#
$(BLD_LIB_DIR)/libmprTest$(BLD_SHOBJ): $(BLD_OBJ_DIR)/test$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libmpr$(BLD_SHOBJ)
	@bld --c++ --shared --library $(BLD_LIB_DIR)/libmprTest --libs "mpr" \
		--objectsDir $(BLD_OBJ_DIR) "$(BLD_OBJ_DIR)/test$(BLD_OBJ)"

cleanExtra:
	@rm -f $(TARGETS)

#
#	Must generate dependencies in all directories so that source distributions
#	will immediately work on all O/Ss.
#
dependExtra:
	@for dir in UNIX WIN VXWORKS ; \
	do \
		echo "  $(MAKE) -S $(MAKEFLAGS) -C $${dir} " ; \
		$(MAKE) --no-print-directory -S $(MAKECMDGOALS) \
			-C $${dir} ; \
	done

#
#	Generate Visual Studio Projects
#
projects: libmpr.dsp libmprTest.dsp

#
#	Only build shared libraries
#
libmpr.dsp: always
	@makeProj --shared --library libmpr --entry mprDllMain \
		--objectList files --resources mpr.rc \
		--headers "mpr.h mprOs.h" --sourceList sourceFiles

libmprTest.dsp: always
	@makeProj --shared --library libmprTest --sources "test.cpp" --libs "mpr" \
		--headers "test.h mprConfig.h" 

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
