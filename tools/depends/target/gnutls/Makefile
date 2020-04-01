include ../../Makefile.include
DEPS= ../../Makefile.include Makefile size-max.patch add-dl-as-private-lib.patch

# lib name, version
LIBNAME=gnutls
VERSION=3.6.13
BASE_URL := https://www.gnupg.org/ftp/gcrypt/gnutls/v3.6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --without-p11-kit --disable-nls --with-included-unistring \
                      --with-included-libtasn1 --enable-local-libopts --disable-doc --disable-tests --disable-guile \
                      --disable-tools $(CONFIGURE_OPTIONS)

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../size-max.patch
	cd $(PLATFORM); patch -p1 -i ../add-dl-as-private-lib.patch
ifeq ($(OS),darwin_embedded)
	# Apple CCAS requires stripping ; from comment block # to build successfully
	cd $(PLATFORM); sed -ie "s|\(^# .*\)[;]\(.*\)|\1\2|g" lib/accelerated/aarch64/macosx/*.s
endif
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
