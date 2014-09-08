include ../../Makefile.include
#DEPS= ../../Makefile.include Makefile

LIBNAME=xbmc-pvr-addons
VERSION=3949502043d520e4707bc3109abf3c6edb23cf67
GIT_DIR=$(TARBALLS_LOCATION)/$(LIBNAME).git
BASE_URL=git://github.com/opdenkamp/$(LIBNAME).git
DYLIB=$(PLATFORM)/addons/pvr.demo/.libs/libpvrdemo-addon.so
XBMC_ADDONSDIR=../../../../addons

#tell git to use the addons repo rather than xbmc's repo
export GIT_DIR
export GIT_WORK_TREE=$(PLATFORM)

#mysql_config is remarkably useless. Help configure find the right one.
export MYSQL_CONFIG=$(PREFIX)/bin/mysql_config

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --enable-addons-with-dependencies

all: .installed-$(PLATFORM)

$(GIT_DIR)/HEAD:
	cd $(TARBALLS_LOCATION); git clone --bare $(BASE_URL)

$(GIT_DIR)/current/$(VERSION): $(GIT_DIR)/HEAD $(DEPS)
	git rev-list -1 $(VERSION) >/dev/null || git fetch origin "+refs/heads/*:refs/remotes/origin/*"
	git rev-list -1 $(VERSION) >/dev/null
	rm -rf $(GIT_DIR)/current; mkdir -p $(GIT_DIR)/current
	touch $@

$(PLATFORM)/bootstrap: $(GIT_DIR)/current/$(VERSION)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	git checkout $(VERSION) -- .

$(PLATFORM)/configure: $(PLATFORM)/bootstrap
	cd $(PLATFORM); ./bootstrap

$(PLATFORM)/Makefile: $(PLATFORM)/configure
	cd $(PLATFORM); $(CONFIGURE)

$(DYLIB): $(PLATFORM)/Makefile
	make -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(DYLIB)
ifeq (darwin, $(findstring darwin, $(HOST)))
	for ADDON in `find $(PLATFORM)/addons -type d -name "pvr.*"`; do \
	  ADDON=`basename $$ADDON` ; \
	  mkdir -p $(XBMC_ADDONSDIR)/$$ADDON ; \
	  cp -PRf $(PLATFORM)/addons/$$ADDON/addon/* $(XBMC_ADDONSDIR)/$$ADDON ; \
	  cp -Pf $(PLATFORM)/addons/$$ADDON/*.pvr $(XBMC_ADDONSDIR)/$$ADDON ; \
	  install_name_tool -id "$$ADDON" `find "$(XBMC_ADDONSDIR)/$$ADDON" -type f -name "*.pvr"` ; \
	done
else
	$(MAKE) -C $(PLATFORM) install
endif
	touch $@

clean:
	-make -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	rm -rf $(GIT_DIR)/current

