#
# Makefile for the PHP handler
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_CFLAGS		:= -DPHP5=1
MAKE_IFLAGS		= -I../.. -I../../../mpr -I../../../esp $(BLD_PHP5_IFLAGS) 

include 		make.dep

ifeq	($(BLD_FEATURE_PHP5_MODULE),1)
ifeq	($(BLD_FEATURE_PHP5_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/libphp5Module$(BLD_SHOBJ)
endif
endif

ifeq	($(BLD_FEATURE_PHP5_MODULE),1)
	GETDLL		+= getPhp5Dll
endif

ifeq	($(BLD_HOST_OS),WIN)
MAKE_CFLAGS		+= -DPHP5_WIN32=1
endif

modules: $(MODULES) dummy

compileExtra: $(GETDLL)

$(BLD_MOD_DIR)/libphp5Module$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/php5Handler$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/libphp5Module --soname \
		--rpath "$(BLD_MOD_PREFIX)" \
		--libraryPath "$(BLD_PHP5_LIBPATH)" \
		--preferShared --libs "appweb $(BLD_PHP5_LIBS)" \
		$(BLD_OBJ_DIR)/php5Handler$(BLD_OBJ)

ifeq ($(BLD_HOST_OS),WIN)
ifeq ($(BLD_FEATURE_MULTITHREAD),1)
PLIB	:= php5ts
else
PLIB	:= php5
endif

getPhp5Dll: $(BLD_MOD_DIR)/$(PLIB).dll

$(BLD_MOD_DIR)/$(PLIB).dll: $(BLD_PHP5_LIBPATH)/$(PLIB).dll
	cp $(BLD_PHP5_LIBPATH)/$(PLIB).dll $(BLD_MOD_DIR)
	if [ -f $(BLD_PHP5_LIBPATH)/$(PLIB).dll.manifest ] ; then \
		cp $(BLD_PHP5_LIBPATH)/$(PLIB).dll.manifest $(BLD_MOD_DIR) ; \
	fi
	chmod 755 $(BLD_MOD_DIR)/$(PLIB).dll *

else
getPhp5Dll: $(BLD_MOD_DIR)/libphp5$(BLD_SHLIB)

$(BLD_MOD_DIR)/libphp5$(BLD_SHLIB): $(BLD_PHP5_LIBPATH)/libphp5$(BLD_SHLIB) 
	cp $(BLD_PHP5_LIBPATH)/libphp5* $(BLD_MOD_DIR)
	chmod 755 $(BLD_MOD_DIR)/libphp5*
	@if [ "$(BLD_HOST_OS)" = "LINUX" -a -f /etc/redhat-release -a -x /usr/bin/chcon ] ; then \
		if [ "`sestatus | grep enabled`" != "" ] ; then \
			echo chcon /usr/bin/chcon -t texrel_shlib_t $(BLD_MOD_DIR)/libphp5$(BLD_SHLIB); \
			priv chcon /usr/bin/chcon -t texrel_shlib_t $(BLD_MOD_DIR)/libphp5$(BLD_SHLIB); \
		fi ; \
	fi

endif
cleanExtra:
	rm -f $(MODULES)

#
#	Create the MS Visual Studio project
#
projects: always
	@makeProj --shared --library libphp5Module \
		--libraryPath "$(BLD_PHP5_LIBPATH)" \
		--libs "appweb $(BLD_PHP5_LIBS)" \
		--sources "php5Handler.cpp" --headers "php5Handler.h"

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
