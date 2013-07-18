include ../../Makefile.include
#DEPS= ../../Makefile.include Makefile

LIBNAME=xbmc-platform
VERSION=master
GIT_DIR=$(TARBALLS_LOCATION)/$(LIBNAME).git
BASE_URL=git://github.com/cptspiff/$(LIBNAME).git
DYLIB=$(PLATFORM)/libxbmcplatform.so

#tell git to use the addons repo rather than xbmc's repo
export GIT_DIR
export GIT_WORK_TREE=$(PLATFORM)

# configuration settings
CONFIGURE=$(CMAKE) . -DCMAKE_BUILD_TYPE=Release

all: .installed-$(PLATFORM)

$(GIT_DIR)/HEAD:
	cd $(TARBALLS_LOCATION); git clone --bare $(BASE_URL)

$(GIT_DIR)/current/$(VERSION): $(GIT_DIR)/HEAD $(DEPS)
	git rev-list -1 $(VERSION) >/dev/null || git fetch origin "+refs/heads/*:refs/remotes/origin/*"
	git rev-list -1 $(VERSION) >/dev/null
	rm -rf $(GIT_DIR)/current; mkdir -p $(GIT_DIR)/current
	touch $@

$(PLATFORM)/CMakeLists.txt: $(GIT_DIR)/current/$(VERSION)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	git checkout $(VERSION) -- .

$(PLATFORM)/Makefile: $(PLATFORM)/CMakeLists.txt
	cd $(PLATFORM); $(CONFIGURE)

$(DYLIB): $(PLATFORM)/Makefile
	make -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(DYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	-make -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	rm -rf $(GIT_DIR)/current
