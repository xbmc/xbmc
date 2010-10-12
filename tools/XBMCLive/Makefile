CLEAN_FILES = setAlsaVolumes

setAlsaVolumes: setAlsaVolumes.c
ifneq ($(findstring osx,$(ARCH)), osx)
	$(CC) $(CFLAGS) $(DEFINES) -g -D_LINUX -D_REENTRANT \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-D__USE_FILE_OFFSET64 $(INCLUDES) $< -o $@ -lasound
endif

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
