include ../../Makefile.include
DEPS= ../../Makefile.include Makefile Python-3.6.2-crosscompile.patch Python-3.6.2-android.patch \
       fix-datetime.patch apple.patch modules.setup make-fork-optional.patch

# lib name, version
LIBNAME=Python
VERSION=3.6.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

ifeq ($(OS),osx)
  HOSTPLATFORM=_PYTHON_HOST_PLATFORM="darwin"
endif
ifeq ($(OS),ios)
  HOSTPLATFORM=_PYTHON_HOST_PLATFORM="darwin"
  LINK_ICONV=-framework CoreFoundation -liconv
endif

# configuration settings
CONFIGURE=OPT="$(CFLAGS)" \
  LIBS=-lm \
  ./configure --prefix=$(PREFIX) --host=$(HOST) \
  --disable-framework --with-system-ffi --without-pymalloc --enable-ipv6

LIBDYLIB=$(PLATFORM)/libpython3.6.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	echo $(NATIVEPREFIX)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../Python-3.6.2-crosscompile.patch
	cd $(PLATFORM); patch -p0 < ../fix-datetime.patch
	cd $(PLATFORM); patch -p0 < ../Python-3.6.2-android.patch
	cd $(PLATFORM); patch -p0 < ../fix-ffi.patch
	cd $(PLATFORM); patch -p1 -i ../apple.patch
ifeq ($(OS),ios)
	cd $(PLATFORM); patch -p0 < ../make-fork-optional.patch
	cd $(PLATFORM); sed -ie 's|MACHDEP="unknown"|MACHDEP="darwin"|' configure.ac
endif
	cp modules.setup $(PLATFORM)/Modules/Setup.dist

#Add -liconv as needed, and add the _scproxy module for darwin
#  disable locale altogether for Android
ifeq ($(OS),android)
	cd $(PLATFORM); sed -ie 's|_locale _localemodule.c   -lintl|#_locale _localemodule.c   -lintl|' Modules/Setup.dist
else
	cd $(PLATFORM); sed -ie 's|_locale _localemodule.c   -lintl|_locale _localemodule.c   -lintl $(LINK_ICONV) |' Modules/Setup.dist
endif
ifeq ($(OS),osx)
	cd $(PLATFORM); sed -ie 's|MACHDEP="unknown"|MACHDEP="darwin"|' configure.ac
	echo "_scproxy \$$(srcdir)/Modules/_scproxy.c -framework SystemConfiguration -framework CoreFoundation" >> $(PLATFORM)/Modules/Setup.dist
endif

ifeq ($(OS),android)
	cd $(PLATFORM); sed -ie 's|-framework CoreFoundation||' configure.ac
endif

	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) PYTHON_FOR_BUILD=$(NATIVEPREFIX)/bin/python3 NATIVE_PGEN=$(NATIVEPREFIX)/bin/pgen CROSS_COMPILE_TARGET=yes libpython3.6.a
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) $(HOSTPLATFORM) PYTHON_FOR_BUILD=$(NATIVEPREFIX)/bin/python3 NATIVE_PGEN=$(NATIVEPREFIX)/bin/pgen CROSS_COMPILE_TARGET=yes install
	# find $(PREFIX)/lib/python3.6 -type f -name "*.py" -delete
	find $(PREFIX)/lib/python3.6 -type f -name "*.pyc" -delete
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
