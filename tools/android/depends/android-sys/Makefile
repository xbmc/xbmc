include ../Makefile.include
DEPS = ../Makefile.include Makefile iomx.cpp

LIBNAME=android-sys
VERSION=0.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PRIVATE_LIBDIR=$(PLATFORM)/android-libs
PRIVATE_LIBS =$(PRIVATE_LIBDIR)/libutils.so
PRIVATE_LIBS+=$(PRIVATE_LIBDIR)/libmedia.so
PRIVATE_LIBS+=$(PRIVATE_LIBDIR)/libbinder.so
PRIVATE_LIBS+=$(PRIVATE_LIBDIR)/libstagefright.so

CXXFLAGS =$(PLATFORM_FLAGS) -I$(PREFIX)/include -fexceptions
LDFLAGS += -L$(PRIVATE_LIBDIR) -lmedia -lutils -lbinder
INCLUDES+=-I$(PLATFORM)/$(SDK_PLATFORM)/system/core/include
INCLUDES+=-I$(PLATFORM)/$(SDK_PLATFORM)/frameworks/base/include
INCLUDES+=-I$(PLATFORM)/$(SDK_PLATFORM)/frameworks/base/include/media/stagefright/openmax

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(PRIVATE_LIBS:.so=.symbols): $(PLATFORM)

$(PRIVATE_LIBS:.so=.c): $(PRIVATE_LIBS:.so=.symbols)
	rm -f $@
	for s in `cat $(@:.c=.symbols)`; do echo "void $$s() {}" >> $@; done

$(PRIVATE_LIBS): $(PRIVATE_LIBS:.so=.c)
	$(CC) $(@:.so=.c) -shared -o $@

$(PLATFORM)/$(SDK_PLATFORM)/libiomx.so: iomx.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -o $@ iomx.cpp $(LDFLAGS) -lstagefright

.installed-$(PLATFORM): $(PRIVATE_LIBS) $(PLATFORM)/$(SDK_PLATFORM)/libiomx.so
	mkdir -p $(PREFIX)/include/$(SDK_PLATFORM)
	mkdir -p $(PREFIX)/lib/$(SDK_PLATFORM)
	cp -f $(PLATFORM)/$(SDK_PLATFORM)/libiomx.so $(PREFIX)/lib/$(SDK_PLATFORM)/
	cp -f $(PLATFORM)/$(SDK_PLATFORM)/frameworks/base/include/media/stagefright/openmax/* $(PREFIX)/include/$(SDK_PLATFORM)/
	touch $@

clean:
	rm -f .installed-$(PLATFORM) $(PLATFORM)/libiomx.so $(PRIVATE_LIBS) $(PRIVATE_LIBS:.so=.c)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
