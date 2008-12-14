#
# Makefile for the core http server 
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		:= -I. -I../mpr -Imodules -I../esp
PRE_DIRS		:= modules
POST_DIRS		:= utils

include 	make.dep

#
#	Targets to build
#
ifeq 			($(BLD_FEATURE_STATIC),1)
	TARGETS		+= $(BLD_LIB_DIR)/libappwebStatic$(BLD_ARCH)
endif
ifeq 			($(BLD_FEATURE_SHARED),1)
	TARGETS		+= $(BLD_LIB_DIR)/libappweb$(BLD_SHOBJ)
endif
TARGETS			+= $(BLD_BIN_DIR)/httpClient$(BLD_EXE)

#
#	Determine which modules must be put into the appweb library
#
ifeq	($(BLD_FEATURE_ADMIN_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/adminHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_AUTH_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/authHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_CGI_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/cgiHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_DIR_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/dirHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_COPY_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/copyHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_EGI_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/egiHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_ESP_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/espHandler$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/esp$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/espProcs$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/ejs$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/ejsCompat$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/ejsLex$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/ejsParser$(BLD_OBJ) \
						   $(BLD_OBJ_DIR)/ejsProcs$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_SSL_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/sslModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_C_API_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/capiModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_GACOMPAT_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/gaCompatModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/matrixSslModule$(BLD_OBJ)
	MAKE_IFLAGS			+= $(BLD_MATRIXSSL_IFLAGS)
	STATIC_LIBS			+= $(BLD_MATRIXSSL_LIBS)
	STATIC_LIBPATH 		+= $(BLD_MATRIXSSL_LIBPATH)
endif
ifeq	($(BLD_FEATURE_OPENSSL_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/openSslModule$(BLD_OBJ)
	MAKE_IFLAGS			+= $(BLD_OPENSSL_IFLAGS)
	STATIC_LIBS			+= $(BLD_OPENSSL_LIBS)
	STATIC_LIBPATH 		+= $(BLD_OPENSSL_LIBPATH)
endif
ifeq	($(BLD_FEATURE_PHP5_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/php5Handler$(BLD_OBJ)
	MAKE_IFLAGS			+= $(BLD_PHP5_IFLAGS)
	STATIC_LIBS			+= $(BLD_PHP5_LIBS)
	STATIC_LIBPATH 		+= $(BLD_PHP5_LIBPATH)
endif
ifeq	($(BLD_FEATURE_PUT_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/putHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_UPLOAD_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/uploadHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_XDB_MODULE_BUILTIN),1)
	STATIC_MODULES		+= $(BLD_OBJ_DIR)/xdbModule$(BLD_OBJ)
endif

#
#	Determine which modules must be put into the appwebStatic library
#
ifeq	($(BLD_FEATURE_ADMIN_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/adminHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_AUTH_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/authHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_CGI_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/cgiHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_DIR_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/dirHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_COPY_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/copyHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_EGI_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/egiHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_ESP_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/espHandler$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/esp$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/espProcs$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/ejs$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/ejsCompat$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/ejsLex$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/ejsParser$(BLD_OBJ) \
				   $(BLD_OBJ_DIR)/ejsProcs$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_SSL_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/sslModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_C_API_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/capiModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_GACOMPAT_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/gaCompatModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/matrixSslModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_OPENSSL_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/openSslModule$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_PHP5_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/php5Handler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_PUT_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/putHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_UPLOAD_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/uploadHandler$(BLD_OBJ)
endif
ifeq	($(BLD_FEATURE_XDB_MODULE),1)
	OBJS		+= $(BLD_OBJ_DIR)/xdbModule$(BLD_OBJ)
endif

#
#	Build the targets in this directory first then build all the dynamically
#	loaded modules in the modules directory.
#
compileExtra: $(TARGETS)
	@T=modules D=modules ; $(DO_RECURSE)

lib: $(TARGETS)

#
#	Always build the appweb static library with everything whether specified 
#	in config.make as static link or not.
#
$(BLD_LIB_DIR)/libappwebStatic$(BLD_ARCH): staticFiles \
	$(shell BLD_OBJ=$(BLD_OBJ) BLD_OBJ_DIR=$(BLD_OBJ_DIR) \
		eval echo `cat staticFiles`) $(OBJS)
	@bld --static --library $(BLD_LIB_DIR)/libappwebStatic \
		--objectsDir $(BLD_OBJ_DIR) --objectList staticFiles $(OBJS)

#
#	Build the appweb shared library only if required and only with core files
#	plus static modules.
#
$(BLD_LIB_DIR)/libappweb$(BLD_SHOBJ): sharedFiles $(FILES) \
	$(shell BLD_OBJ=$(BLD_OBJ) BLD_OBJ_DIR=$(BLD_OBJ_DIR) \
		eval echo `cat sharedFiles`)
	@bld --c++ --shared --versionSoname --library $(BLD_LIB_DIR)/libappweb \
		--libraryPath "$(STATIC_LIBPATH)" --libs "$(STATIC_LIBS)" \
		--objectsDir $(BLD_OBJ_DIR) --objectList sharedFiles \
		$(STATIC_MODULES)

#
#	Build the http client
#
$(BLD_BIN_DIR)/httpClient$(BLD_EXE): \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB) \
		$(BLD_OBJ_DIR)/httpClient$(BLD_OBJ) $(BLD_OBJ_DIR)/client$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/url$(BLD_OBJ) $(BLD_OBJ_DIR)/crypt$(BLD_OBJ)
	@bld --c++ --executable $(BLD_BIN_DIR)/httpClient$(BLD_EXE) \
		--preferStatic --libs "mpr" \
		--objectsDir $(BLD_OBJ_DIR) \
		--objects "$(BLD_OBJ_DIR)/httpClient$(BLD_OBJ) \
			$(BLD_OBJ_DIR)/client$(BLD_OBJ) $(BLD_OBJ_DIR)/url$(BLD_OBJ) \
			$(BLD_OBJ_DIR)/crypt$(BLD_OBJ)"

cleanExtra:
	rm -f httpClient$(BLD_EXE) $(TARGETS)
	rm -f trace.txt access.log error.log leak.log rom.files 

rom:
	find web -type f -print | grep -v CVS >rom.files
	find http.conf -type f -print >>rom.files
	httpComp rom.files >romFiles.cpp

#
#	Create the MS Visual Studio projects
#
projects: libappweb.dsp httpClient.dsp

#
#	Build static and shared
#
libappweb.dsp: always
	@makeProj --shared --library libappweb --objectList sharedFiles \
		--sourceList sourceFiles --headerList headerFiles

httpClient.dsp: always
	@makeProj --executable httpClient$(BLD_EXE) --libs "mpr" \
		--sources "httpClient.cpp client.cpp url.cpp crypt.cpp" \
		--headers "client.h shared.h"

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
