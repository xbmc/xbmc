include ../../Makefile.include
DEPS= ../../Makefile.include Makefile removelegacy.patch android.patch ntohl.patch md5.patch

# lib name, version
LIBNAME=libssh
VERSION=0.7.0a
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); patch -p0 < ../removelegacy.patch
	cd $(PLATFORM); patch -p0 < ../android.patch
	cd $(PLATFORM); patch -p0 < ../ntohl.patch
	cd $(PLATFORM); patch -p0 < ../md5.patch
	cd $(PLATFORM); patch -p1 < ../darwin.patch
	sed -ie "s|-fstack-protector|-fnostack-protector|" "$(PLATFORM)/cmake/Modules/DefineCompilerFlags.cmake"
	sed -ie "s|add_subdirectory(examples)||" "$(PLATFORM)/CMakeLists.txt"
	cd $(PLATFORM)/build; $(CMAKE) -DWITH_STATIC_LIB=1 -DWITH_EXAMPLES=0 -DTHREADS_PTHREAD_ARG=0 -DWITH_GSSAPI=0 VERBOSE=1 ..

$(LIBDYLIB): $(PLATFORM)
	make -j 1 -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	make -C $(PLATFORM)/build install
	rm -f $(PREFIX)/lib/libssh.so $(PREFIX)/lib/libssh.so.4.3.0 $(PREFIX)/lib/libssh.so.4
	rm -f $(PREFIX)/lib/libssh.*dylib*
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
