include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=Sparkle
VERSION=1.16.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

FRAMEWORK=Sparkle.framework
FRAMEWORK_SRC=$(PLATFORM)/build/Release/$(FRAMEWORK)
DYLIB=$(FRAMEWORK_SRC)/Sparkle

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../obey_os_attribute.patch


$(DYLIB): $(PLATFORM)
	cd $(PLATFORM); xcodebuild -sdk $(SDK_NAME) -project \
	                           Sparkle.xcodeproj/ -target Sparkle \
	                           MACOSX_DEPLOYMENT_TARGET=$(PLATFORM_MIN) \
	                           ONLY_ACTIVE_ARCH=YES ARCHS=$(CPU) VALID_ARCHS=$(CPU) \
	                           CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" CPPFLAGS="$(CPPFLAGS)" \
	                           DYLIB_INSTALL_NAME_BASE=$(PREFIX)/Frameworks \
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

