CC=gcc

OBJS=bits.o cfft.o common.o decoder.o drc.o error.o filtbank.o hcr.o huffman.o ic_predict.o is.o lt_predict.o mdct.o mp4.o ms.o output.o pns.o pulse.o rvlc.o sbr_dct.o sbr_dec.o sbr_e_nf.o sbr_fbt.o sbr_hfadj.o sbr_hfgen.o sbr_huff.o sbr_qmf.o sbr_syntax.o sbr_tf_grid.o specrec.o ssr.o syntax.o tns.o bits.o cfft.o common.o decoder.o drc.o error.o filtbank.o hcr.o huffman.o ic_predict.o is.o lt_predict.o mdct.o mp4.o ms.o output.o pns.o pulse.o rvlc.o sbr_dct.o sbr_dec.o sbr_e_nf.o sbr_fbt.o sbr_hfadj.o sbr_hfgen.o sbr_huff.o sbr_qmf.o sbr_syntax.o sbr_tf_grid.o specrec.o ssr.o ssr_fb.o ssr_ipqf.o syntax.o tns.o

CFLAGS=-fPIC -DLINUX -DHAS_SYS_TYPES -DHAVE_SYS_STAT -DHAVE_STDLIB_H -DHAVE_STRING_H -DHAVE_MEMORY_H -DHAVE_STRINGS_H -DHAVE_STDINT_H -DHAVE_STRCHR -DHAVE_MEMCPY -I.
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common
endif

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

libfaad: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
