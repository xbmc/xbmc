#
# Makefile for the Appweb server 
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

BUILD_NATIVE	:= 0
MAKE_IFLAGS		:= -I../mpr -I../ejs -I../esp -I../http -I../http/modules -I../http/modules/gaCompat
POST_DIRS		:= package

include 	make.dep 


ifeq	($(BLD_FEATURE_TEST),1)
PRE_DIRS	:= test
endif

#	#
#	#	If STATIC is defined, then we are building $(BLD_PRODUCT)Static so we 
#	#	must use builtin module headers.
#	#
#	ifeq	($(BLD_FEATURE_STATIC),1)
#	ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE),1)
#	COMPILE_MATRIXSSL	:= 1
#	endif
#	ifeq	($(BLD_FEATURE_OPENSSL_MODULE),1)
#	COMPILE_OPENSSL		:= 1
#	endif
#	ifeq	($(BLD_FEATURE_PHP5_MODULE),1)
#	COMPILE_PHP5		:= 1
#	endif
#	endif

#
#	If a module is builtin, then we must compile appwebStaticLink with 
#	IFLAGS set to include that modules headers.
#
ifeq	($(BLD_FEATURE_MATRIXSSL_MODULE_BUILTIN),1)
COMPILE_MATRIXSSL	:= 1
endif
ifeq	($(BLD_FEATURE_OPENSSL_MODULE_BUILTIN),1)
COMPILE_OPENSSL		:= 1
endif
ifeq	($(BLD_FEATURE_PHP5_MODULE_BUILTIN),1)
COMPILE_PHP5		:= 1
endif

ifeq	($(COMPILE_MATRIXSSL),1)
	MAKE_IFLAGS		+= $(BLD_MATRIXSSL_IFLAGS)
	STATIC_LIBS		+= $(BLD_MATRIXSSL_LIBS)
	STATIC_LIBPATH 	+= $(BLD_MATRIXSSL_LIBPATH)
endif
ifeq	($(COMPILE_OPENSSL),1)
	MAKE_IFLAGS		+= $(BLD_OPENSSL_IFLAGS)
	STATIC_LIBS		+= $(BLD_OPENSSL_LIBS)
	STATIC_LIBPATH 	+= $(BLD_OPENSSL_LIBPATH)
endif
ifeq	($(COMPILE_PHP5),1)
	MAKE_IFLAGS		+= $(BLD_PHP5_IFLAGS)
	STATIC_LIBS		+= $(BLD_PHP5_LIBS)
	STATIC_LIBPATH 	+= $(BLD_PHP5_LIBPATH)
endif

#
#	Package libraries
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

ifeq	($(BLD_FEATURE_DLL),0)
TEST_OBJS 	+=	test/testCompatModule$(BLD_OBJ) \
				test/testEspModule$(BLD_OBJ) \
				test/testEgiModule$(BLD_OBJ) \
				test/romFiles$(BLD_OBJ)
endif

ifeq	($(BLD_FEATURE_DLL),0)
MODE		= --preferStatic
else
MODE		= --preferShared
endif

#
#	Enable this for VxWorks 5.4 on X86 if umoddi3 and udivdi3 are undefined
#
# ifeq	($(BLD_HOST_OS),VXWORKS)
# SYSLIBS=		$(BLD_EXP_OBJ_DIR)/vxlib.a
# endif

#
#	Extra libraries
#
ifeq	($(BLD_HOST_OS),WIN)
	SYSLIBS	+= shell32.lib
	RES		= --resources appweb.rc
endif

#
#	Executables to build
#
TARGETS		+= $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
ifeq	($(BLD_FEATURE_TEST),1)
TARGETS		+= $(BLD_BIN_DIR)/testAppweb$(BLD_EXE)
endif

compileExtra: $(TARGETS) appweb.conf install.conf

#
#	Link standard appweb program. Linked with shared libraries supporting
#	dynamic loading of modules.
#
$(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE): \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB) $(FILES)
	@bld --c++ --graphical --executable $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE) \
		--libraryPath "$(LIBPATH)" \
		--libraryPath "$(STATIC_LIBPATH)" \
		$(MODE) --libs "appweb $(LIBS)" $(RES) \
		--libs "$(STATIC_LIBS)" \
		$(BLD_OBJ_DIR)/appweb$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/romFiles$(BLD_OBJ) \
		$(BLD_OBJ_DIR)/appwebStaticLink$(BLD_OBJ) $(SYSOBJS) $(SYSLIBS) 
	@rm -f $(BLD_PRODUCT)$(BLD_EXE)
	@echo "    cp $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE) $(BLD_PRODUCT)$(BLD_EXE)"
	@cp $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE) $(BLD_PRODUCT)$(BLD_EXE)

#
#	Test appweb program. Linked with shared libraries using dynamic
#	loading for all modules.
#
$(BLD_BIN_DIR)/testAppweb$(BLD_EXE): \
		$(BLD_LIB_DIR)/libappweb$(BLD_LIB) $(FILES) $(TEST_OBJS)
	@bld --c++ --graphical --executable $(BLD_BIN_DIR)/testAppweb$(BLD_EXE) \
		--libraryPath "$(LIBPATH)" \
		$(MODE) --libs "appweb $(LIBS)" $(RES) \
		$(BLD_OBJ_DIR)/appweb$(BLD_OBJ) \
		test/testStaticLink$(BLD_OBJ) \
		$(TEST_OBJS) $(SYSOBJS) $(SYSLIBS) 
	@rm -f test/testAppweb$(BLD_EXE)
	@echo "    cp $(BLD_BIN_DIR)/testAppweb$(BLD_EXE) test/testAppweb$(BLD_EXE)"
	@cp $(BLD_BIN_DIR)/testAppweb$(BLD_EXE) test/testAppweb$(BLD_EXE)

appweb.conf: conf/${BLD_APPWEB_CONFIG} ../buildConfig.make
	cp conf/${BLD_APPWEB_CONFIG} appweb.conf
	chmod +w appweb.conf
	BLD_PREFIX=. BLD_WEB_PREFIX=./web BLD_LIB_PREFIX=$(BLD_LIB_DIR) \
		BLD_DOC_PREFIX=.. \
		BLD_SERVER=`hostname` BLD_LOG_PREFIX=./logs \
		BLD_HTTP_PORT=$(BLD_HTTP_PORT) BLD_SSL_PORT=$(BLD_SSL_PORT) \
		patchAppwebConf appweb.conf

install.conf: conf/${BLD_APPWEB_CONFIG} ../buildConfig.make
	cp conf/${BLD_APPWEB_CONFIG} install.conf
	chmod +w install.conf
	BLD_PREFIX="$(BLD_PREFIX)" BLD_WEB_PREFIX="$(BLD_WEB_PREFIX)" \
		BLD_DOC_PREFIX="$(BLD_DOC_PREFIX)" BLD_LIB_PREFIX="$(BLD_LIB_PREFIX)" \
		BLD_SERVER=`hostname` BLD_LOG_PREFIX="$(BLD_LOG_PREFIX)" \
		BLD_HTTP_PORT=$(BLD_HTTP_PORT) BLD_SSL_PORT=$(BLD_SSL_PORT) \
		patchAppwebConf install.conf

#
#	Create the MS Visual Studio project
#
projects: $(BLD_PRODUCT).dsp

$(BLD_PRODUCT).dsp: always
	@makeProj --graphical --executable $(BLD_PRODUCT)$(BLD_EXE) \
		--libraryPath "$(LIBPATH)" \
		--libs "appweb $(LIBS)" \
		--resources appweb.rc \
		--sources "appweb.cpp romFiles.cpp appwebStaticLink.cpp" \
		--headers "appweb.h" shell32.lib

run:
	LD_LIBRARY_PATH="$(BLD_LIB_DIR):$(BLD_MOD_DIR)" \
	DYLD_LIBRARY_PATH="$(BLD_LIB_DIR):$(BLD_MOD_DIR)" \
		$(BLD_BIN_DIR)/$(BLD_PRODUCT) -r . -l stdout:6 -f appweb.conf


cleanExtra:
	rm -f $(TARGETS) $(BLD_PRODUCT)$(BLD_EXE)
	rm -f trace.txt access.log error.log leak.log
	rm -f *.obj *.pdb
	rm -f logs/*.log logs/*.old
	rm -f appweb.conf install.conf
	rm -fr target

rom:
	find web -print | \
	egrep -v 'CVS|\.svn|\.cvsignore|\.ignore|\.tmp|\.sav' >rom.files
	echo -e ".\nappweb.conf\nmime.types\nusers.db\ngroups.db" >>rom.files
	LD_LIBRARY_PATH=$(BLD_LIB_DIR) \
	DYLD_LIBRARY_PATH=$(BLD_LIB_DIR) \
		$(BLD_BIN_DIR)/httpComp rom.files >romFiles.cpp

#
#	VxWorks Quick deploy target to copy files to ./target
#
deploy:
	@echo "Copy files to $(BLD_TOP)/appweb/target"
	@[ ! -d target ] && mkdir target ; true
	@[ ! -d target/bin ] && mkdir target/bin ; true
	@[ ! -d target/put ] && mkdir target/put ; true
	@cp appweb.conf mime.types server.crt server.key.pem target
	@cp ../bin/appweb.out target
	@[ ! -d target/web ] && mkdir target/web ; true
	@[ ! -d target/web/icons ] && mkdir target/web/icons ; true
	@cp web/*.esp web/*.html web/*.ico web/*.js target/web
	@cp web/icons/* target/web/icons
	@echo "Sizes for appweb.out"
	@size$(BLD_BUILD_CPU_VX) target/appweb.out 
	cp C:/tornado/target/proj/appweb/default/vxWorks target
	echo OLD cp C:/tornado/target/proj/mob/default/vxWorks target
	@mkdir -p target/sub1/sub2
	@date >target/sub1/a.txt
	@date >target/sub1/sub2/b.txt

vxSetup:
	mkdir -p target
	subst v: c:\usr\svn\appweb\trunk\appweb\target

#	subst /d v:

size:
	@echo -e "    #\n    #	Size of $(BLD_PRODUCT)\n    #"
	@size $(BLD_PRODUCT) | sed -e 's/^/    # /'


#
#	Some targets to help you get going with SSL (using OpenSSL)
#	All these certificates are for internal use only.
#	Use "make ServerCertReq" to generate a certificate request to send to a
#	certificate authority like Verisign
#
#	To run appweb, you need to do:
#
#		make caCert
#		make serverCert
#
#
#	Setup a test certificate authority. Use this if you will be generating
#	multiple certificates for clients and servers and want to validate them. 
#	The browser will say it doesn't recognize this certificate, but that is ok.
#
ca:
	umask 77 ; OPENSSL_CONF=openssl.ca ; export OPENSSL_CONF ; \
	openssl req -x509 -newkey rsa -passout pass:ca -out ca.crt -outform PEM; \
	openssl rsa -passin pass:ca -in ca.key -out ca.key.pem ; \
	echo "01" >ca.serial ; \
	>ca.db

#
#	Client certificate to put in your browser. Used when doing client validation
#
clientCert:
	umask 77 ; OPENSSL_CONF=openssl.conf ; export OPENSSL_CONF ; \
	rm -f client.crt client.key client.key.pem client.req ; \
	openssl req -passout pass:client -newkey rsa:1024 -keyout client.key \
		-out client.req < response.client ; \
	openssl rsa -passin pass:client -in client.key -out client.key.pem ; \
	OPENSSL_CONF=openssl.ca ; export OPENSSL_CONF ; \
	openssl ca -in client.req -out client.crt

#
#	Server certificate to put in Appweb. Necessary.
#
serverCert:
	umask 77 ; OPENSSL_CONF=openssl.conf ; export OPENSSL_CONF ; \
	rm -f server.crt server.key server.key.pem server.req ; \
	openssl req -passout pass:server -newkey rsa:1024 -keyout server.key \
		-out server.req < response.server ; \
	openssl rsa -passin pass:server -in server.key -out server.key.pem ; \
	OPENSSL_CONF=openssl.ca ; export OPENSSL_CONF ; \
	openssl ca -in server.req -out server.crt

#
#	Server certificate to put in Appweb. Necessary.
#	Use a self-signed certificate when you just want quick and dirty testing.
#	The browser will say it doesn't recognize this certificate, but that is ok.
#
serverSelfSignCert:
	umask 77 ; OPENSSL_CONF=openssl.conf ; export OPENSSL_CONF ; \
	openssl genrsa -des3 -passout pass:server -out server.key 1024 ; \
	openssl req -passin pass:server -new -x509 -days 365 -key server.key \
		-out server.crt < response.server ; \
	openssl rsa -passin pass:server -in server.key -out server.key.pem

#
#	Self-signed client request
#
clientSelfSignCert:
	umask 77 ; OPENSSL_CONF=openssl.conf ; export OPENSSL_CONF ; \
	openssl genrsa -des3 -passout pass:client -out client.key 1024 ; \
	openssl req -passin pass:client -new -x509 -days 365 -key client.key \
		-out client.crt < response.client ; \
	openssl rsa -passin pass:client -in client.key -out client.key.pem

#
#	Generate a request to send to a certificate authority like Verisign
#
ServerCertReq:
	umask 77 ; \
	openssl genrsa -des3 -passout pass:server -out server.key 1024 ; \
	openssl req -new -key server.key -out server.csr

#
#	Stop the key asking for a password by decrypting it and rewriting back
#	to server.key. WARNING: this is not secure as the key is now decrypted.
#
noPassPhrase:
	openssl genrsa 1024 -out server.key
	chmod 600 server.key

#
#	Decrypt the key into server.key.pem
#
decrypted:
	openssl rsa -in server.key -out server.key.pem

curl:
	curl --trace trace.tmp --dump-header header.tmp \
		https://127.0.0.1:4443/index.html

val:
	(valgrind -v --tool=memcheck --suppressions=d.supp --leak-check=yes --num-callers=12 --leak-resolution=high --show-reachable=yes --track-fds=yes ../bin/appweb -r . -f appweb.conf -l stdout:1  2>&1) | tee val.out

mem:
	@echo -e "\nImage and code sizes:"
	@ls -hs  $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
	@size $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
	pid=`ps -ef | grep 'appweb.*conf' | grep -v grep | awk '{ print \$$2 }'` ; \
	echo -e "\nMemory usage for $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE), pid $$pid:" ; \
	cat /proc/$$pid/status | egrep 'VmSize|VmPeak|VmRSS|VmData|VmStk|VmExe'


## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
