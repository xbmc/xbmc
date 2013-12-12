include ../../Makefile.include
DEPS= ../../Makefile.include Makefile Imaging-1.1.7-access.patch \
      Imaging-1.1.7-crosscompiling-0.1.patch Imaging-1.1.7-setuptools-0.1.patch

# lib name, version
LIBNAME=Imaging
VERSION=1.1.7
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" LDFLAGS="$(LDFLAGS)"

LIBDYLIB=$(PLATFORM)/dist/PIL-$(VERSION)-py2.6-$(OS)-$(CPU).egg

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../Imaging-1.1.7-crosscompiling-0.1.patch
	cd $(PLATFORM); patch -p1 < ../Imaging-1.1.7-setuptools-0.1.patch
	cd $(PLATFORM); patch -p1 < ../Imaging-1.1.7-access.patch
	cd $(PLATFORM); patch -p0 < ../Imaging-1.1.7-zip-include.patch
	cd $(PLATFORM); patch -p0 < ../Imaging-1.1.7-remove-zlib-check.patch
	cd $(PLATFORM); sed -ie "s|"darwin"|"darwinNot"|g" "setup.py"

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python setup.py build -x bdist_egg --plat-name $(OS)-$(CPU)

.installed-$(PLATFORM): $(LIBDYLIB)
	mkdir -p $(PREFIX)/lib/python2.6/site-packages/PIL
	unzip -oq $(LIBDYLIB) -d $(PREFIX)/lib/python2.6/site-packages/PIL/
	echo 'PIL' > $(PREFIX)/lib/python2.6/site-packages/PIL.pth
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
