-include ../../Makefile.include
DEPS = Makefile

LIBNAME=Neptune
VERSION=da409293e2e075df17b4ae2f2afcc03631615b55
BASE_URL=https://github.com/lrusak/Neptune/archive
ARCHIVE=$(VERSION).tar.gz

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  PREFIX ?= $(ROOT_DIR)/neptune-install
  RETRIEVE_TOOL := curl -Ls --retry 10 --retry-delay 3 -o $(ARCHIVE)
  ARCHIVE_TOOL := tar --strip-components=1 -xf

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    NEPTUNE_INSTALL_PREFIX = -DCMAKE_INSTALL_PREFIX=$(PREFIX)
    CMAKE = cmake
  endif
endif

ifeq ($(OS),android)
  CMAKE_OPTIONS := -DTARGET_ANDROID=ON
else
  CMAKE_OPTIONS := -DTARGET_ANDROID=OFF
endif

# configuration settings
CONFIGURE = $(CMAKE) $(NEPTUNE_INSTALL_PREFIX) \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF \
            $(CMAKE_OPTIONS)

ifeq ($(CROSS_COMPILING), yes)
  export CC=$(CC_FOR_BUILD)
  export CXX=$(CXX_FOR_BUILD)
  export CFLAGS=$(CFLAGS_FOR_BUILD)
  export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
  export CC CXX CFLAGS CXXFLAGS
endif

LIBDYLIB=$(PLATFORM)/lib/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

download: $(TARBALLS_LOCATION)/$(ARCHIVE)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CONFIGURE) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	rm -rf $(PLATFORM)/build
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
