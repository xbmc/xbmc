CURDIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include ../../Makefile.include LIBICU-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBICU-VERSION Makefile ../../download-files.include

ifeq ($(PLATFORM),)
  PREFIX = $(NATIVEPREFIX)
  PLATFORM = $(NATIVEPLATFORM)
endif


# See https://unicode-org.github.io/icu/userguide/icu_data/buildtool.html 
# It is non-trivial to reduce size of libicudata, which by default is 32MB
#
LIBICU_LIB_TYPE = STATIC
SYSTEM_ICU = NO
DEBUG_ICU = NO
SHRINK_DATA_FILE = yes
ENABLE_TRACING = yes

TR=/usr/bin/tr

LIBICU_STATIC_LINKER_FLAGS=
#-l libicudata.a -l libicuuc.a -l libicui18n.a 

BUILD_ENV_VARS = U_DEFAULT_SHOW_DRAFT=0
BUILD_ENV_VARS += U_HIDE_DRAFT_API=1
BUILD_ENV_VARS += U_HIDE_DEPRECATED_API=1
BUILD_ENV_VARS += U_HIDE_OBSOLETE_UTF_OLD_H=1
BUILD_ENV_VARS += U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1
BUILD_ENV_VARS += UNISTR_FROM_CHAR_EXPLICIT=explicit
BUILD_ENV_VARS += UNISTR_FROM_STRING_EXPLICIT=explicit
BUILD_ENV_VARS += UCONFIG_NO_LEGACY_CONVERSION=1
BUILD_ENV_VARS += UNISTR_OBJECT_SIZE=256

BUILD_ENV_VARS += CC="$(CC)"
BUILD_ENV_VARS += CXX="$(CXX)"
BUILD_ENV_VARS += CXX_FLAGS="$(CXXFLAGS) -std=c++17"
BUILD_ENV_VARS += PKG_CONFIG=$(PKG_CONFIG_EXECUTABLE)


ConfigArgs = --disable-draft
ConfigArgs += --disable-extras
ConfigArgs += --enable-icu-config # A command
ConfigArgs += --disable-layoutex # Default
ConfigArgs += --disable-samples
ConfigArgs += --enable-strict
ConfigArgs += --disable-tests
ConfigArgs += --enable-tools
ConfigArgs += --host=$(HOST)

ConfigArgs += --prefix=$(PREFIX)
ConfigArgs += --srcdir=./
ConfigArgs += --libdir=$(PREFIX)/lib
ConfigArgs += --includedir=$(PREFIX)/include

# To reduce size

ConfigArgs += --disable-icuio
uconfig_local = 

ifeq ($(DEBUG_ICU), yes)
  ConfigArgs += --enable-debug 
  ConfigArgs += --disable-release
else
  ConfigArgs += --disable-debug
  ConfigArgs += --enable-release
endif

ifeq ($(LIBICU_LIB_TYPE), SHARED)
   ConfigArgs += --libdir=$(PREFIX)/lib
   ConfigArgs += --with-data-packaging=library
   BUILD_ENV_VARS += LDFLAGS=$(LIBICU_SHARED_LINKER_FLAGS)
else
   ConfigArgs += --enable-static
   ConfigArgs += --with-data-packaging=static
   BUILD_ENV_VARS += LDFLAGS=$(LIBICU_STATIC_LINKER_FLAGS)
endif

ifeq ($(ENABLE_TRACING), yes)
   ConfigArgs += --enable-tracing
endif

ifeq ($(SHRINK_DATA_FILE), yes)
    # There must be a better way

   ICU_DATA_FILTER_FILE=$(CURDIR)/filters.json
   BUILD_ENV_VARS += ICU_DATA_FILTER_FILE=$(ICU_DATA_FILTER_FILE)
   BUILD_ENV_VARS += UCONFIG_NO_FORMATTING=1
   BUILD_ENV_VARS += UCONFIG_NO_COLLATION=1
   BUILD_ENV_VARS += UCONFIG_NO_TRANSLITERATION=1
   
   uconfig_local += define UCONFIG_NO_FORMATTING 1$$
   uconfig_local += define UCONFIG_NO_COLLATION 1$$
   uconfig_local += define UCONFIG_NO_TRANSLITERATION 1
endif

ifeq ($(OS),android)
  ConfigArgs += --with-library-suffix=so
endif

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
  ConfigArgs += --with-cross-build=$(abs_top_srcdir)/native/libicu/$(NATIVEPLATFORM)/icu4c/source
else
  echo MAKEFILE_LIST: $(MAKEFILE_LIST)
  
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  PREFIX := $(NATIVEPREFIX)
  
  ifeq ($(PLATFORM),)
    # PLATFORM = native
    # TARBALLS_LOCATION = $(ROOT_DIR)
    EXTRA_CFLAGS += ""
  endif
endif

# Help for runConfig: ./runConfig --help
# Help for configure: ./configure --help

ICU_PLATFORM=Linux
ifeq ($(OS),osx)
  EXTRA_CFLAGS += -D__DARWIN__
endif
ifeq ($(OS),darwin_embedded)
  EXTRA_CFLAGS += -D__DARWIN__
endif

BUILD_ENV_VARS += CFLAGS="$(CFLAGS) $(EXTRA_CFLAGS)"

# configuration settings
CONFIGURE = $(BUILD_ENV_VARS) \
			./runConfigureICU $(ICU_PLATFORM) $(ConfigArgs)

LIBDYLIB=$(PLATFORM)/.libs/$(BYPRODUCT)

export CFLAGS LDFLAGS CPPFLAGS CXXFLAGS
export CC

export LD
export CC
export CXX
export CPP
export AR
export AS
export NM
export STRIP
export RANLIB
export OBJDUMP

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	echo OS: $(OS)
	echo HOST: $(HOST)
	echo ROOT_DIR: $(ROOT_DIR)
	echo PREFIX: $(PREFIX)
	echo NATIVEPREFIX: $(NATIVEPREFIX)
	echo PLATFORM: $(PLATFORM)
	echo NATIVEPLATFORM: $(NATIVEPLATFORM)
	echo TARGET: $(TARGET)
	echo CROSS_COMPILING: $(CROSS_COMPILING)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); pwd; $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo PATCH
	echo $(uconfig_local)
	cd $(PLATFORM)/icu4c/source/common/unicode; \
		echo $(uconfig_local) > uconfig_local.h.new; \
		$(TR) //$$ \\n < uconfig_local.h.new > uconfig_local.h; \
		sed -e '/define/s/define/#define/' uconfig_local.h >uconfig_local.h.tmp; mv uconfig_local.h.tmp uconfig_local.h; \
		sed -e '/^.*UCONFIG_USE_LOCAL.*$$/i #define UCONFIG_USE_LOCAL 1' uconfig.h >uconfig.h.tmp; \
		mv uconfig.h.tmp uconfig.h
	# cd $(PLATFORM)/icu4c/source/data/mappings; mkdir save; mv *.mk *.ucm save;
	cd $(PLATFORM)/icu4c/source; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/icu4c/source

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/icu4c/source install
	touch $@

clean:
	if [ -d $(PLATFORM)/icu4c/source ]; then \
	  $(MAKE) -C $(PLATFORM)/icu4c/source clean; \
	fi
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
