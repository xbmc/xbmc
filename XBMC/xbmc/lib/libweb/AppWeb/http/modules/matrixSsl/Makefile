#
# Makefile for the MatrixSsl handler
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		= -I.. -I../.. -I../../../mpr -I../../../esp $(BLD_MATRIXSSL_IFLAGS)

include 		make.dep

ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE),1)
ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/libmatrixSslModule$(BLD_LIB)
endif
endif

ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE),1)
	GETDLL		+= getdll
endif

ifeq	($(BLD_FEATURE_SSL_MODULE_BUILTIN),1)
	STATIC_MODULES	+= $(BLD_OBJ_DIR)/sslModule$(BLD_OBJ)
else
	MODULE_LIBS		+= sslModule
endif

modules: $(MODULES) dummy

compileExtra: $(GETDLL)

#
#	Linux will do a static link with openssl, windows will dynamic link
#
$(BLD_MOD_DIR)/libmatrixSslModule$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/matrixSslModule$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/libmatrixSslModule --soname \
		--rpath "$(BLD_MOD_PREFIX)" \
		--libraryPath "$(BLD_LIB_DIR)" --libraryPath "$(BLD_MOD_DIR)" \
		--libraryPath $(BLD_MATRIXSSL_LIBPATH) --preferShared \
		--libs "appweb $(MODULE_LIBS) appweb $(BLD_MATRIXSSL_LIBS)" \
		--objectsDir "$(BLD_LIB_DIR)" \
		--objects "$(BLD_OBJ_DIR)/matrixSslModule$(BLD_OBJ) $(STATIC_MODULES)"

getdll: $(BLD_MOD_DIR)/libmatrixssl$(BLD_SHOBJ)

$(BLD_MOD_DIR)/libmatrixssl$(BLD_SHOBJ): \
	$(BLD_MATRIXSSL_LIBPATH)/libmatrixssl$(BLD_SHOBJ)
	cp $(BLD_MATRIXSSL_LIBPATH)/libmatrixssl* $(BLD_MOD_DIR)
	chmod 755 $(BLD_MOD_DIR)/libmatrixssl*

cleanExtra:
	rm -f $(MODULES)

projects: always
	makeProj --shared --library libmatrixSslModule \
		--libraryPath $(BLD_MATRIXSSL_LIBPATH) \
		--libs "appweb sslModule appweb $(BLD_MATRIXSSL_LIBS)" \
		--sources "matrixSslModule.cpp" --headers "matrixSslModule.h"
	
## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
