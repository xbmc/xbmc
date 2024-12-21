include ../../Makefile.include
include BOOST-VERSION
include ../../download-files.include

DEPS := \
  ../../download-files.include \
  ../../Makefile.include \
  BOOST-VERSION \
  Makefile

# The comma-separated list of boost libraries to build
# This list is for libtorrent - required by cmakebuild
# asio,beast,crc,config,context,core,json,logic,multi_index,optional,system,multiprecision,pool
# b2 only requires actual linkable lib names
BOOST_LIBRARIES=context,json,system

# Boost build options
BOOST_OPTIONS := \
  link=static \
  cflags="$(CFLAGS)" \
  cxxflags="$(CXXFLAGS)" \
  linkflags="$(LDFLAGS)" \
  cxxstd=17

ifeq ($(DEBUG_BUILD),yes)
  BOOST_OPTIONS += variant=debug
else
  BOOST_OPTIONS += variant=release
endif

BOOST_OPTIONS += toolset=gcc-cross
ifneq (,$(findstring arm64,$(CPU)))
  BOOST_OPTIONS += architecture=arm address-model=64
else ifneq (,$(findstring arm,$(CPU)))
  BOOST_OPTIONS += architecture=arm address-model=32
else ifeq ($(CPU),i686)
  BOOST_OPTIONS += architecture=x86 address-model=32
else ifeq ($(CPU),x86_64)
  BOOST_OPTIONS += architecture=x86 address-model=64
endif

# TODO: FreeBSD (verify), iOS, tvOS
ifeq ($(OS),osx)
  BOOST_OPTIONS += target-os=darwin
else ifeq ($(OS),darwin_embedded)
  BOOST_OPTIONS += target-os=darwin
else
  BOOST_OPTIONS += target-os=$(OS)
endif

# The generated library
LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(PLATFORM)/b2: $(PLATFORM)
	cd $(PLATFORM); ./bootstrap.sh \
	  --prefix=$(PREFIX) \
	  --with-python=$(NATIVEPREFIX)/bin/python3 \
	  --with-libraries=$(BOOST_LIBRARIES) \
	  --without-icu
ifeq ($(CROSS_COMPILING), yes)
	echo "using gcc : cross : $(CXX) ;" >> $(PLATFORM)/project-config.jam
endif

$(LIBDYLIB): $(PLATFORM)/b2
	cd $(PLATFORM); ./b2 $(BOOST_OPTIONS)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); ./b2 $(BOOST_OPTIONS) install
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
