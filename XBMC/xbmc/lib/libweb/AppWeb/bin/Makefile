#
#  Makefile -- Binaries Makefile
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

include 	make.dep

dependExtra:
	[ "$(BLD_CROSS)" = 1 ] && mkdir -p $(BLD_HOST_SYSTEM) ; true

cleanExtra:
	rm -f ./$(BLD_HOST_SYSTEM)/*
	rm -f tmp*.exe appweb* cgiProgram* charGen* httpClient* httpComp*
	rm -f httpPassword* testAppweb* testClient*

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
