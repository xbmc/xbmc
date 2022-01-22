-include ../../Makefile.include
DEPS =Makefile ../../download-files.include

# lib name, version
LIBNAME=wayland-protocols
VERSION=1.20
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=56c99b1534ca12e094c0ba1a7d38e7551d38dd7dea80d1a35ae4cd60e8b28ddbd8f00374394da871bbfc91aa3a42f77ebed7d62a8fe6165684a385f2028a1bf4
include ../../download-files.include

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
else
	# Building as part of depends
	DEPS += ../../Makefile.include
endif

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) install
	ln -sf $(PREFIX)/share/pkgconfig/wayland-protocols.pc $(PREFIX)/lib/pkgconfig/wayland-protocols.pc
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
