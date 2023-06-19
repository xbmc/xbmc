include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile

APP=$(PLATFORM)/build/TexturePacker

CMAKE_OPTIONS := -DNATIVEPREFIX=${NATIVEPREFIX} \
                 -DKODI_SOURCE_DIR=$(CMAKE_SOURCE_DIR) \
                 -DENABLE_STATIC=1 \
                 -DARCH_DEFINES="$(NATIVE_ARCH_DEFINES)"

all: .installed-$(PLATFORM)

download:

$(PLATFORM): $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); cp -a ../src/* .
	cd $(PLATFORM)/build; $(CMAKE_FOR_BUILD) $(CMAKE_OPTIONS) ..

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	-rm -rf bin
