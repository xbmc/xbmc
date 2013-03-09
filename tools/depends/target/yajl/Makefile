include ../../Makefile.include
DEPS= ../../Makefile.include Makefile static.patch

# lib name, version
LIBNAME=yajl
VERSION=2.0.1
SOURCE=lloyd-$(LIBNAME)-f4b2b1a
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
# configuration settings
LIBDYLIB=$(PLATFORM)/build/$(LIBNAME)-$(VERSION)/lib/libyajl.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	sed -ie "s|ADD_CUSTOM_TARGET|#ADD_CUSTOM_TARGET|g" "$(PLATFORM)/CMakeLists.txt"
	sed -ie "s|./run_tests.sh|#./run_tests.sh|g" "$(PLATFORM)/CMakeLists.txt"
	sed -ie "s|WORKING_DIRECTORY|#WORKING_DIRECTORY|g" "$(PLATFORM)/CMakeLists.txt"
	sed -ie "s|ADD_SUBDIRECTORY(reformatter)|#ADD_SUBDIRECTORY(reformatter)|g" "$(PLATFORM)/CMakeLists.txt"
	cd $(PLATFORM); patch -p0 < ../static.patch
	cd $(PLATFORM); patch -p0 < ../shared.patch
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
