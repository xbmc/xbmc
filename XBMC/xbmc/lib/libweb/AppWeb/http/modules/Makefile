#
# Makefile for the http modules
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		:= -I.. -I../../mpr -I../../ejs -I../../esp 

ADMIN_MODULE	:= libadminModule
ASPNET_MODULE	:= libaspnetModule
AUTH_MODULE		:= libauthModule
C_API_MODULE	:= libcapiModule
CGI_MODULE		:= libcgiModule
COPY_MODULE		:= libcopyModule
PUT_MODULE		:= libputModule
DIR_MODULE		:= libdirModule
EGI_MODULE		:= libegiModule
ESP_MODULE		:= libespModule
SSL_MODULE		:= libsslModule
UPLOAD_MODULE	:= libuploadModule
XDB_MODULE		:= libxdbModule

include 		make.dep

ifeq	($(BLD_FEATURE_ASPNET_MODULE),1)
POST_DIRS		+= aspnet 
endif
ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE),1)
POST_DIRS		+= matrixSsl 
endif
ifeq	($(BLD_FEATURE_OPENSSL_MODULE),1)
POST_DIRS		+= openSsl 
endif
ifeq	($(BLD_FEATURE_PHP5_MODULE),1)
POST_DIRS		+= php5 
endif
ifeq	($(BLD_FEATURE_CONSOLE_MODULE),1)
POST_DIRS		+= console 
endif
ifeq	($(BLD_FEATURE_GACOMPAT_MODULE),1)
POST_DIRS		+= gaCompat 
endif

#
#	Order matters. Dependencies must be earlier in the list
#
ifeq	($(BLD_FEATURE_AUTH_MODULE),1)
ifeq	($(BLD_FEATURE_AUTH_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(AUTH_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_ADMIN_MODULE),1)
ifeq	($(BLD_FEATURE_ADMIN_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(ADMIN_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_CGI_MODULE),1)
ifeq	($(BLD_FEATURE_CGI_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(CGI_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_DIR_MODULE),1)
ifeq	($(BLD_FEATURE_DIR_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(DIR_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_COPY_MODULE),1)
ifeq	($(BLD_FEATURE_COPY_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(COPY_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_EGI_MODULE),1)
ifeq	($(BLD_FEATURE_EGI_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(EGI_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_ESP_MODULE),1)
ifeq	($(BLD_FEATURE_ESP_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(ESP_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_SSL_MODULE),1)
ifeq	($(BLD_FEATURE_SSL_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(SSL_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_C_API_MODULE),1)
ifeq	($(BLD_FEATURE_C_API_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(C_API_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_ASPNET_MODULE),1)
ifeq	($(BLD_FEATURE_ASPNET_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(ASPNET_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_UPLOAD_MODULE),1)
ifeq	($(BLD_FEATURE_UPLOAD_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(UPLOAD_MODULE)$(BLD_SHOBJ)
endif
endif

ifeq	($(BLD_FEATURE_XDB_MODULE),1)
ifeq	($(BLD_FEATURE_XDB_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(XDB_MODULE)$(BLD_SHOBJ)
endif
endif
ifeq	($(BLD_FEATURE_PUT_MODULE),1)
ifeq	($(BLD_FEATURE_PUT_MODULE_LOADABLE),1)
	MODULES		+= $(BLD_MOD_DIR)/$(PUT_MODULE)$(BLD_SHOBJ)
endif
endif

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

#
#	This target is called manually from ../Makefile once the appweb library is 
#	built. 
#
modules: $(MODULES)
	@T=modules ; D="$(POST_DIRS)" ; $(DO_RECURSE) 

#
#	Build Modules -- Always build all modules -- even if statically linking
#
$(BLD_MOD_DIR)/$(AUTH_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/authHandler$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(AUTH_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/authHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(CGI_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/cgiHandler$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(CGI_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/cgiHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(C_API_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/capiModule$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(C_API_MODULE) --soname \
		--libraryPath "$(BLD_MOD_DIR)" \
		--preferShared --libs "$(ESP_LIB) $(EGI_LIB)" \
		--libs "appweb" $(BLD_OBJ_DIR)/capiModule$(BLD_OBJ)

$(BLD_MOD_DIR)/$(ADMIN_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/adminHandler$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(ADMIN_MODULE) --soname \
		--preferShared --libs "appweb" \
		$(BLD_OBJ_DIR)/adminHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(DIR_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/dirHandler$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(DIR_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/dirHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(EGI_MODULE)$(BLD_SHOBJ): $(BLD_OBJ_DIR)/egiHandler$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(EGI_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/egiHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(ESP_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/espHandler$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/esp$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/espProcs$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejs$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsLex$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsParser$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsProcs$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsCompat$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(ESP_MODULE) --soname \
		--preferShared --libs "appweb" \
		$(BLD_OBJ_DIR)/espHandler$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/esp$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/espProcs$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejs$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsLex$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsParser$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsProcs$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/ejsCompat$(BLD_OBJ)

$(BLD_MOD_DIR)/$(ASPNET_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/aspnetHandler$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(ASPNET_MODULE) --soname \
		--preferShared --libs "appweb" \
		$(BLD_OBJ_DIR)/aspnetHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(COPY_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/copyHandler$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(COPY_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/copyHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(PUT_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/putHandler$(BLD_OBJ) $(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(PUT_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/putHandler$(BLD_OBJ)

$(BLD_MOD_DIR)/$(SSL_MODULE)$(BLD_SHOBJ): $(BLD_OBJ_DIR)/sslModule$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(SSL_MODULE) --soname \
		--preferShared --libs "appweb" $(BLD_OBJ_DIR)/sslModule$(BLD_OBJ)

$(BLD_MOD_DIR)/$(UPLOAD_MODULE)$(BLD_SHOBJ): \
		$(BLD_OBJ_DIR)/uploadHandler$(BLD_OBJ) \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB)
	@bld --c++ --shared --library $(BLD_MOD_DIR)/$(UPLOAD_MODULE) --soname \
		--preferShared --libs "appweb" \
		$(BLD_OBJ_DIR)/uploadHandler$(BLD_OBJ)

cleanExtra:
	rm -f $(MODULES)

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
