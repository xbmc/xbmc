-include ../../Makefile.include

ifeq ($(NATIVEPREFIX),)
  PREFIX = .
else
  PREFIX = $(NATIVEPREFIX)/bin
endif

ifeq ($(NATIVEPLATFORM),)
  PLATFORM = native
else
  PLATFORM = $(NATIVEPLATFORM)
endif

all: .installed-$(PLATFORM)

$(PREFIX)/JsonSchemaBuilder:
	make -C src
	cp src/JsonSchemaBuilder $(PREFIX)

clean:
	make -C src clean
	@rm -f JsonSchemaBuilder .installed-$(PLATFORM)

.installed-$(PLATFORM): $(PREFIX)/JsonSchemaBuilder
	touch $@
