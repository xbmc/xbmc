include ../depends.mk

export CFLAGS+=-marm

LOCATION=ftp://ftp.mars.org/pub/mpeg
TARBALL=libmad-0.15.1b.tar.gz

all: .installed

.installed: .builded
	(cd libmad*; sudo make install)
	touch $@

.builded: .configured
	(cd libmad*; make)
	touch $@

.configured: .extracted
	(  cd libmad* &&\
	   ./configure \
		--host=$(HOST) \
		--build=$(BUILD) \
		--prefix=$(SDKSTAGE)/usr \
		--libdir=$(SDKSTAGE)/usr/lib/arm-linux-gnueabi \
	)
	touch $@

.extracted: $(TARBALLS)/$(TARBALL)
	tar xvfz $(TARBALLS)/$(TARBALL)
	touch .extracted

$(TARBALLS)/$(TARBALL):
	wget $(LOCATION)/$(TARBALL) -O $(TARBALLS)/$(TARBALL)
	
clean:
	rm -rf .installed .builded .configured .extracted libmad*
	
	
	
