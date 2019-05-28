include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=waylandpp
VERSION=0.2.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

APP=$(PLATFORM)/wayland-scanner++

# Only this package uses CMake for build on native at the moment,
# so there is no separate toolchain file. Still we have to unset
# the CMAKE_TOOLCHAIN_FILE, which is part of $(CMAKE) and set to
# the target toolchain file.
CMAKE_OPTIONS := -DCMAKE_TOOLCHAIN_FILE= -DBUILD_DOCUMENTATION=OFF -DBUILD_LIBRARIES=OFF -DBUILD_EXAMPLES=OFF -DBUILD_SCANNER=ON -DCMAKE_C_COMPILER="$(CC_BINARY_FOR_BUILD)" -DCMAKE_CXX_COMPILER="$(CXX_BINARY_FOR_BUILD)" -DCMAKE_C_FLAGS="$(NATIVE_CFLAGS)" -DCMAKE_CXX_FLAGS="$(NATIVE_CXXFLAGS)" -DCMAKE_EXE_LINKER_FLAGS="$(NATIVE_LDFLAGS)" -DCMAKE_INSTALL_LIBDIR=lib $(CMAKE_OPTIONS)
BUILDDIR = $(PLATFORM)/build

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(APP): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
