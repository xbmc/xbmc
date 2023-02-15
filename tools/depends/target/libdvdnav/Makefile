-include ../../Makefile.include
include LIBDVDNAV-VERSION
DEPS = LIBDVDNAV-VERSION Makefile ../../download-files.include

# configuration settings
config = --prefix=$(PREFIX) --disable-shared --enable-static --with-pic
EXTRA_CFLAGS = -D_XBMC -DHAVE_DVDCSS_DVDCSS_H

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  PREFIX := $(ROOT_DIR)/dvdnav-install
  AUTORECONF := autoreconf

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    PKGCONFIGPATH = PKG_CONFIG_PATH="$(ROOT_DIR)/../libdvdread/dvdread-install/lib/pkgconfig:$(ROOT_DIR)/../libdvdcss/dvdcss-install/lib/pkgconfig"
    EXTRA_CFLAGS += -I$(ROOT_DIR)/../libdvdread/dvdread-install/include -I$(ROOT_DIR)/../libdvdcss/dvdcss-install/include
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
  endif
endif


ifeq ($(OS),osx)
  EXTRA_CFLAGS +=  -D__DARWIN__
endif
ifeq ($(OS),darwin_embedded)
  EXTRA_CFLAGS +=  -D__DARWIN__
endif

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif && $(PKGCONFIGPATH) CFLAGS="$(CFLAGS) $(EXTRA_CFLAGS)" ac_cv_path_GIT= ./configure $(config)

$(PLATFORM)/.libs/$(LIBNAME).a: $(PLATFORM)
	[ -d $(PLATFORM)/.libs ] && [ ! -f $@ ] && $(MAKE) -C $(PLATFORM) clean || :
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(PLATFORM)/.libs/$(LIBNAME).a
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

