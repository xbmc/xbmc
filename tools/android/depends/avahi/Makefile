include ../Makefile.include
DEPS= ../Makefile.include Makefile

LIBNAME=avahi
VERSION=0.6.31
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL=http://www.avahi.org/download/

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) --disable-shared \
                --disable-qt3 \
                --disable-qt4 \
                --disable-gdbm \
                --disable-python-dbus \
                --disable-pygtk \
                --disable-gtk3 \
                --disable-gtk \
                --disable-mono \
                --disable-monodoc \
                --disable-stack-protector \
                --disable-manpages \
                --disable-autoipd \
                --disable-glib \
                --disable-gobject \
                --disable-dbus \
                --disable-libdaemon \
                --with-distro=none \
                --with-avahi-user=root \
                --with-avahi-group=root \
                --with-autoipd-user=default \
                --with-autoipd-group=default \


LIBDYLIB=$(PLATFORM)/avahi-common/.libs/libavahi-common.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../avahi-optional-ipv6.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

