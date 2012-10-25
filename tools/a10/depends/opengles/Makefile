include ../depends.mk

MALISDK=Mali_OpenGL_ES_2.0_SDK_for_Linux_On_ARM_v1.2.0
SUBVERSION1=.9310_Linux
LOCATION1=http://malideveloper.com/files
TARBALL1=$(MALISDK)$(SUBVERSION1).tar.gz

GLESLIBS=mali-libs
LOCATION2=https://github.com/linux-sunxi/mali-libs/tarball/master
TARBALL2=$(GLESLIBS).tar.gz
ifeq ($(USEARMHF), 1)
LIBVERSION=r3p0/armhf/framebuffer
else
LIBVERSION=r2p4/armel/framebuffer
endif

all: .installed

.installed: .include .lib
	install -d ${XBMCPREFIX}/lib
	cp -a lib/* ${XBMCPREFIX}/lib
	touch $@

.include:	$(TARBALLS)/$(TARBALL1)
	rm -rf tmp include
	mkdir tmp
	(	cd tmp &&\
		tar xvfz $(TARBALLS)/$(TARBALL1) &&\
		cd $(MALISDK) &&\
		mv inc ../../include &&\
		cp simple-framework/inc/mali/EGL/fbdev_window.h ../../include/EGL &&\
		cd ../../ &&\
		rm -r tmp &&\
		chmod -R ugo+r include &&\
		chmod ugo+x include include/* &&\
		chmod ugo-x include/*/*.h \
	)
	touch $@	
	
$(TARBALLS)/$(TARBALL1):
	wget $(LOCATION1)/$(TARBALL1) -O $(TARBALLS)/$(TARBALL1)

.lib: $(TARBALLS)/$(TARBALL2)
	rm -rf tmp lib
	mkdir tmp
	(	cd tmp &&\
		tar xvfz $(TARBALLS)/$(TARBALL2) &&\
		mv linux-sunxi-mali-libs*/$(LIBVERSION)/lib .. &&\
		cd .. &&\
		rm -r tmp\
	)
	touch $@

$(TARBALLS)/$(TARBALL2):
	install -d $(TARBALLS)
	wget $(LOCATION2) -O $(TARBALLS)/$(TARBALL2)
	
clean:
	rm -rf .installed .lib lib .include include
