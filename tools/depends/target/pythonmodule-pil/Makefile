include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=Pillow
VERSION=7.1.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PYTHONPATH=PYTHONPATH="$(PYTHON_SITE_PKG)"

BUILD_OPTS=--disable-jpeg2000 --disable-webp --disable-imagequant --disable-tiff --disable-webp --disable-webpmux --disable-xcb --disable-lcms --disable-platform-guessing

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	. $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)/bin/activate \
	&& cd $(PLATFORM) && $(PYTHONPATH) cross-python setup.py \
		build_ext $(BUILD_OPTS) install --prefix $(PREFIX)
ifeq ($(OS),android)
	cd $(PYTHON_SITE_PKG) && unzip -o ./Pillow-*.egg
	cd $(PYTHON_SITE_PKG) && rm -rf Pillow-*.egg
	cd $(PYTHON_SITE_PKG)/PIL && \
          sed -i -e 's/import sys/import os, sys /' \
                 -e '/__file__/ s/_imaging/lib_imaging/g' \
                 -e 's/pkg_resources.resource_filename(__name__,/os.path.join(os.environ["KODI_ANDROID_LIBS"], /'  _imaging*.py
	cd $(PYTHON_SITE_PKG)/PIL && find . -type d -name '__pycache__' -exec rm -rf {} +
else ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
# All apple platforms require the egg to be extracted
	cd $(PYTHON_SITE_PKG) && unzip -o ./Pillow-*.egg
	cd $(PYTHON_SITE_PKG) && rm -rf Pillow-*.egg
	cd $(PYTHON_SITE_PKG)/PIL && find . -type d -name '__pycache__' -exec rm -rf {} +
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
