include ../Makefile.include
DEPS= ../Makefile.include Makefile

LIBNAME=xbmc-pvr-addons
VERSION=1dc2b52e2d219fcac4d00227fd576e5f5158d7f3
GIT_DIR=$(TARBALLS_LOCATION)/$(LIBNAME).git
BASE_URL=git://github.com/opdenkamp/$(LIBNAME).git
DYLIB=$(PLATFORM)/addons/pvr.demo/.libs/libpvrdemo-addon.so
#tell git to use the addons repo rather than xbmc's repo
export GIT_DIR
export GIT_WORK_TREE=$(PLATFORM)
# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)

all: .installed-$(PLATFORM)

$(GIT_DIR)/HEAD:
	cd $(TARBALLS_LOCATION); git clone --bare $(BASE_URL)

$(GIT_DIR)/current/$(VERSION): $(GIT_DIR)/HEAD $(DEPS)
	git rev-list -1 $(VERSION) >/dev/null || git fetch -a
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

.installed-$(PLATFORM): $(DYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	-make -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	rm -rf $(GIT_DIR)/current

