#
# Makefile to build a linux package
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

PACKS		:= release binary dev doc samples src
PKG_DIR 	:= /tmp/pkg/appweb

include 	make.dep

packageExtra:
	set -e ; \
	export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 TRACE=$(TRACE) ROOT_DIR=$(PKG_DIR); \
	fakePriv $(BLD_TOOLS_DIR)/makeInstall $(PACKS) ; \
	fakePriv $(BLD_TOOLS_DIR)/makePackage $(PACKS)

packageOnly:
	export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 TRACE=$(TRACE) ROOT_DIR=$(PKG_DIR); \
	fakePriv "$(BLD_TOOLS_DIR)/makePackage $(PACKS)"

cleanExtra:
	@rm -fr .pbldtmp rpm.out

clobberExtra:
	@rm -fr /tmp/pkg/appweb


## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
