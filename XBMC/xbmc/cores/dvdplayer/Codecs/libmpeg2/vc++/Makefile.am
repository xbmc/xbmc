DISTCLEANFILES = cpu_accel.obj rgb_mmx.obj cpu_state.obj \
		 idct_mmx.obj motion_comp_mmx.obj

EXTRA_DIST = config.h inttypes.h libmpeg2.dsp libmpeg2convert.dsp \
	     libvo.dsp mpeg2dec.dsp mpeg2dec.dsw $(DISTCLEANFILES)

WIN_GCC = i586-mingw32msvc-gcc \
	  -I$(top_srcdir)/include -I$(top_builddir)/include \
	  -Wall -Werror -O3 -fomit-frame-pointer -mcpu=pentiumpro

cpu_accel.obj: FORCE
	$(WIN_GCC) -c $(top_srcdir)/libmpeg2/cpu_accel.c -o cpu_accel.obj

cpu_state.obj: FORCE
	$(WIN_GCC) -c $(top_srcdir)/libmpeg2/cpu_state.c -o cpu_state.obj

idct_mmx.obj: FORCE
	$(WIN_GCC) -c $(top_srcdir)/libmpeg2/idct_mmx.c -o idct_mmx.obj

motion_comp_mmx.obj: FORCE
	$(WIN_GCC) -c $(top_srcdir)/libmpeg2/motion_comp_mmx.c -o motion_comp_mmx.obj

rgb_mmx.obj: FORCE
	$(WIN_GCC) -c $(top_srcdir)/libmpeg2/convert/rgb_mmx.c -o rgb_mmx.obj

FORCE:
