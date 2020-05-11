include ../../Makefile.include
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=libicundk
VERSION=master
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL=https://gitlab.com/koying/libicundk/-/archive/master

CMAKE_OPTIONS := -DCMAKE_BUILD_TYPE=Release \
                 -DCMAKE_C_COMPILER="$(CC)" \
                 -DCMAKE_CXX_COMPILER="$(CXX)" \
                 -DCMAKE_C_FLAGS="$(CFLAGS)" \
                 -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
                 -DCMAKE_EXE_LINKER_FLAGS="$(LDFLAGS)" \
                 $(CMAKE_OPTIONS)
BUILDDIR = $(PLATFORM)/build-cmake
LIBDYLIB=$(PLATFORM)/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
