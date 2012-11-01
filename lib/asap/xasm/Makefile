VERSION = 3.0.2

ASCIIDOC = asciidoc -o $@ -a doctime
ASCIIDOC_POSTPROCESS = perl -pi.bak -e "s/527bbd;/20a0a0;/;END{unlink '$@.bak'}" $@
ASCIIDOC_VALIDATE = xmllint --valid --noout --nonet $@
RM = rm -f
ZIP = 7z a -mx=9 -tzip $@
GDC ?= gdc

all: xasm

xasm.o: xasm.d
	$(GDC) -c -o xasm.o -O2 $<

xasm: xasm.o
	$(GDC) -o xasm xasm.o

xasm.html: xasm.1.txt
	$(ASCIIDOC) -d manpage $<
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

dist: xasmpage-$(VERSION).zip

xasmpage-$(VERSION).zip: xasm261.zip xasm-$(VERSION)-src.zip xasm-$(VERSION)-windows.zip inflate6502.zip index.html inflate.html scite.png
	$(RM) $@
	$(ZIP) $^

xasm-$(VERSION)-src.zip: xasm.d Makefile xasm.1.txt
	$(RM) $@
	$(ZIP) $^

xasm-$(VERSION)-windows.zip: xasm.exe xasm.html xasm.properties
	$(RM) $@
	$(ZIP) $^

inflate6502.zip: inflate.asx gzip2deflate.c gzip2deflate.exe
	$(RM) $@
	$(ZIP) $^

gzip2deflate.exe: gzip2deflate.c
	mingw32-gcc -s -O2 -Wall -o $@ $<

index.html: index.txt
	$(ASCIIDOC) $<
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

inflate.html: inflate.txt
	$(ASCIIDOC) $<
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

clean:
	$(RM) xasm.o xasm

.DELETE_ON_ERROR:
