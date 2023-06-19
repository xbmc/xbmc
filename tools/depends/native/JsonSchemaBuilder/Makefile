include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile

APP=$(PLATFORM)/build/JsonSchemaBuilder

all: .installed-$(PLATFORM)

download:

$(PLATFORM): $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); cp -a ../src/* .
	cd $(PLATFORM)/build; $(CMAKE_FOR_BUILD) ..

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
