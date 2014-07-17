-include ../../Makefile.include

ADDONS = "audioencoder.flac audioencoder.lame audioencoder.vorbis audioencoder.wav"
ifeq ($(PLATFORM),)
  PLATFORM = native
endif
ifeq ($(CMAKE),)
  CMAKE = cmake
endif

CMAKE_EXTRA =
ifeq (darwin, $(findstring darwin, $(HOST)))
  INSTALL_PREFIX = ../../../../../addons/
  CMAKE_EXTRA = -DPACKAGE_ZIP=1 -DDEPENDS_PATH=$(PREFIX)
else ifneq ($(PREFIX),)
  INSTALL_PREFIX = $(PREFIX)
  CMAKE_EXTRA = -DDEPENDS_PATH=$(PREFIX)
endif

all: .installed-$(PLATFORM)


clean:
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) native

.installed-$(PLATFORM):
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the xbmc install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	mkdir -p $(PLATFORM); \
         cd $(PLATFORM); \
         $(CMAKE) -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) -DCMAKE_PREFIX_PATH=$(PREFIX) $(CMAKE_EXTRA) \
         -DADDONS_TO_BUILD=$(ADDONS) ../../../../../project/cmake/addons/ ;\
         $(MAKE);
	touch $@
