include ../Makefile.include
DEPS= ../Makefile.include Makefile removelegacy.patch android.patch

# lib name, version
LIBNAME=libssh
VERSION=0.5.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME).so.4.2.0

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); patch -p0 < ../removelegacy.patch
	cd $(PLATFORM); patch -p0 < ../android.patch
	cd $(PLATFORM); patch -p0 < ../md5.patch
	sed -ie "s|-fstack-protector|-fnostack-protector|" "$(PLATFORM)/cmake/Modules/DefineCompilerFlags.cmake"
	sed -ie "s|add_subdirectory(examples)||" "$(PLATFORM)/CMakeLists.txt"
	cd $(PLATFORM)/build; CC=$(CC) CXX=$(CXX) cmake    \
	-DCMAKE_C_FLAGS:STRING="$(CPPFLAGS) $(CFLAGS)"     \
	-DCMAKE_CXX_FLAGS:STRING="$(CPPFLAGS) $(CXXFLAGS)" \
	-DCMAKE_INSTALL_NAME_DIR=$(PREFIX)/lib             \
	-DCMAKE_INSTALL_PREFIX=$(PREFIX)                   \
	-DCMAKE_SYSTEM_NAME=Linux                          \
	-DWITH_STATIC_LIB=1                                \
	-DCMAKE_FIND_ROOT_PATH="$(TOOLCHAIN)/sysroot/usr;$(PREFIX)" ..
$(LIBDYLIB): $(PLATFORM)
	make -j 1 -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	make -C $(PLATFORM)/build install
	rm -f $(PREFIX)/lib/libssh.so $(PREFIX)/lib/libssh.so.4.2.0 $(PREFIX)/lib/libssh.so.4
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
