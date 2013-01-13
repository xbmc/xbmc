include ../Makefile.include
DEPS= ../Makefile.include Makefile libstagefright.cpp libutils.cpp libbinder.cpp libcutils.c
INCLUDES+=-I$(PREFIX)/opt/android-source/frameworks/base/include -I$(PREFIX)/opt/android-source/frameworks/base/native/include -I$(PREFIX)/opt/android-source/system/core/include -I$(PREFIX)/opt/android-source/frameworks/base/media/libstagefright -I$(PREFIX)/opt/android-source/frameworks/base/include/media/stagefright/openmax -I$(PREFIX)/opt/android-source/frameworks/base/media/libstagefright -I$(PREFIX)/opt/android-source/libhardware/include

ANDROID_VERSION=ics
ANDROID_SOURCE=android-source
ANDROID_LIBS=android-libs

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): $(ANDROID_SOURCE) $(ANDROID_LIBS)
	touch $@

$(ANDROID_SOURCE):
	echo "Fetching Android system headers"
	git clone --depth=1 --branch $(ANDROID_VERSION)-release git://github.com/CyanogenMod/android_frameworks_base.git $(ANDROID_SOURCE)/frameworks/base
	git clone --depth=1 --branch $(ANDROID_VERSION)-release git://github.com/CyanogenMod/android_system_core.git $(ANDROID_SOURCE)/system/core
	git clone --depth=1 --branch $(ANDROID_VERSION)-release git://github.com/CyanogenMod/android_hardware_libhardware.git $(ANDROID_SOURCE)/libhardware
	mkdir -p $(PREFIX)/opt
	ln -s $(CURDIR)/$(ANDROID_SOURCE) $(PREFIX)/opt/$(ANDROID_SOURCE)

$(ANDROID_LIBS): $(PLATFORM)/libstagefright.so $(PLATFORM)/libutils.so $(PLATFORM)/libbinder.so $(PLATFORM)/libcutils.so
	mkdir -p $(PREFIX)/opt/$(ANDROID_LIBS)
	cp $(PLATFORM)/libstagefright.so $(PLATFORM)/libutils.so $(PLATFORM)/libbinder.so  $(PLATFORM)/libcutils.so $(PREFIX)/opt/$(ANDROID_LIBS)

$(PLATFORM):
	mkdir -p $(PLATFORM)

$(PLATFORM)/libstagefright.so:  $(PLATFORM) $(DEPS)
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -DHAVE_PTHREADS -shared -o $(PLATFORM)/libstagefright.so libstagefright.cpp
	
$(PLATFORM)/libutils.so:  $(PLATFORM) $(DEPS)
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libutils.so libutils.cpp

$(PLATFORM)/libbinder.so:  $(PLATFORM) $(DEPS)
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libbinder.so libbinder.cpp

$(PLATFORM)/libcutils.so:  $(PLATFORM) $(DEPS)
	$(CXX) -Wno-multichar -fno-exceptions -fno-rtti $(INCLUDES) -shared -o $(PLATFORM)/libcutils.so libcutils.c

clean:
	rm -rf .installed-$(PLATFORM) $(ANDROID_SOURCE) $(ANDROID_LIBS) $(ANDROID_LIBS_ARCHIVE)

distclean: clean
