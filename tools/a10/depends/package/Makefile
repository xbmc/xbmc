#warning: change if XBMCPREFIX gets changed.

include ../depends.mk

LCLPREFIX=allwinner/xbmc-pvr-bin

DEB=xbmca10.deb
TARBALL=xbmca10.tar.gz

all: $(TARBALL)

$(TARBALL): root/DEBIAN/control $(XBMCPREFIX)/lib/xbmc/xbmc.bin README
	(	cd root &&\
		rm -rf $(LCLPREFIX) &&\
		tar cf /tmp/tmp.tar $(XBMCPREFIX) &&\
		tar xf /tmp/tmp.tar &&\
		rm -rf $(LCLPREFIX)/include &&\
		rm -rf $(LCLPREFIX)/lib/pkgconfig &&\
		rm -f  $(LCLPREFIX)/lib/*.a &&\
		$(STRIP) --strip-unneeded `find . -name xbmc.bin -o -name *.so -print` \
	)
	dpkg -b root $(DEB)
	tar cvfz $(TARBALL) README $(DEB)
	
clean:
	rm -rf root/$(LCLPREFIX)
	rm -f $(TARBALL) $(DEB)

