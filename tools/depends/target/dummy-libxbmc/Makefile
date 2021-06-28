include ../../Makefile.include
VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
DEPS= ../../Makefile.include Makefile dummy-libxbmc.c

ifeq ($(TARGET_PLATFORM),appletvos)
  TARGET_FLAGS=$(CFLAGS) $(LDFLAGS)
endif

LIBDYLIB=lib$(APP_NAME).so

all: $(PLATFORM)/$(LIBDYLIB) .installed-$(PLATFORM)

$(PLATFORM):
	mkdir -p $(PLATFORM)

$(PLATFORM)/$(LIBDYLIB): $(PLATFORM) $(DEPS)
	$(CC) -shared $(TARGET_FLAGS) -o $(PLATFORM)/lib$(APP_NAME).so dummy-libxbmc.c

.installed-$(PLATFORM): $(PLATFORM)/$(LIBDYLIB)
	mkdir -p  $(PREFIX)/lib/dummy-lib$(APP_NAME)
	cp $(PLATFORM)/lib$(APP_NAME).so $(PREFIX)/lib/dummy-lib$(APP_NAME)
	touch $@

clean:
	rm -rf $(PLATFORM)
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
