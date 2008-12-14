#
# Makefile for the OpenSSL handler
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		= -I.. -I../.. -I../../../mpr -I../../../esp $(BLD_OPENSSL_IFLAGS)

include 		make.dep

ifeq	($(BLD_FEATURE_OPENSSL_MODULE),1)
ifeq	($(BLD_FEATURE_OPENSSL_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/libopenSslModule$(BLD_LIB)
endif
endif

ifeq	($(BLD_FEATURE_OPENSSL_MODULE),1)
	GETDLL		+= getdll
endif

ifeq	($(BLD_FEATURE_SSL_MODULE_BUILTIN),1)
	STATIC_MODULES	+= $(BLD_OBJ_DIR)/sslModule$(BLD_OBJ)
else
	MODULE_LIBS	+= sslModule
endif

modules: $(MODULES) dummy

compileExtra: $(GETDLL)

#
#	Linux will do a static link with openssl, windows will dynamic link
#
$(BLD_MOD_DIR)/libopenSslModule$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/openSslModule$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/libopenSslModule --soname \
		--rpath "$(BLD_MOD_PREFIX)" \
		--libraryPath "$(BLD_LIB_DIR)" --libraryPath "$(BLD_MOD_DIR)" \
		--libraryPath "$(BLD_OPENSSL_LIBPATH)" --preferShared \
		--libs "appweb $(MODULE_LIBS) appweb $(BLD_OPENSSL_LIBS)" \
		--objectsDir "$(BLD_LIB_DIR)" \
		--objects "$(BLD_OBJ_DIR)/openSslModule$(BLD_OBJ) $(STATIC_MODULES)"

ifeq ($(BLD_HOST_OS),WIN)
getdll: $(BLD_MOD_DIR)/libeay32.dll $(BLD_MOD_DIR)/ssleay32.dll

$(BLD_MOD_DIR)/libeay32.dll: $(BLD_OPENSSL_LIBPATH)/libeay32.dll
	cp $(BLD_OPENSSL_LIBPATH)/libeay32.dll* $(BLD_MOD_DIR)
	chmod 755 $(BLD_MOD_DIR)/libeay32.dll*

$(BLD_MOD_DIR)/ssleay32.dll: $(BLD_OPENSSL_LIBPATH)/ssleay32.dll
	cp $(BLD_OPENSSL_LIBPATH)/ssleay32.dll* $(BLD_MOD_DIR)
	chmod 755 $(BLD_MOD_DIR)/ssleay32.dll*

else
getdll: $(BLD_MOD_DIR)/libcrypto$(BLD_SHLIB) \
		$(BLD_MOD_DIR)/libssl$(BLD_SHLIB)

$(BLD_MOD_DIR)/libcrypto$(BLD_SHLIB): $(BLD_OPENSSL_DIR)/libcrypto$(BLD_SHLIB)
	cp $(BLD_OPENSSL_DIR)/libcrypto*$(BLD_SHLIB) $(BLD_MOD_DIR)
	chmod 755 $(BLD_MOD_DIR)/libcrypto*$(BLD_SHLIB)
	@if [ "$(BLD_HOST_OS)" = "LINUX" -a -f /etc/redhat-release -a -x /usr/bin/chcon ] ; then \
		if [ "`sestatus | grep enabled`" != "" ]  ; then \
			echo chcon /usr/bin/chcon -t texrel_shlib_t $(BLD_MOD_DIR)/libcrypto.so; \
			priv chcon /usr/bin/chcon -t texrel_shlib_t $(BLD_MOD_DIR)/libcrypto.so; \
		fi ; \
	fi
	[ -x /sbin/ldconfig ] && ldconfig -n $(BLD_MOD_DIR) ; true

$(BLD_MOD_DIR)/libssl$(BLD_SHLIB): $(BLD_OPENSSL_DIR)/libssl$(BLD_SHLIB)
	cp $(BLD_OPENSSL_DIR)/libssl*$(BLD_SHLIB) $(BLD_MOD_DIR)
	chmod 755 $(BLD_MOD_DIR)/libssl*$(BLD_SHLIB)
	@if [ "$(BLD_HOST_OS)" = "LINUX" -a -f /etc/redhat-release -a -x /usr/bin/chcon ] ; \
	then \
		if [ "`sestatus | grep enabled`" != "" ] ; then \
			echo chcon /usr/bin/chcon -t texrel_shlib_t $(BLD_MOD_DIR)/libssl.so; \
			priv chcon /usr/bin/chcon -t texrel_shlib_t $(BLD_MOD_DIR)/libssl.so; \
		fi ; \
	fi
	[ -x /sbin/ldconfig ] && ldconfig -n $(BLD_MOD_DIR) ; true
endif

cleanExtra:
	rm -f $(MODULES)

projects: always
	@makeProj --shared --library libopenSslModule \
		--libraryPath $(BLD_OPENSSL_LIBPATH) \
		--libs "appweb sslModule appweb $(BLD_OPENSSL_LIBS)" \
		--sources "openSslModule.cpp" --headers "openSslModule.h"
	
#
#	This should be done for OpenSSL for each new application to generate 
#	new DH parameters
#
opensslParams:
	openssl dhparam -out dh512.pem -C -2 512 >openSslDh.h
	openssl dhparam -out dh1024.pem -C -2 1024 >>openSslDh.h

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
