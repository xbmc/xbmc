CFLAGS=-fPIC
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common
endif
INCLUDES+=-Iinclude
LIB=jasper.a

OBJS=bmp/bmp_cod.o bmp/bmp_dec.o bmp/bmp_enc.o base/jas_cm.o base/jas_debug.o base/jas_getopt.o base/jas_icc.o base/jas_iccdata.o base/jas_image.o base/jas_init.o base/jas_malloc.o base/jas_seq.o base/jas_stream.o base/jas_string.o base/jas_tvp.o base/jas_version.o jp2/jp2_cod.o jp2/jp2_dec.o jp2/jp2_enc.o jpc/jpc_bs.o jpc/jpc_cs.o jpc/jpc_dec.o jpc/jpc_enc.o jpc/jpc_math.o jpc/jpc_mct.o jpc/jpc_mqcod.o jpc/jpc_mqdec.o jpc/jpc_mqenc.o jpc/jpc_qmfb.o jpc/jpc_t1cod.o jpc/jpc_t1dec.o jpc/jpc_t1enc.o jpc/jpc_t2cod.o jpc/jpc_t2dec.o jpc/jpc_t2enc.o jpc/jpc_tagtree.o jpc/jpc_tsfb.o jpc/jpc_util.o jpg/jpg_dummy.o jpg/jpg_val.o mif/mif_cod.o pgx/pgx_cod.o pgx/pgx_dec.o pgx/pgx_enc.o pnm/pnm_cod.o pnm/pnm_dec.o pnm/pnm_enc.o ras/ras_cod.o ras/ras_dec.o ras/ras_enc.o

include ../../../Makefile.include

