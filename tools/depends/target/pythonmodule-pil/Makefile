include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=Pillow
VERSION=7.1.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PILPATH=$(PYTHON_SITE_PKG)
PYTHONPATH=PYTHONPATH="$(PYTHON_SITE_PKG)"
ifeq ($(OS),android)
  PILPATH=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
  PYTHONPATH=PYTHONPATH="$(PILPATH):$(PYTHON_SITE_PKG)"
else ifeq ($(OS),darwin_embedded)
  PILPATH=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
  PYTHONPATH=PYTHONPATH="$(PILPATH):$(PYTHON_SITE_PKG)"
endif

BUILD_OPTS=--plat-name $(OS)-$(CPU) --disable-jpeg2000 --disable-webp --disable-imagequant --disable-tiff --disable-webp --disable-webpmux --disable-xcb --disable-lcms --disable-platform-guessing

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(PILPATH)/lib:
ifeq ($(OS),android)
	mkdir -p $(PILPATH)/lib
else ifeq ($(OS),darwin_embedded)
	mkdir -p $(PILPATH)/lib
endif

.installed-$(PLATFORM): $(PLATFORM) $(PILPATH)/lib
	. $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)/bin/activate \
	&& cd $(PLATFORM) && $(PYTHONPATH) cross-python setup.py \
		build_ext $(BUILD_OPTS) install --install-lib $(PILPATH)
ifeq ($(OS),android)
	cd $(PILPATH)/lib && unzip -o ../Pillow-*.egg
	cd $(PILPATH)/lib/PIL && \
          sed -i -e 's/import sys/import os, sys /' \
                 -e '/__file__/ s/_imaging/lib_imaging/g' \
                 -e 's/pkg_resources.resource_filename(__name__,/os.path.join(os.environ["KODI_ANDROID_LIBS"], /'  _imaging*.py
	cd $(PILPATH) && rm -rf Pillow-*.egg
else ifeq ($(OS),darwin_embedded)
	cd $(PILPATH)/lib && unzip -o ../Pillow-*.egg
	cd $(PILPATH) && rm -rf Pillow-*.egg
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
