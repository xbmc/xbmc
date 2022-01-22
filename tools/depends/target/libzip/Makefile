include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libzip
VERSION=1.8.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=fe94a26a2797dff7c9b27334f29982f8e523dd5095bf32ebcadc6443726864f0ff7b2e8bf243e52cad8f79297bc7a87d73fdbc35edae4ffc8ab6aa0bcc93b03e
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/lib/libzip.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CMAKE) -B build -DBUILD_DOC:BOOL=OFF -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_REGRESS:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DBUILD_TOOLS:BOOL=OFF

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
