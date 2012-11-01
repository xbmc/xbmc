include ../Makefile.include
DEPS= ../Makefile.include Makefile dummy-libxbmc.c

LIBDYLIB=libxbmc.so

all: $(PLATFORM)/$(LIBDYLIB) .installed-$(PLATFORM)

$(PLATFORM):
	mkdir -p $(PLATFORM)

$(PLATFORM)/$(LIBDYLIB): $(PLATFORM) $(DEPS)
	$(CC) -shared -o $(PLATFORM)/libxbmc.so dummy-libxbmc.c

.installed-$(PLATFORM): $(PLATFORM)/$(LIBDYLIB)
	mkdir -p  $(PREFIX)/lib/dummy-libxbmc
	cp $(PLATFORM)/libxbmc.so $(PREFIX)/lib/dummy-libxbmc
	touch $@

clean:
	rm -rf $(PLATFORM)
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
