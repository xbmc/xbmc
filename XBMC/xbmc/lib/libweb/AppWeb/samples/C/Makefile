#
#	Makefile for the C AppWeb samples
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#
#	This Makefile is for use in the checked out source tree.
#

include 	make.dep

PRE_DIRS 	= 

#
#	All these samples require the C API
#
ifeq	($(BLD_FEATURE_C_API_MODULE),1)

ifeq	($(BLD_FEATURE_ESP_MODULE),1)
PRE_DIRS += simpleEsp esp
endif

ifeq	($(BLD_FEATURE_EGI_MODULE),1)
PRE_DIRS += simpleEgi
endif

ifeq	($(BLD_FEATURE_COPY_MODULE),1)
PRE_DIRS += simpleServer
endif

ifeq	($(BLD_FEATURE_COPY_MODULE),1)
PRE_DIRS += selectEventLoop
endif

ifeq	($(BLD_FEATURE_COPY_MODULE),1)
PRE_DIRS += pollEventLoop
endif

ifeq	($(BLD_FEATURE_COMPAT_MODULE),1)
ifeq	($(BLD_FEATURE_ESP_MODULE),1)
#
#	This is currently disabled because the Compat module is not included
#	in the default build.
#
#		PRE_DIRS += goaheadCompat
#
endif
endif

ifeq	($(BLD_HOST_OS),WIN)
	PRE_DIRS += winEventLoop
endif

endif

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
