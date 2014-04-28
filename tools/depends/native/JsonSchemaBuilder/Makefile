-include ../../Makefile.include

ifeq ($(NATIVEPREFIX),)
  PREFIX = .
else
  PREFIX = $(NATIVEPREFIX)/bin
endif

all: $(PREFIX)/JsonSchemaBuilder

$(PREFIX)/JsonSchemaBuilder:
	make -C src
	cp src/JsonSchemaBuilder $(PREFIX)

clean:
	@rm -f JsonSchemaBuilder
	make -C src clean
