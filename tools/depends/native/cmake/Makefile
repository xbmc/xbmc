include ../../Makefile.include CMAKE-VERSION  ../../download-files.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile CMAKE-VERSION ../../download-files.include

# configuration settings

SETENV=CC="$(CC_FOR_BUILD)" CXX="$(CXX_FOR_BUILD)" LD=$(LD_FOR_BUILD) CFLAGS="$(NATIVE_CFLAGS)" \
	CXXFLAGS="$(NATIVE_CXXFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)"

CONFIGURE=./bootstrap --prefix=$(NATIVEPREFIX) --system-curl
ifeq ($(USE_CCACHE), yes)
	CONFIGURE+=--enable-ccache
endif

APP=$(PLATFORM)/bin/$(APPNAME)

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(SETENV) $(CONFIGURE)

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

