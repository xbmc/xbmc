include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=crossenv
VERSION=1.0+git581c885f
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# provide any platform specific env variables to be available to the cross env
ifeq ($(OS),android)
  LDSHAREDCC=--env=LDSHARED:="$(CC) -shared $(PYTHON_MODULELDFLAGS)"
  SDKROOT=$(NDKROOT)/sysroot
endif
ifeq (darwin, $(findstring darwin, $(HOST)))
  ifeq ($(OS),darwin_embedded)
    CROSS_CC_FLAGS=-arch $(CPU) --sysroot=$(SDKROOT)
  endif
  ZLIBROOT=--env=ZLIB_ROOT:="$(SDKROOT)/usr"
  ifeq ($(TARGET_PLATFORM),appletvos)
    # -fembedbitcode means we cannot do any undefined things like -undefined, -U<symbol>
    # At linktime, have to supply static link flags
    LDSHAREDCC=--env=LDSHARED:="$(CC) $(PYTHON_MODULELDFLAGS) $(LDFLAGS)"
  else
    LDSHAREDCC=--env=LDSHARED:="$(CC) -bundle -undefined dynamic_lookup $(LDFLAGS)"
  endif
endif

ifneq ($(SDKROOT),)
  SYSROOTENV=--sysroot $(SDKROOT)
  PKGCONFIGSYSROOT=--env=PKG_CONFIG_SYSROOT_DIR:=$(SDKROOT)
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py install
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 -m crossenv \
			                                       $(SYSROOTENV) \
			                                       --env=CPATH:=$(PREFIX)/include \
			                                       --env=LIBRARY_PATH:=$(PREFIX)/lib \
			                                       --env=PATH:=$(PREFIX)/bin \
			                                       --env=CFLAGS:="$(CFLAGS)" \
			                                       --env=CXXFLAGS:="$(CXXFLAGS)" \
			                                       --env=LDFLAGS:="$(LDFLAGS)" \
			                                       $(LDSHAREDCC) \
			                                       --env=PKG_CONFIG:=$(NATIVEPREFIX)/bin/pkg-config \
			                                       --env=PKG_CONFIG_PATH:=$(PREFIX)/lib/pkgconfig \
			                                       --env=PKG_CONFIG_LIBDIR:=$(PREFIX)/lib/pkgconfig \
			                                       $(PKGCONFIGSYSROOT) \
			                                       --env=PYTHONPATH:=$(PYTHON_SITE_PKG) \
			                                       $(ZLIBROOT) \
			                                       --cc "$(CC) $(CROSS_CC_FLAGS)" \
			                                       --cxx "$(CXX)" \
			                                       --ar "$(AR)" \
			                                       $(PREFIX)/bin/python3 \
			                                       $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)
