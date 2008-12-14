#
#	Makefile -- for the obj directory
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

include		make.dep

dependExtra:
	mkdir -p classes verified crushed
	if [ "$(BLD_CROSS)" = 1 ] ; then \
		mkdir -p ./$(BLD_HOST_SYSTEM)/classes ./$(BLD_HOST_SYSTEM)/verified ./$(BLD_HOST_SYSTEM)/crushed ; \
		mkdir -p ./$(BLD_HOST_SYSTEM)/src ; \
	fi

cleanExtra:
	rm -rf ./classes/* ./verified/* ./crushed/* ./*${BLD_OBJ}
	rm -rf ./$(BLD_HOST_SYSTEM)/classes/* ./$(BLD_HOST_SYSTEM)/verified/* ./$(BLD_HOST_SYSTEM)/crushed/* \
		./$(BLD_HOST_SYSTEM)/*${BLD_OBJ}

clobberExtra:
	rm -rf src/* classes/* verified/* crushed/* 
	rm -rf ./$(BLD_HOST_SYSTEM)/src/* ./$(BLD_HOST_SYSTEM)/classes/* ./$(BLD_HOST_SYSTEM)/verified/* ./$(BLD_HOST_SYSTEM)/crushed/* 

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
