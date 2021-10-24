include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile 01-gettext-tools-stpncpy.patch 02-disable-test-doc.patch

# lib name, version
LIBNAME=gettext
VERSION=0.21
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

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

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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

