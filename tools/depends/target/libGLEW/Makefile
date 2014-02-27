include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=glew
VERSION=1.7.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SYSTEM=$(OS)
ifeq ($(OS),osx)
  SYSTEM=darwin
endif
export SYSTEM
export GLEW_DEST=$(PREFIX)

LIBDYLIB=$(PLATFORM)/lib/libGLEW.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	sed -ie "s|CC = cc|CC = $(CC)|" "$(PLATFORM)/config/Makefile.$(SYSTEM)"
	sed -ie "s|LD = cc|LD = $(CC)|" "$(PLATFORM)/config/Makefile.$(SYSTEM)"
	sed -ie "s|CFLAGS.EXTRA =|CFLAGS.EXTRA = \$(CFLAGS)|" "$(PLATFORM)/config/Makefile.$(SYSTEM)"
	sed -ie "s|LDFLAGS.EXTRA =|LDFLAGS.EXTRA = \$(LDFLAGS)|" "$(PLATFORM)/config/Makefile.$(SYSTEM)"
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
