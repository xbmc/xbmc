include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=pcre
VERSION=8.45
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=91bff52eed4a2dfc3f3bfdc9c672b88e7e2ffcf3c4b121540af8a4ae8c1ce05178430aa6b8000658b9bb7b4252239357250890e20ceb84b79cdfcde05154061a
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-cpp \
	--disable-shared --disable-stack-for-recursion \
	--enable-pcre8 --disable-pcre16 --disable-pcre32 \
	--enable-jit --enable-utf --enable-unicode-properties


LIBDYLIB=$(PLATFORM)/.libs/lib$(LIBNAME).so

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $(LIBDYLIB)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
