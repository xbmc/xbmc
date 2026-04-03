include ../../Makefile.include
DEPS = ../../Makefile.include Makefile dummy-libxbmc.c

LIBDYLIB=lib$(APP_NAME).so

all: $(PLATFORM)/$(LIBDYLIB) .installed-$(PLATFORM)

.configured-$(PLATFORM):
	mkdir -p $(PLATFORM)
	touch $@

$(PLATFORM)/$(LIBDYLIB): .configured-$(PLATFORM) $(DEPS)
	$(CC) -shared -o $(PLATFORM)/lib$(APP_NAME).so dummy-libxbmc.c

.installed-$(PLATFORM): $(PLATFORM)/$(LIBDYLIB)
	mkdir -p  $(PREFIX)/lib/dummy-lib$(APP_NAME)
	cp $(PLATFORM)/lib$(APP_NAME).so $(PREFIX)/lib/dummy-lib$(APP_NAME)
	touch $@

clean:
	rm -rf $(PLATFORM)
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
