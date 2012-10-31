include ../depends.mk

LCLPREFIX=$(shell echo $(XBMCPREFIX) | sed 's/^\///g')

DEB=xbmca10.deb
TARBALL=xbmca10.tar.gz

all: $(TARBALL)

$(TARBALL): root/DEBIAN/control $(XBMCPREFIX)/lib/xbmc/xbmc.bin README
	(	rm -rf tmp &&\
		cp -r root tmp &&\
		cd tmp &&\
		tar cf - $(XBMCPREFIX) | tar xf - &&\
		rm -rf $(LCLPREFIX)/include &&\
		rm -rf $(LCLPREFIX)/lib/pkgconfig &&\
		rm -f  $(LCLPREFIX)/lib/*.a &&\
		$(STRIP) --strip-unneeded `find . -name xbmc.bin -print` &&\
		$(STRIP) --strip-unneeded `find . -name *.so -print` \
	)
	dpkg -b tmp $(DEB)
	tar cvfz $(TARBALL) README $(DEB)
	rm -r tmp $(DEB)
	
clean:
	rm -rf tmp
	rm -f $(TARBALL) $(DEB)

