CC=gcc

ifeq ($(ARCH), osx)
	CFLAGS=-O2 -fPIC -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
	CXXFLAGS==-fPIC -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
else
	CFLAGS=-O2 -fPIC -D_LINUX
endif

OBJS=fax3sm_winnt.o tif_aux.o tif_close.o tif_codec.o tif_compress.o tif_dir.o tif_dirinfo.o tif_dirread.o tif_dirwrite.o tif_dumpmode.o tif_error.o tif_fax3.o tif_flush.o tif_getimage.o tif_jpeg.o tif_luv.o tif_lzw.o tif_next.o tif_ojpeg.o tif_open.o tif_packbits.o tif_pixarlog.o tif_predict.o tif_print.o tif_read.o tif_strip.o tif_swab.o tif_thunder.o tif_tile.o tif_version.o tif_warning.o tif_write.o tif_zip.o

%o : %c
	$(CC) $(CFLAGS) -c -o $@ $< 

tiff.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) tiff.a

distclean:
	$(RM) $(OBJS) tiff.a
