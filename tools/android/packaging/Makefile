include ../depends/Makefile.include

OBJS = libcurl.so \
  librtmp.so  libmad.so \
  libafpclient.so  \
  libshairport.so libplist.so \
  libxbogg.so libxbvorbis.so libxbvorbisfile.so libxbFLAC.so libxbmpeg2.so \
  libxbmpeg2convert.so

PLATFORM_OBJS =

XBMCROOT = $(shell cd $(CURDIR)/../../..; pwd)
COPYDIRS = system addons language media
AAPT = $(SDKROOT)/platform-tools/aapt
DX = $(SDKROOT)/platform-tools/dx
GCC_VERSION=$(shell $(CC) -dumpversion)

X86OVERRIDES=XBMC_OVERRIDE_HOST=i686-android-linux XBMC_OVERRIDE_TOOLCHAIN=$(XBMC_X86_TOOLCHAIN)
ARMOVERRIDES=XBMC_OVERRIDE_HOST=arm-linux-androideabi XBMC_OVERRIDE_TOOLCHAIN=$(XBMC_ARM_TOOLCHAIN)

#this fixes a android ndk fuckup where the pathes to 
#prebuilt stuff follow different name shemes for
#arm and x86
ifeq ($(findstring x86,$(PLATFORM)),x86)
GDBPATH=$(NDKROOT)/toolchains/$(PLATFORM)-$(GCC_VERSION)/prebuilt/gdbserver
else
GDBPATH=$(NDKROOT)/toolchains/$(HOST)-$(GCC_VERSION)/prebuilt/gdbserver
endif

all: package
SRCLIBS = $(addprefix $(PREFIX)/lib/,$(OBJS)) $(addprefix $(PREFIX)/lib/$(SDK_PLATFORM)/,$(PLATFORM_OBJS))
DSTLIBS = $(PLATFORM)/lib/libxbmc.so $(addprefix $(PLATFORM)/lib/,$(OBJS)) $(addprefix $(PLATFORM)/lib/,$(PLATFORM_OBJS))
libs= $(DSTLIBS)

multi: x86 arm
	@cp images/xbmcapp-debug-skeleton.apk images/xbmcapp-debug-multi-unaligned.apk
	@cd xbmc; zip -r -q ../images/xbmcapp-debug-multi-unaligned.apk lib/ assets
	@jarsigner -keystore ~/.android/debug.keystore -storepass android images/xbmcapp-debug-multi-unaligned.apk androiddebugkey
	@$(SDKROOT)/tools/zipalign -f 4 images/xbmcapp-debug-multi-unaligned.apk $(XBMCROOT)/xbmcapp-multi-debug.apk
	@rm images/xbmcapp-debug-multi-unaligned.apk
	@echo "$(XBMCROOT)/xbmcapp-multi-debug.apk created"

package: extras
	@cp images/xbmcapp-debug-skeleton.apk images/xbmcapp-debug-$(PLATFORM)-unaligned.apk
	@javac xbmc/src/org/xbmc/xbmc/xbmc.java
	@$(DX) --dex --output=xbmc/classes.dex xbmc/src/org/xbmc/xbmc/
	@cd xbmc; zip -r -q ../images/xbmcapp-debug-$(PLATFORM)-unaligned.apk lib/$(PLATFORM) assets classes.dex
	@jarsigner -keystore ~/.android/debug.keystore -storepass android images/xbmcapp-debug-$(PLATFORM)-unaligned.apk androiddebugkey
	@$(SDKROOT)/tools/zipalign -f 4 images/xbmcapp-debug-$(PLATFORM)-unaligned.apk $(XBMCROOT)/xbmcapp-$(PLATFORM)-debug.apk
	@rm images/xbmcapp-debug-$(PLATFORM)-unaligned.apk
	@echo "$(XBMCROOT)/xbmcapp-$(PLATFORM)-debug.apk created"

extras: libs
	rm -rf xbmc/assets/python2.6/lib/
	mkdir -p xbmc/assets xbmc/res xbmc/assets/python2.6/lib/
	cp -rfp $(PREFIX)/share/xbmc/* ./xbmc/assets
	find `pwd`/xbmc/assets/system/ -name "*.so" -exec rm {} \;
	find `pwd`/xbmc/assets/addons/skin.*/media/* -depth -not -iname "Textures.xbt" -exec rm -rf {} \;
	cp -rfp $(PREFIX)/lib/python2.6 xbmc/assets/python2.6/lib/
	cd xbmc/assets/python2.6/lib/python2.6/; rm -rf test config lib-dynload
	mkdir -p tmp/res; $(AAPT) c -S xbmc/res -C tmp/res; cp -r -n xbmc/res tmp/ || true
	$(AAPT) p -f -I $(SDKROOT)/platforms/$(SDK_PLATFORM)/android.jar -S tmp/res/ -M xbmc/AndroidManifest.xml -F images/xbmcapp-debug-skeleton.apk
	@rm -rf tmp/

libs: $(PREFIX)/lib/xbmc/libxbmc.so
	rm -rf xbmc/lib/$(PLATFORM) xbmc/libs/$(PLATFORM) xbmc/obj/local/$(PLATFORM)
	mkdir -p xbmc/lib/$(PLATFORM) images xbmc/assets/python2.6/lib/ xbmc/libs/$(PLATFORM) xbmc/obj/local/$(PLATFORM)
	cp -fp $(SRCLIBS) xbmc/obj/local/$(PLATFORM)/
	cp -fp $(PREFIX)/lib/xbmc/libxbmc.so xbmc/obj/local/$(PLATFORM)/
	find $(PREFIX)/lib/xbmc/addons -name "*.so" -exec cp -fp {} xbmc/obj/local/$(PLATFORM)/ \;
	find $(PREFIX)/lib/xbmc/system -name "*.so" -exec cp -fp {} xbmc/obj/local/$(PLATFORM)/ \;
	cd xbmc/obj/local/$(PLATFORM)/; find . -name "*.so" -not -name "lib*.so" | sed "s/\.\///" | xargs -I@ mv @ lib@
	cp -fp xbmc/obj/local/$(PLATFORM)/*.so xbmc/lib/$(PLATFORM)/
	$(STRIP) --strip-unneeded xbmc/lib/$(PLATFORM)/*.so
	install -p $(GDBPATH) ./xbmc/libs/$(PLATFORM)/gdbserver
	echo "set solib-search-path ./obj/local/$(PLATFORM)" > ./xbmc/libs/$(PLATFORM)/gdb.setup
	echo "directory $(TOOLCHAIN)/sysroot/usr/include $(NDKROOT)/sources/android/native_app_glue" \
	     "$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/include/$(GCC_VERSION) $(XBMCROOT)  $(PREFIX)/include jni" >> ./xbmc/libs/$(PLATFORM)/gdb.setup
	cp -fp xbmc/libs/$(PLATFORM)/* xbmc/lib/$(PLATFORM)

$(PREFIX)/lib/xbmc/libxbmc.so: $(SRCLIBS)
	$(MAKE) -C ../depends/xbmc

$(SRCLIBS):

arm: armeabi-v7a

armeabi-v7a: $(XBMC_ARM_TOOLCHAIN) force
	XBMC_OVERRIDE_PLATFORM=$@ $(ARMOVERRIDES) $(MAKE) package

x86: $(XBMC_X86_TOOLCHAIN) force
	XBMC_OVERRIDE_PLATFORM=$@ $(X86OVERRIDES) $(MAKE) package

.PHONY: arm armeabi-v7a x86 force extras libs
