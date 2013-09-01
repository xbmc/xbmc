## Make Autoconf tests.

# Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
# 2009, 2010 Free Software Foundation, Inc.

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

# We don't actually distribute the built testsuite or package.m4, since one
# only needs m4 to build them, and m4 is required to install Autoconf.
# But if you are borrowing from this file for setting up autotest in your
# project, remember to distribute both testsuite and package.m4.
EXTRA_DIST = $(TESTSUITE_AT) local.at mktests.sh \
	     atlocal.in wrapper.as statesave.m4

# Running the uninstalled scripts.  Build them upon `all', for the manpages.
noinst_SCRIPTS = $(wrappers)
DISTCLEANFILES = atconfig atlocal $(TESTSUITE)
MAINTAINERCLEANFILES = Makefile.in

# Import the dependencies on Autotest and M4sh.
include ../lib/freeze.mk


## ------------ ##
## package.m4.  ##
## ------------ ##

# The `:;' works around a redirected compound command bash exit status bug.
package.m4: Makefile
	:;{ \
	  echo '# Signature of the current package.' && \
	  echo 'm4_define([AT_PACKAGE_NAME],      [$(PACKAGE_NAME)])' && \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])' && \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [$(PACKAGE_VERSION)])' && \
	  echo 'm4_define([AT_PACKAGE_STRING],    [$(PACKAGE_STRING)])' && \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])' && \
	  echo 'm4_define([AT_PACKAGE_URL],       [$(PACKAGE_URL)])'; \
	} > $@-t
	mv $@-t $@



## ---------- ##
## Wrappers.  ##
## ---------- ##

wrappers = autoconf autoheader autom4te autoreconf autoscan autoupdate ifnames
CLEANFILES = wrapper.in $(wrappers) package.m4

wrapper.in: $(srcdir)/wrapper.as $(m4sh_m4f_dependencies)
	$(MY_AUTOM4TE) --language=M4sh $(srcdir)/wrapper.as -o $@

edit = sed \
	-e 's|@wrap_program[@]|$@|g' \
	-e 's|@abs_top_srcdir[@]|@abs_top_srcdir@|g' \
	-e 's|@abs_top_builddir[@]|@abs_top_builddir@|g' \
	-e "s|@configure_input[@]|Generated from $$input.|g"

$(wrappers): wrapper.in
	rm -f $@ $@.tmp
	input=wrapper.in; \
	$(edit) wrapper.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv -f $@.tmp $@



## ------------ ##
## Test suite.  ##
## ------------ ##

TESTSUITE_GENERATED_AT = \
	$(srcdir)/aclang.at \
	$(srcdir)/acc.at \
	$(srcdir)/acfortran.at \
	$(srcdir)/acgeneral.at \
	$(srcdir)/acstatus.at \
	$(srcdir)/acautoheader.at \
	$(srcdir)/acautoupdate.at \
	$(srcdir)/acspecific.at \
	$(srcdir)/acfunctions.at \
	$(srcdir)/acheaders.at \
	$(srcdir)/actypes.at \
	$(srcdir)/aclibs.at \
	$(srcdir)/acprograms.at

TESTSUITE_HAND_AT = \
	suite.at \
	m4sugar.at m4sh.at autotest.at \
	base.at tools.at torture.at \
	compile.at c.at erlang.at fortran.at \
	semantics.at \
	autoscan.at \
	foreign.at

TESTSUITE_AT = $(TESTSUITE_GENERATED_AT) $(TESTSUITE_HAND_AT)
TESTSUITE = ./testsuite

# Run the non installed autom4te.
# Don't use AUTOM4TE since `make alpha' makes it unavailable although
# we are allowed to use it (since we ship it).
AUTOTEST = $(MY_AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 \
	      local.at \
	      $(TESTSUITE_AT) \
	      $(autotest_m4f_dependencies)
	cd $(top_builddir)/lib/autotest && $(MAKE) $(AM_MAKEFLAGS) autotest.m4f
	$(AUTOTEST) -I . -I $(srcdir) suite.at -o $@.tmp
	mv $@.tmp $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status tests/$@

# Avoid a race condition that would make parallel "distclean" fail.
# The rule in clean-local tests for existence of $(TESTSUITE), and
# if found, attempts to run it.  But the distclean-generic rule may
# be running in parallel, and it removes $(DISTCLEANFILES) which
# includes $(TESTSUITE).  This is the Automake rule, plus our
# dependency, and we silence the warning from 'automake -Wall' by
# hiding the dependency behind a variable.
# TODO - fix this if newer automake accomodates the dependency.
distclean_generic = distclean-generic
$(distclean_generic): clean-local

clean-local:
	test ! -f $(TESTSUITE) || $(SHELL) $(TESTSUITE) --clean
	rm -f *.tmp
	rm -f -r autom4te.cache

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

# Run the test suite on the *installed* tree.
installcheck-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE) AUTOTEST_PATH="$(bindir)" $(TESTSUITEFLAGS)



## ------------------ ##
## Maintainer rules.  ##
## ------------------ ##

MAINTAINERCLEANFILES += $(TESTSUITE_GENERATED_AT)

## Producing the test files.

# The files which contain macros we check for syntax.  Use $(top_srcdir)
# for the benefit of non-GNU make.  Fix the names in the rule below
# where we `cd' to $srcdir.
autoconfdir = $(top_srcdir)/lib/autoconf
AUTOCONF_FILES = $(autoconfdir)/general.m4 \
		 $(autoconfdir)/status.m4 \
		 $(autoconfdir)/autoheader.m4 \
		 $(autoconfdir)/autoupdate.m4 \
		 $(autoconfdir)/specific.m4 \
		 $(autoconfdir)/functions.m4 \
		 $(autoconfdir)/lang.m4 \
		 $(autoconfdir)/c.m4 \
		 $(autoconfdir)/erlang.m4 \
		 $(autoconfdir)/fortran.m4 \
		 $(autoconfdir)/headers.m4 \
		 $(autoconfdir)/libs.m4 \
		 $(autoconfdir)/types.m4 \
		 $(autoconfdir)/programs.m4

$(TESTSUITE_GENERATED_AT): mktests.stamp
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  rm -f mktests.stamp; \
	  $(MAKE) $(AM_MAKEFLAGS) mktests.stamp; \
	fi

mktests.stamp : mktests.sh $(AUTOCONF_FILES)
	@rm -f mktests.tmp
	@touch mktests.tmp
	cd $(srcdir) && ./mktests.sh \
	  `echo " "$(AUTOCONF_FILES) | sed 's, [^ ]*/, ../lib/autoconf/,g'`
	@mv -f mktests.tmp $@

## Distribute the stamp file, since we distribute the generated files.
EXTRA_DIST += mktests.stamp
CLEANFILES += mktests.tmp
MAINTAINERCLEANFILES += mktests.stamp

## maintainer-check ##

# These cannot be run in parallel.
maintainer-check:
	$(MAKE) $(AM_MAKEFLAGS) maintainer-check-posix
	$(MAKE) $(AM_MAKEFLAGS) maintainer-check-c++

# The hairy heredoc is more robust than using echo.
CLEANFILES += expr
expr:
	:;{					\
	  echo '#! $(SHELL)';			\
	  echo 'result=`$(EXPR) "$$@"`';	\
	  echo 'estatus=$$?';			\
	  echo 'cat <<EOF';			\
	  echo '$${result:-0}';			\
	  echo 'EOF';				\
	  echo 'exit $$estatus';		\
	} > $@-t
	chmod +x $@-t
	mv $@-t $@

# Try the test suite with more severe environments.
maintainer-check-posix: expr
	POSIXLY_CORRECT=yes $(MAKE) $(AM_MAKEFLAGS) check
	rm expr

# Try using G++ as a C compiler.
maintainer-check-c++:
	CC=g++ $(MAKE) $(AM_MAKEFLAGS) check
