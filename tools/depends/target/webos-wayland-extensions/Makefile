-include ../../Makefile.include
include WEBOS-WAYLAND-EXTENSIONS-VERSION
DEPS = WEBOS-WAYLAND-EXTENSIONS-VERSION Makefile ../../download-files.include

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
endif

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	mkdir -p $(PREFIX)/share/wayland-webos $(PREFIX)/lib/pkgconfig
	cp $(PLATFORM)/protocol/webos-shell.xml $(PREFIX)/share/wayland-webos
	cp $(PLATFORM)/protocol/webos-foreign.xml $(PREFIX)/share/wayland-webos
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
