top_builddir=.
-include $(top_builddir)/Make.conf

SUBDIRS = src demo docs

AUTOGEN = Make.conf config.log config.status libtool autom4te.cache ftgl.pc
AUTOGEN_MAINT = aclocal.m4 acinclude.m4 config.guess config.sub configure ltmain.sh install-sh

all: Make.conf
all: all-recursive

Make.conf:
	./configure

ftgl.pc:
	./configure

install-local: ftgl.pc
	$(INSTALL) -d -m 0755 $(libdir)/pkgconfig
	$(INSTALL) -m 0644 ftgl.pc $(libdir)/pkgconfig/

maint-clean-local:
	$(RM) $(AUTOGEN_MAINT)

distclean-local:
	$(RM) -r $(AUTOGEN)

# Extremely non-GNU unfriendly dist rule.
dist: distclean
	( set -e ; \
	  $(RM) -r ../ftgl-dist-tmp ; \
	  mkdir ../ftgl-dist-tmp ; \
	  version=`grep AC_INIT configure.ac | tr -d [:blank:] | cut -d , -f 2` ; \
	  distdir=../ftgl-dist-tmp/ftgl-$$version ; \
	  mkdir $$distdir ; \
	  cp -a . $$distdir ; \
	  cd $$distdir ; \
	  ./bootstrap ; \
	  find -type d -name CVS | xargs -r rm -rf ; \
	  rm bootstrap ; \
	  find -type f | xargs chmod a-x ; \
	  chmod a+x config.guess config.sub configure install-sh ; \
	  tar -C .. -cvzf ../../ftgl-$$version.tar.gz ftgl-$$version )

include Make.rules

.PHONY: all clean clean-local maint-clean distclean distclean-local
