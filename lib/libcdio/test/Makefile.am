#   $Id: Makefile.am,v 1.45 2005/01/23 00:45:57 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
####################################################
# Things to regression testing
####################################################
#
#
# There's a problem with doing make distcheck for testdefault. 
# A reminder of why I hate automake.

if BUILD_CD_PARANOIA
testparanoia=testparanoia
testparanoia_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)
endif

hack = check_sizeof testassert  testbincue testischar testiso9660 \
       $(testparanoia) testtoc

noinst_PROGRAMS = $(hack) testdefault 

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS) $(LIBISO9660_CFLAGS)

check_sizeof_LDADD  = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
testassert_LDADD    = $(LIBCDIO_LIBS)
testdefault_LDADD   = $(LIBCDIO_LIBS)
testischar_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
testiso9660_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

testtoc_LDADD       = $(LIBCDIO_LIBS)
testtoc_CFLAGS      = -DTEST_DIR=\"$(srcdir)\"

testbincue_LDADD    = $(LIBCDIO_LIBS)
testbincue_CFLAGS   = -DTEST_DIR=\"$(srcdir)\"

check_SCRIPTS = check_nrg.sh  check_cue.sh  check_cd_read.sh \
                check_iso.sh  check_paranoia.sh check_opts.sh
# If we beefed this up so it checked to see if a CD-DA was loaded
# it could be an automatic test. But for now, not so.
#               check_paranoia.sh

check_PROGRAMS = $(hack)

check_DATA = vcd_demo.right vcd_demo_vcdinfo.right \
	     videocd.right videocd.nrg \
	     cdda.right cdda.toc cdda.cue cdda.bin \
	     isofs-m1.right isofs-m1.toc isofs-m1.cue isofs-m1.bin \
	     check_opts0.right check_opts1.right check_opts2.right \
             check_opts3.right check_opts4.right check_opts5.right \
             check_opts6.right check_opts7.right \
	     isofs-m1-read.right cdda-read.right \
	     t1.toc t2.toc t3.toc t4.toc t5.toc t6.toc t7.toc t8.toc t9.toc \
	     data1.toc data2.toc data5.toc data6.toc data7.toc \
	     vcd2.toc cdtext.toc \
	     bad-mode1.cue bad-msf-1.cue bad-msf-2.cue bad-msf-3.cue \
	     bad-mode1.toc bad-msf-1.toc bad-msf-2.toc bad-msf-3.toc \
	     bad-cat1.cue bad-cat2.cue bad-cat3.cue \
	     bad-cat1.toc bad-cat2.toc bad-cat3.toc bad-file.toc \
	     copying.iso copying.right \
	     joliet.iso joliet.right joliet-nojoliet.right

EXTRA_DIST = $(check_SCRIPTS) $(check_DATA) \
	check_common_fn check_cue.sh.in check_nrg.sh.in check_iso.sh.in

TESTS = $(check_PROGRAMS) $(check_SCRIPTS) 
XFAIL_TESTS = testassert

MOSTLYCLEANFILES = core core.* *.dump cdda-orig.wav cdda-try.wav *.raw

test: check

# This is a really bad hack to make sure check_nrg and check_cue.sh
# are executable. Automake will remake check_nrg.sh and check_cue.sh
# but not run the configure default commands for them to make sure
# they are executable. You know it would be nice one could just set
# permissions and mode when it makes the files. I'm sure there's some
# cleaner a way to do this, but frankly I've wasted far too much of my
# life the crappy automess system that I've really lost interest in
# learning any more of this awful system than I need to.
check-am:	make-executable
make-executable: check_nrg.sh check_cue.sh check_paranoia.sh
	chmod +x *.sh
	if test ! -f cdda.bin ; then $(LN_S) $(srcdir)/cdda.bin cdda.bin ; fi
	if test ! -f isofs-m1.bin ; then $(LN_S) $(srcdir)/isofs-m1.bin isofs-m1.bin ; fi
