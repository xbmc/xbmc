# @(#) $Id: Makefile.am,v 1.7 2003/12/22 15:32:37 yeti Exp $

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = $(extconv_SCRIPTS)

extconvdir = $(libexecdir)/enca/extconv

extconv_SCRIPTS = \
  cstocs \
  map \
  piconv \
  recode \
  umap

EXTRA_DIST = \
  b-recode \
  b-cstocs.in \
  b-map.in \
  b-piconv.in \
  b-umap.in

%: b-%
	cat $< >$@

clean-local:
	rm -f $(extconv_SCRIPTS) *~ core.*
