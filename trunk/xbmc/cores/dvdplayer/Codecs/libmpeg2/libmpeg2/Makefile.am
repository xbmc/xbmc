SUBDIRS = convert

AM_CFLAGS = $(OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

lib_LTLIBRARIES = libmpeg2.la
libmpeg2_la_SOURCES = alloc.c header.c decode.c slice.c motion_comp.c idct.c
libmpeg2_la_LIBADD = libmpeg2arch.la
libmpeg2_la_LDFLAGS = -no-undefined -version-info 1:0:1

noinst_LTLIBRARIES = libmpeg2arch.la
libmpeg2arch_la_SOURCES = motion_comp_mmx.c idct_mmx.c \
			  motion_comp_altivec.c idct_altivec.c \
			  motion_comp_alpha.c idct_alpha.c \
			  motion_comp_vis.c motion_comp_arm.c \
			  cpu_accel.c cpu_state.c
if ARCH_ARM
libmpeg2arch_la_SOURCES += motion_comp_arm_s.S
endif
libmpeg2arch_la_CFLAGS = $(OPT_CFLAGS) $(ARCH_OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpeg2.pc

EXTRA_DIST = vlc.h mpeg2_internal.h
