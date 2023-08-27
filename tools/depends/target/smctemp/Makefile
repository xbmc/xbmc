include ../../Makefile.include SMCTEMP-VERSION ../../download-files.include
DEPS = ../../Makefile.include SMCTEMP-VERSION  Makefile ../../download-files.include \

LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

ifeq ($(CPU), arm64)
    CXXFLAGS += -DARCH_TYPE_ARM64
else
    CXXFLAGS += -DARCH_TYPE_X86_64
endif

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" AR=$(AR) RANLIB=$(RANLIB) -C $(PLATFORM) staticlib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) DEST_PREFIX=$(PREFIX) -C $(PLATFORM) installstaticlib
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
