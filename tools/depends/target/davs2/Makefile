-include ../../Makefile.include
include DAVS2-VERSION
DEPS = Makefile DAVS2-VERSION ../../download-files.include

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  PREFIX ?= $(ROOT_DIR)/davs2-install
  BASE_URL ?= https://github.com/pkuvcl/davs2/archive/refs/tags
  AUTORECONF := autoreconf
  RETRIEVE_TOOL := curl
  RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
  ARCHIVE_TOOL := tar
  ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    HASH_TOOL = sha512sum
    HASH_TOOL_FLAGS = -c --status
  endif
endif

# configuration settings
CONFIGURE = ./configure --prefix=$(PREFIX) \
	--enable-static \
	--disable-shared \
	--enable-pic \
	--disable-cli

ifeq ($(CROSS_COMPILING), yes)
CONFIGURE += --host=$(HOST)
export CC=$(CC_FOR_BUILD)
export CXX=$(CXX_FOR_BUILD)
export CFLAGS=$(CFLAGS_FOR_BUILD)
export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
export CC CXX CFLAGS CXXFLAGS
endif

LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build/linux; $(CONFIGURE)
	$(MAKE) -C $(PLATFORM)/build/linux

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build/linux install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build/linux clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
