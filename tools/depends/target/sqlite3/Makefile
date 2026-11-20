include ../../Makefile.include SQLITE3-VERSION ../../download-files.include
DEPS = ../../Makefile.include SQLITE3-VERSION Makefile ../../download-files.include \
                                sqlite3.c.patch

# configuration settings
export CXXFLAGS+=-DSQLITE_ENABLE_COLUMN_METADATA=1
export CFLAGS+=-DSQLITE_TEMP_STORE=3 -DSQLITE_DEFAULT_MMAP_SIZE=0x10000000
# autosetup does not read config.site, so pass the toolchain explicitly
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) \
          --disable-shared --enable-threadsafe --disable-readline \
          CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" AR="$(AR)" \
          CPPFLAGS="$(CPPFLAGS)"

LIBDYLIB=$(PLATFORM)/lib$(LIBNAME)3.a

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
# seems MAP_POPULATE is broken on aarch64
ifneq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../sqlite3.c.patch
endif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

# only build the library, not the sqlite3 program
$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM) lib$(LIBNAME)3.a

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install-lib install-headers install-pc
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
