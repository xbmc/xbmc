include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

LIBNAME=Sparkle
VERSION=1.6.1
GIT_DIR=$(TARBALLS_LOCATION)/$(LIBNAME).git
BASE_URL=git://github.com/sparkle-project/$(LIBNAME).git
FRAMEWORK=Sparkle.framework
FRAMEWORK_SRC=$(PLATFORM)/build/Release/$(FRAMEWORK)
DYLIB=$(FRAMEWORK_SRC)/Sparkle

#tell git to use the addons repo rather than xbmc's repo
export GIT_DIR
export GIT_WORK_TREE=$(PLATFORM)

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

$(DYLIB): $(PLATFORM)/bootstrap
	cd $(PLATFORM); xcodebuild -sdk $(SDK_NAME) -project \
	                           Sparkle.xcodeproj/ -target Sparkle \
	                           MACOSX_DEPLOYMENT_TARGET=$(PLATFORM_MIN) \
	                           ONLY_ACTIVE_ARCH=YES ARCHS=$(CPU) VALID_ARCHS=$(CPU) \
	                           CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" CPPFLAGS="$(CPPFLAGS)" \
	                           DYLIB_INSTALL_NAME_BASE=$(PREFIX)/Frameworks
	                           CONFIGURATION_BUILD_DIR=$(PLATFORM)/build/Release
	                           -configuration Release build
	touch $@

.installed-$(PLATFORM): $(DYLIB)
	mkdir -p $(PREFIX)/Frameworks
	-rm -r $(PREFIX)/Frameworks/$(FRAMEWORK)
	cp -a $(FRAMEWORK_SRC) $(PREFIX)/Frameworks/$(FRAMEWORK)
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	rm -rf $(GIT_DIR)/current

