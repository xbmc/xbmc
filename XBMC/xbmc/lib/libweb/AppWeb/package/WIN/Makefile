#
# Makefile for the Windows Appweb release
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

PACKS		:= release binary dev doc samples src
PKG_DIR 	:= /tmp/pkg/appweb

include 	make.dep 

packageExtra:
	@priv "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 TRACE=$(TRACE) ; \
			export ROOT_DIR=$(PKG_DIR); \
			set -e ; $(BLD_TOOLS_DIR)/makeInstall $(PACKS) ; \
			$(BLD_TOOLS_DIR)/makePackage $(PACKS)"

justPack:
	@priv "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 TRACE=$(TRACE) ; \
			export ROOT_DIR=$(PKG_DIR); \
			set -e ; \
			$(BLD_TOOLS_DIR)/makePackage $(PACKS)"

cleanExtra:
	@rm -fr pkgtmp input.iss .pbldtmp bin.iss make.out Output

clobberExtra:
	@rm -fr /tmp/pkg/appweb


## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
