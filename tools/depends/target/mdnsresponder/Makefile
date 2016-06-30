include ../../Makefile.include
DEPS= ../../Makefile.include Makefile makefile.internal 01-android-embedded.patch


# lib name, version
LIBNAME=mDNSResponder
VERSION=625.41.2
OUTPUTNAME=libmDNSEmbedded
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/$(OUTPUTNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../01-android-embedded.patch
	cd $(PLATFORM); cp ../makefile.internal Makefile

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); CC="$(CC)" AR="$(AR)" CFLAGS="$(CFLAGS)" $(MAKE)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); PREFIX="$(PREFIX)" $(MAKE) install
	touch $@

clean:
	make -C $(PLATFORM) clean 
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
