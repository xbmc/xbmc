ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
-include ../../Makefile.include

ifeq ($(NATIVEPREFIX),)
  PREFIX = $(ROOT_DIR)
else
  PREFIX = $(NATIVEPREFIX)
endif

ifeq ($(NATIVEPLATFORM),)
  PLATFORM = native
else
  PLATFORM = $(NATIVEPLATFORM)
endif

SOURCE=$(ROOT_DIR)/src

CONFIGURE=./configure --prefix=$(PREFIX)
APP=$(SOURCE)/JsonSchemaBuilder
APPBIN=$(PREFIX)/bin/JsonSchemaBuilder

all: .installed-$(PLATFORM)

$(PLATFORM):
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); cp -a $(SOURCE)/* .
	cd $(PLATFORM); ./autogen.sh
	cd $(PLATFORM); $(CONFIGURE)


$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)


.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	-rm -rf bin
