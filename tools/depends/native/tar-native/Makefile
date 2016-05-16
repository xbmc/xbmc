include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include.in Makefile

# app name, version
APPNAME=tar
VERSION=1.28
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

ifeq ($(NATIVE_OS),osx)
# https://openradar.appspot.com/radar?id=6160634819379200
PLATFORM_CONFIGURE="gl_cv_func_getcwd_abort_bug=no"
endif

export LIBTOOL=builds/unix/libtool
export PATH:=$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) \
  --program-transform-name=s/tar/gtar/   \
  --disable-dependency-tracking ac_cv_func_fdopendir=no \
  $(PLATFORM_CONFIGURE)

APP=$(SOURCE)/src/tar
APPBIN=$(PREFIX)/bin/tar

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../fix_xattrs_configure.patch
	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/bin/tar $(PREFIX)/bin/gnutar
	cd $(PREFIX)/bin; ln -s gtar tar; ln -s gtar gnutar
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
