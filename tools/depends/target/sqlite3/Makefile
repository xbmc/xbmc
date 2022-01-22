include ../../Makefile.include
DEPS = ../../Makefile.include Makefile sqlite3.c.patch ../../download-files.include

# lib name, version
LIBNAME=sqlite
VERSION=3360000
SOURCE=$(LIBNAME)-autoconf-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=e59c57f421b4956c7759af528a2da929167e15179ab9d28267474683e2272f93c901203e5a648732f1a3d43e7be3ac4217c3cdd7adf108c378b9127b771a7cd0
include ../../download-files.include

# configuration settings
export CXXFLAGS+=-DSQLITE_ENABLE_COLUMN_METADATA=1
export CFLAGS+=-DSQLITE_TEMP_STORE=3 -DSQLITE_DEFAULT_MMAP_SIZE=0x10000000
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared --enable-threadsafe --disable-readline

LIBDYLIB=$(PLATFORM)/.libs/lib$(LIBNAME)3.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
# seems MAP_POPULATE is broken on aarch64
ifneq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../sqlite3.c.patch
endif
	# do not build the program sqlite3
	sed -ie "s|bin_PROGRAMS = sqlite3||" "$(PLATFORM)/Makefile.am";
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

