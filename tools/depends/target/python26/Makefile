include ../../Makefile.include
DEPS= ../../Makefile.include Makefile Python-2.6.2-android.patch \
      Python-2.6.5-xcompile.patch Python-2.6-ctypes.patch \
      Python-no-modules.patch Python-no-export-path.patch modules.setup

# lib name, version
LIBNAME=Python
VERSION=2.6.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=OPT="$(CFLAGS)" \
  LIBS=-lm \
  ./configure --prefix=$(PREFIX) \
  --disable-toolbox-glue --disable-framework --with-system-ffi --without-pymalloc \

LIBDYLIB=$(PLATFORM)/libpython2.6.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	echo $(NATIVEPREFIX)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -Np1 -i ../Python-2.6.5-xcompile.patch
	cd $(PLATFORM); patch -Np1 -i ../Python-2.6.2-android.patch
	cd $(PLATFORM); patch -p0 < ../Python-no-modules.patch
	cd $(PLATFORM); patch -p0 < ../Python-no-export-path.patch
	cd $(PLATFORM); patch -p1 < ../python-osx-environ-fix.patch
	cd $(PLATFORM); patch -p0 < ../Python-2.6.5-ffi-static.patch
	cd $(PLATFORM); patch -p0 < ../python-android-binmodule.patch
ifeq ($(OS),ios)
	cd $(PLATFORM); patch -p0 < ../Python-2.6.5-urllib.diff
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
	echo "_scproxy \$$(srcdir)/Mac/Modules/_scproxy.c -framework SystemConfiguration -framework CoreFoundation" >> $(PLATFORM)/Modules/Setup.dist
endif

ifeq ($(OS),android)
	cd $(PLATFORM); sed -ie 's|-framework CoreFoundation||' configure.in
endif

	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) HOSTPYTHON=$(NATIVEPREFIX)/bin/python HOSTPGEN=$(NATIVEPREFIX)/bin/pgen CROSS_COMPILE_TARGET=yes libpython2.6.a
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	make -j1 -C $(PLATFORM) HOSTPYTHON=$(NATIVEPREFIX)/bin/python HOSTPGEN=$(NATIVEPREFIX)/bin/pgen CROSS_COMPILE_TARGET=yes install
	find $(PREFIX)/lib/python2.6 -type f -name "*.py" -delete
	find $(PREFIX)/lib/python2.6 -type f -name "*.pyc" -delete
	#
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
