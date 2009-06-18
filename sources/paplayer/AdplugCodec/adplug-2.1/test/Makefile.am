check_PROGRAMS = playertest emutest

playertest_SOURCES = playertest.cpp

emutest_SOURCES = emutest.cpp

AM_LDFLAGS = $(top_builddir)/src/.libs/libadplug.la $(libbinio_LIBS)

AM_CPPFLAGS = $(libbinio_CFLAGS)

TESTS = playertest emutest

EXTRA_DIST = 2001.MKJ 2001.ref ADAGIO.DFM ADAGIO.ref adlibsp.ref adlibsp.s3m \
	ALLOYRUN.RAD ALLOYRUN.ref ARAB.BAM ARAB.ref BEGIN.KSM BEGIN.ref \
	bmf1_2.ref bmf1_2.xad BOOTUP.M BOOTUP.ref CHILD1.ref CHILD1.XSM \
	DTM-TRK1.DTM DTM-TRK1.ref fdance03.dmo fdance03.ref flash.ref \
	flash.xad HIP_D.ref HIP_D.ROL hybrid.ref hybrid.xad hyp.ref hyp.xad \
	icepatch.003 ice_thnk.ref ice_thnk.sci inc.raw inc.ref insts.dat \
	loudness.lds loudness.ref MARIO.A2M MARIO.ref mi2.laa mi2.ref \
	michaeld.cmf michaeld.ref PLAYMUS1.ref PLAYMUS1.SNG psi1.ref psi1.xad \
	rat.ref rat.xad REVELAT.ref REVELAT.SNG SAILOR.CFF SAILOR.ref \
	samurai.dro samurai.ref SATNIGHT.HSP SATNIGHT.ref SCALES.ref SCALES.SA2 \
	SMKEREM.HSC SMKEREM.ref SONG1.ins SONG1.ref SONG1.sng standard.bnk \
	TOCCATA.MAD TOCCATA.ref TUBES.ref TUBES.SAT TU_BLESS.AMD TU_BLESS.ref \
	VIB_VOL3.D00 VIB_VOL3.ref WONDERIN.WLF WONDERIN.ref blaster2.msc \
	blaster2.ref RI051.RIX RI051.ref
