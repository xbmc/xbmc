-include ../../Makefile.include
include DVDCSS-VERSION
DEPS= DVDCSS-VERSION Makefile

ifeq ($(CROSS_COMPILING), yes)
#  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  PREFIX := $(ROOT_DIR)/dvdcss-install
  AUTORECONF := autoreconf

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
  endif
endif

RETRIEVE_TOOL := curl -Ls --create-dirs --retry 10 --retry-delay 3
ARCHIVE_TOOL := tar --strip-components=1 -xf

# configuration settings
config = --build=$(BUILD_SYS) --host=$(HOST_SYS) --prefix=$(PREFIX) --disable-shared --enable-static --with-pic

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

GITREV=$(shell git ls-remote $(BASE_URL) $(VERSION) | awk '{print $$1}')
ifeq ($(GITREV),)
  ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
else
  ARCHIVE=$(LIBNAME)-$(GITREV).tar.gz
endif


all: .installed-$(PLATFORM)

download: $(TARBALLS_LOCATION)/$(ARCHIVE)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) -o $(TARBALLS_LOCATION)/$(ARCHIVE) $(BASE_URL)/archive/$(VERSION).tar.gz

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(TARBALLS_LOCATION)/$(ARCHIVE)
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

