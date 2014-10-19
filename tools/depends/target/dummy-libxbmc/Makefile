include ../../Makefile.include
VERSION.TXT := $(XBMCROOT)/version.txt
DEPS= ../../Makefile.include Makefile dummy-libxbmc.c $(VERSION.TXT)
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

LIBDYLIB=lib$(APP_NAME).so

all: $(PLATFORM)/$(LIBDYLIB) .installed-$(PLATFORM)

$(PLATFORM):
	mkdir -p $(PLATFORM)

$(PLATFORM)/$(LIBDYLIB): $(PLATFORM) $(DEPS)
	$(CC) -shared -o $(PLATFORM)/lib$(APP_NAME).so dummy-libxbmc.c

.installed-$(PLATFORM): $(PLATFORM)/$(LIBDYLIB)
	mkdir -p  $(PREFIX)/lib/dummy-lib$(APP_NAME)
	cp $(PLATFORM)/lib$(APP_NAME).so $(PREFIX)/lib/dummy-lib$(APP_NAME)
	touch $@

clean:
	rm -rf $(PLATFORM)
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
