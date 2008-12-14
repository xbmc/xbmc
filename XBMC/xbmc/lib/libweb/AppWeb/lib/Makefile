#
#  Makefile -- Libraries Makefile
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

PRE_DIRS	:= modules

include 	make.dep

dependExtra:
	[ "$(BLD_CROSS)" = 1 ] && mkdir -p $(BLD_HOST_SYSTEM) ; true
ifeq ($(BLD_CC_MSCL),1)
	cp msvcrt/$(BLD_CC_CL_VERSION)/* .
endif

#
#	We must restore the msvcrt files after cleaning
#
cleanExtra: cleanFiles dependExtra

cleanFiles:
	rm -f httpClient httpComp httpPassword charGen
	rm -f dsiPatch* genLicense* genDepend* httpServer* dumpext*
	rm -f testClient* testAppweb*
	rm -f cgiProgram appweb winAppweb http testHttp staticAppweb
	rm -f httpComp* httpPassword*
	rm -f libcrypto.* ssleay32.* libssl.* libappweb*
	rm -f libejs.so libmatrixssl.so libmpr.so libmprTest.so libphp5.so
	rm -f libeay32.* libejs.* libejsStatic.*
	rm -f libmatrixssl.* libmpr.* libmprStatic.* libmprTest.*
	rm -f libmprTestStatic.* libespStatic.* php5* msvcr*.*

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
