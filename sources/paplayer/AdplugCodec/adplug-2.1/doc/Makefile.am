info_TEXINFOS = libadplug.texi

libadplug_TEXINFOS = fdl.texi

man_MANS = adplugdb.1

EXTRA_DIST = adplugdb.1.in

MOSTLYCLEANFILES = stamp-vti libadplug.info libadplug.info-1 \
	libadplug.info-2

CLEANFILES = libadplug.cps libadplug.fns libadplug.vrs version.texi

DISTCLEANFILES = adplugdb.1

edit = sed \
	-e 's,@sharedstatedir\@,$(sharedstatedir),g' \
	-e 's,@VERSION\@,$(VERSION),g'

adplugdb.1: Makefile $(srcdir)/adplugdb.1.in
	rm -f adplugdb.1 adplugdb.1.tmp
	$(edit) $(srcdir)/adplugdb.1.in >adplugdb.1.tmp
	mv adplugdb.1.tmp adplugdb.1
