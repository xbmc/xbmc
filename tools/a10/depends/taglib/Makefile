include ../depends.mk

LOCATION=https://github.com/downloads/taglib/taglib
TARBALL=taglib-1.8.tar.gz

all: .installed

.installed: .builded
	$(MAKE) -C taglib*/build install
	touch $@

.builded: .extracted
	(	cd taglib* && rm -rf build && mkdir build && cd build &&\
		CC=$(CC) CXX=$(CXX) cmake        \
        	-DENABLE_STATIC=1 \
          	-DCMAKE_C_FLAGS:STRING="$(CPPFLAGS) $(CFLAGS)" \
          	-DCMAKE_CXX_FLAGS:STRING="$(CPPFLAGS) $(CXXFLAGS)" \
          	-DCMAKE_INCLUDE_PATH=$(PREFIX)/include \
          	-DCMAKE_LIBRARY_PATH=$(PREFIX)/lib \
          	-DCMAKE_INSTALL_NAME_DIR=$(PREFIX)/lib \
          	-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
          	-DCMAKE_FIND_ROOT_PATH="$(SDKSTAGE):/" .. \
    )
	touch $@

.extracted: $(TARBALLS)/$(TARBALL)
	tar xvfz $(TARBALLS)/$(TARBALL)
	touch .extracted

$(TARBALLS)/$(TARBALL):
	wget $(LOCATION)/$(TARBALL) -O $(TARBALLS)/$(TARBALL)
	
clean:
	rm -rf .installed .builded .configured .extracted taglib*
	
	
	
