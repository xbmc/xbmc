include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

LIBNAME=date
VERSION=2.4.1
ARCHIVE=v$(VERSION).tar.gz
BASE_URL := https://github.com/HowardHinnant/$(LIBNAME)/archive

LIBDYLIB := $(PLATFORM)/build/libtz.a

CMAKE_OPTIONS := -DBUILD_SHARED_LIBS=OFF -DENABLE_DATE_TESTING=OFF -DUSE_SYSTEM_TZ_DB=OFF -DUSE_TZ_DB_IN_DOT=OFF

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
