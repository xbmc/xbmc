#warning: change if XBMCPREFIX gets changed.

include ../depends.mk

DEB=xbmca10.deb
TARBALL=xbmca10.tar.gz

all: $(TARBALL)

$(TARBALL): root/DEBIAN/control $(XBMCPREFIX)/lib/xbmc/xbmc.bin README
	(	cd root &&\
		rm -rf allwinner &&\
		tar cf /tmp/tmp.tar $(XBMCPREFIX) &&\
		tar xf /tmp/tmp.tar &&\
		$(STRIP) --strip-unneeded `find . -name xbmc.bin -o -name *.so -print` \
	)
	dpkg -b root $(DEB)
	tar cvfz $(TARBALL) README $(DEB)
	
clean:
	rm -f $(TARBALL) $(DEB)

