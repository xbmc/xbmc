# Freeze M4 files.

# Copyright (C) 2002, 2004, 2006, 2007, 2008, 2009, 2010 Free Software
# Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


## ----------------- ##
## Freeze M4 files.  ##
## ----------------- ##

SUFFIXES = .m4 .m4f

AUTOM4TE_CFG = $(top_builddir)/lib/autom4te.cfg
$(AUTOM4TE_CFG): $(top_srcdir)/lib/autom4te.in
	cd $(top_builddir)/lib && $(MAKE) $(AM_MAKEFLAGS) autom4te.cfg

# Do not use AUTOM4TE here, since maint.mk (my-distcheck)
# checks if we are independent of Autoconf by defining AUTOM4TE (and
# others) to `false'.  Autoconf provides autom4te, so that doesn't
# apply to us.
MY_AUTOM4TE =									\
	autom4te_perllibdir='$(top_srcdir)'/lib					\
	AUTOM4TE_CFG='$(AUTOM4TE_CFG)'         $(top_builddir)/bin/autom4te	\
		-B '$(top_builddir)'/lib -B '$(top_srcdir)'/lib        # keep ` '

# When processing the file with diversion disabled, there must be no
# output but comments and empty lines.
# If freezing produces output, something went wrong: a bad `divert',
# or an improper paren etc.
# It may happen that the output does not end with an end of line, hence
# force an end of line when reporting errors.
.m4.m4f:
	$(MY_AUTOM4TE)				\
		--language=$*			\
		--freeze			\
		--output=$@

# Factor the dependencies between all the frozen files.
# Some day we should explain to Automake how to use autom4te to compute
# the dependencies...
src_libdir   = $(top_srcdir)/lib
build_libdir = $(top_builddir)/lib

m4f_dependencies = $(top_builddir)/bin/autom4te $(AUTOM4TE_CFG)

# For parallel builds.
$(build_libdir)/m4sugar/version.m4:
	cd $(build_libdir)/m4sugar && $(MAKE) $(AM_MAKEFLAGS) version.m4

m4sugar_m4f_dependencies =			\
	$(m4f_dependencies)			\
	$(src_libdir)/m4sugar/m4sugar.m4	\
	$(build_libdir)/m4sugar/version.m4

m4sh_m4f_dependencies =				\
	$(m4sugar_m4f_dependencies)		\
	$(src_libdir)/m4sugar/m4sh.m4

autotest_m4f_dependencies =			\
	$(m4sh_m4f_dependencies)		\
	$(src_libdir)/autotest/autotest.m4	\
	$(src_libdir)/autotest/general.m4	\
	$(src_libdir)/autotest/specific.m4

autoconf_m4f_dependencies =			\
	$(m4sh_m4f_dependencies)		\
	$(src_libdir)/autoconf/autoscan.m4	\
	$(src_libdir)/autoconf/general.m4	\
	$(src_libdir)/autoconf/autoheader.m4	\
	$(src_libdir)/autoconf/autoupdate.m4	\
	$(src_libdir)/autoconf/autotest.m4	\
	$(src_libdir)/autoconf/status.m4	\
	$(src_libdir)/autoconf/oldnames.m4	\
	$(src_libdir)/autoconf/specific.m4	\
	$(src_libdir)/autoconf/lang.m4		\
	$(src_libdir)/autoconf/c.m4		\
	$(src_libdir)/autoconf/fortran.m4	\
	$(src_libdir)/autoconf/erlang.m4	\
	$(src_libdir)/autoconf/functions.m4	\
	$(src_libdir)/autoconf/headers.m4	\
	$(src_libdir)/autoconf/types.m4		\
	$(src_libdir)/autoconf/libs.m4		\
	$(src_libdir)/autoconf/programs.m4	\
	$(src_libdir)/autoconf/autoconf.m4


## --------------------------- ##
## Run ETAGS on some M4 code.  ##
## --------------------------- ##

ETAGS_FOR_M4 = \
  --lang=none \
  --regex='/\(m4_define\|define\)(\[\([^]]*\)\]/\2/'

ETAGS_FOR_M4SUGAR = \
  $(ETAGS_FOR_M4) \
  --regex='/m4_defun(\[\([^]]*\)\]/\1/'

ETAGS_FOR_AUTOCONF = \
  $(ETAGS_FOR_M4SUGAR) \
  --regex='/\(A[CU]_DEFUN\|AU_ALIAS\)(\[\([^]]*\)\]/\2/' \
  --regex='/AN_\(FUNCTION\|HEADER\|IDENTIFIER\|LIBRARY\|MAKEVAR\|PROGRAM\)(\[\([^]]*\)\]/\2/'


## -------------------------------- ##
## Looking for forbidden patterns.  ##
## -------------------------------- ##

check-forbidden-patterns:
	@if (cd $(srcdir) && \
	    $(GREP) $(forbidden_patterns) $(forbidden_patterns_files)) \
	    >forbidden.log; then \
	  echo "ERROR: forbidden patterns were found:" >&2; \
	  sed "s|^|$*.m4: |" <forbidden.log >&2; \
	  echo >&2; \
	  exit 1; \
	else \
	  rm -f forbidden.log; \
	fi
