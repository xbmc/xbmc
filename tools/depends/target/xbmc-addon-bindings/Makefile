include ../../Makefile.include
include $(XBMCROOT)/xbmc/addons/addon-bindings.mk
DEPS= ../../Makefile.include Makefile $(XBMCROOT)/xbmc/addons/addon-bindings.mk

all: .installed-$(PLATFORM)

$(foreach header,$(notdir $(BINDINGS)),$(PREFIX)/include/xbmc/$(header)):
	@echo "Copy addon bindings to $(PREFIX)/include/xbmc"
	@mkdir -p $(PREFIX)/include/xbmc
	@for f in $(BINDINGS); do \
	  cp -f $(XBMCROOT)/$$f $(PREFIX)/include/xbmc ; \
	done

.installed-$(PLATFORM): $(DEPS) $(foreach header,$(notdir $(BINDINGS)),$(PREFIX)/include/xbmc/$(header))
	@mkdir -p $(PREFIX)/lib/xbmc
	@cat $(XBMCROOT)/project/cmake/xbmc-config.cmake.in | sed -e 's|@prefix@|$(PREFIX)|' -e 's|@ARCH_DEFINES@|$(ARCH_DEFINES)|' > $(PREFIX)/lib/xbmc/xbmc-config.cmake
	@cp -f $(XBMCROOT)/project/cmake/xbmc-addon-helpers.cmake $(PREFIX)/lib/xbmc
	@cp -f $(XBMCROOT)/project/cmake/AddOptions.cmake $(PREFIX)/lib/xbmc
	@touch $@

clean:
	@rm -rf .installed-$(PLATFORM) $(PREFIX)/include/xbmc

distclean:: clean
