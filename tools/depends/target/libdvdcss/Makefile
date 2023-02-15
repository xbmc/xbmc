-include ../../Makefile.include
include LIBDVDCSS-VERSION
DEPS = LIBDVDCSS-VERSION Makefile ../../download-files.include

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  PREFIX := $(ROOT_DIR)/dvdcss-install
  AUTORECONF := autoreconf

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
  endif
endif

# configuration settings
config = --prefix=$(PREFIX) --disable-shared --enable-static --with-pic

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif && ac_cv_path_GIT= ./configure $(config)

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

