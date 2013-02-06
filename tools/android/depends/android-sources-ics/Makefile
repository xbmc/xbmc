include ../Makefile.include
DEPS= ../Makefile.include Makefile 
INCLUDES += -I$(PREFIX)/opt/android-source/frameworks/base/include
INCLUDES += -I$(PREFIX)/opt/android-source/frameworks/base/native/include
INCLUDES += -I$(PREFIX)/opt/android-source/frameworks/base/include/media/stagefright
INCLUDES += -I$(PREFIX)/opt/android-source/frameworks/base/include/media/stagefright/openmax
INCLUDES += -I$(PREFIX)/opt/android-source/system/core/include
INCLUDES += -I$(PREFIX)/opt/android-source/libhardware/include

ANDROID_VERSION=ics-mr0
ANDROID_SOURCE=android-source
ANDROID_LIBS=android-libs

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): install-sources $(ANDROID_LIBS)
	rm -rf $(PREFIX)/opt/$(ANDROID_LIBS)
	mkdir -p $(PREFIX)/opt/$(ANDROID_LIBS)
	cp $(PLATFORM)/libstagefright.so $(PLATFORM)/libutils.so $(PLATFORM)/libbinder.so  $(PLATFORM)/libcutils.so $(PLATFORM)/libui.so $(PLATFORM)/libgui.so $(PREFIX)/opt/$(ANDROID_LIBS)
	touch $@

$(ANDROID_SOURCE):
	echo "Fetching Android system headers"
	git clone --depth=1 --branch $(ANDROID_VERSION) https://android.googlesource.com/platform/frameworks/base $(ANDROID_SOURCE)/frameworks/base
	git clone --depth=1 --branch $(ANDROID_VERSION) https://android.googlesource.com/platform/system/core $(ANDROID_SOURCE)/system/core
	git clone --depth=1 --branch $(ANDROID_VERSION) https://android.googlesource.com/platform/hardware/libhardware $(ANDROID_SOURCE)/libhardware

install-sources: $(ANDROID_SOURCE)
	mkdir -p $(PREFIX)/opt
	rm -f $(PREFIX)/opt/$(ANDROID_SOURCE)
	ln -s $(CURDIR)/$(ANDROID_SOURCE) $(PREFIX)/opt/$(ANDROID_SOURCE)

$(ANDROID_LIBS): $(PLATFORM)/libstagefright.so $(PLATFORM)/libutils.so $(PLATFORM)/libbinder.so $(PLATFORM)/libcutils.so $(PLATFORM)/libui.so $(PLATFORM)/libgui.so
	mkdir -p $(PREFIX)/opt/$(ANDROID_LIBS)
	cp $(PLATFORM)/libstagefright.so $(PLATFORM)/libutils.so $(PLATFORM)/libbinder.so  $(PLATFORM)/libcutils.so $(PLATFORM)/libui.so $(PLATFORM)/libgui.so $(PREFIX)/opt/$(ANDROID_LIBS)

$(PLATFORM): install-sources
	mkdir -p $(PLATFORM)

$(PLATFORM)/libstagefright.so:  $(PLATFORM) $(DEPS) libstagefright.cpp
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -DHAVE_PTHREADS -shared -o $(PLATFORM)/libstagefright.so libstagefright.cpp
	
$(PLATFORM)/libutils.so:  $(PLATFORM) $(DEPS) libutils.cpp
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libutils.so libutils.cpp

$(PLATFORM)/libbinder.so:  $(PLATFORM) $(DEPS) libbinder.cpp
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libbinder.so libbinder.cpp

$(PLATFORM)/libcutils.so:  $(PLATFORM) $(DEPS) libcutils.c
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libcutils.so libcutils.c

$(PLATFORM)/libui.so:  $(PLATFORM) $(DEPS) libui.cpp
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libui.so libui.cpp

$(PLATFORM)/libgui.so:  $(PLATFORM) $(DEPS) libgui.cpp
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libgui.so libgui.cpp

clean:
	rm -f $(PREFIX)/opt/$(ANDROID_SOURCE)
	rm -rf .installed-$(PLATFORM) $(ANDROID_SOURCE) $(PLATFORM) $(ANDROID_LIBS_ARCHIVE) $(ANDROID_LIBS)

distclean: clean
