include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile ../../download-files.include

LIBNAME=libheif
VERSION=1.16.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=d565bb81c09e5511379bce8300b7f8c51a60be42903383663599db5d67acf9286b87381182916b19d2dba18175019e4cdbcd6b8c80fa63c8c911d64b3fb558a7
include ../../download-files.include

CMAKE_OPTIONS := -DCMAKE_INSTALL_LIBDIR=lib \
                 -DCMAKE_INSTALL_PREFIX=$(NATIVEPREFIX) \
                 -DBUILD_SHARED_LIBS=OFF \
                 -DENABLE_PLUGIN_LOADING=OFF \
                 -DWITH_LIBDE265=OFF \
                 -DWITH_X265=OFF \
                 -DWITH_DAV1D=ON \
                 -DWITH_AOM_ENCODER=OFF \
                 -DWITH_AOM_DECODER=OFF \
                 -DWITH_SvtEnc=OFF \
                 -DWITH_RAV1E=OFF \
                 -DWITH_EXAMPLES=OFF \
                 -DWITH_GDK_PIXBUF=OFF \
                 -DWITH_REDUCED_VISIBILITY=ON \
                 -DWITH_DEFLATE_HEADER_COMPRESSION=OFF \
                 -DENABLE_MULTITHREADING_SUPPORT=ON \
                 -DENABLE_PARALLEL_TILE_DECODING=ON

BUILDDIR = $(PLATFORM)/build

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(NATIVEPREFIX)/bin/cmake $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
