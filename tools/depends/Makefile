include Makefile.include

NATIVE=native/.installed-$(NATIVEPLATFORM)
target=target/.installed-$(PLATFORM)
all: $(target)

download:
	$(MAKE) -C native download
	$(MAKE) -C target download

$(NATIVE):
	$(MAKE) -C native

$(target): $(NATIVE)
	$(MAKE) -C target

clean:
	$(MAKE) -C native clean
	$(MAKE) -C target clean

test-dependencies:
	$(MAKE) -C native test-dependencies
	$(MAKE) -C native
	$(MAKE) -C target test-dependencies

distclean::
	$(MAKE) -C native distclean
	$(MAKE) -C target distclean

