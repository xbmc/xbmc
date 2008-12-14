#
# Makefile for the packages
#
# Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#

PRE_DIRS	= $(BLD_HOST_OS)

include 	make.dep 

BASE_NAME	= "$(BLD_PRODUCT)-$(BLD_VERSION)-$(BLD_NUMBER)-$(BLD_HOST_DIST)-$(BLD_HOST_OS)-$(BLD_HOST_CPU)"
WIN_INS_DIR	= c:/tmp/testInsAppweb

install-package:
	@if [ $(BLD_HOST_OS) = WIN ] ; then \
		name="$(BASE_NAME).exe.zip" ; \
		rm -fr "$(WIN_INS_DIR)" ; \
		cd $(BLD_TOP); top=`pwd` ; cd /tmp ; unzip -o $$top/releases/$(BLD_PRODUCT)/$$name ; \
		name="${BLD_PRODUCT}-${BLD_VERSION}-${BLD_NUMBER}-${BLD_HOST_OS}-${BLD_HOST_CPU}.exe" ; \
		cp $$top/package/WIN/unattended.ans /tmp ; \
		echo "Dir=$(WIN_INS_DIR)" >>/tmp/unattended.ans ; \
		$$name /loadinf=/tmp/unattended.ans /silent ; \
	else \
		name="$(BASE_NAME).tar.tar.gz" ; \
		cd $(BLD_TOP); top=`pwd` ; cd /tmp ; tar xvfz $$top/releases/$(BLD_PRODUCT)/$$name ; \
		priv "$(BLD_PRODUCT)_HEADLESS=1 ./install" ; \
	fi

uninstall-package:
	@if [ $(BLD_HOST_OS) = WIN ] ; then \
		if [ ! -x "$(WIN_INS_DIR)/unins000.exe" ] ; then \
			echo "Can't find $(WIN_INS_DIR)/unins000.exe to uninstall" ; \
			exit 255 ; \
		fi ; \
		"$(WIN_INS_DIR)/unins000.exe" /silent ; \
	else \
		name="$(BASE_NAME).tar.tar.gz" ; \
		cd $(BLD_TOP); top=`pwd` ; cd /tmp ; tar xvfz $$top/releases/$(BLD_PRODUCT)/$$name ; \
		priv "$(BLD_PRODUCT)_HEADLESS=1 ./remove" ; \
	fi

install install-%:
	@packs=$* ; [ "$$packs" = "" ] && packs="binary" ; \
	[ "$$packs" = "all" ] && packs="binary release dev doc samples" ; \
	priv "export BLD_TOP=$$BLD_TOP TRACE=$(TRACE) TASK=Install ; \
			export DESTDIR="$(DESTDIR)" ; \
		set -e ; $(BLD_TOP)/tools/makeInstall $$packs" ; \

uninstall uninstall-%:
	@packs=$* ; [ "$$packs" = "" ] && packs="binary" ; \
	[ "$$packs" = "all" ] && packs="samples doc dev binary release" ; \
	priv "export BLD_TOP=$$BLD_TOP TRACE=$(TRACE) TASK=Remove ; \
			export DESTDIR="$(DESTDIR)" ; \
		set -e ; $(BLD_TOP)/tools/makeInstall $$packs" ; \
	
checkInstalled:
	@for d in /etc /usr/src /usr/share /usr/share/doc /usr/lib /usr/include /var/log /var/www ; \
	do \
		[ -x $$d/$(BLD_PRODUCT)* ] && echo "Found $$d/$(BLD_PRODUCT)*" || true; \
	done

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
