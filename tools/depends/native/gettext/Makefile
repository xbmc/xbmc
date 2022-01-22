include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile 01-gettext-tools-stpncpy.patch 02-disable-test-doc.patch ../../download-files.include

# lib name, version
LIBNAME=gettext
VERSION=0.21
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=f7e2968651879f8444d43a176a149db9f9411f4a03132a7f3b37c2ed97e3978ae6888169c995c1953cb78943b6e3573811abcbb8661b6631edbbe067b2699ddf
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
    --disable-csharp --disable-native-java --disable-java --without-emacs \
    --disable-libasprintf --disable-openmp \
    --with-included-gettext \
    --with-included-glib \
    --with-included-libcroco \
    --with-included-libxml \
    --without-git --without-cvs \
    --disable-shared --disable-curses --disable-acl --disable-c++ --disable-nls

LIBDYLIB=$(PLATFORM)/gettext-tools/src/.libs/libgettextsrc.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); sed -ie '/gets is a security hole/d' gettext-tools/gnulib-lib/stdio.in.h
	cd $(PLATFORM); sed -ie '/gets is a security hole/d' gettext-tools/libgettextpo/stdio.in.h
	cd $(PLATFORM); sed -ie '/gets is a security hole/d' gettext-runtime/gnulib-lib/stdio.in.h
	cd $(PLATFORM); patch -p1 -i ../01-gettext-tools-stpncpy.patch
	cd $(PLATFORM); patch -p1 -i ../02-disable-test-doc.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/libtextstyle
	$(MAKE) -C $(PLATFORM)/gettext-tools

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -j1 -C $(PLATFORM)/gettext-tools install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

