# Make Autoconf library for M4sugar.

# Copyright (C) 2001, 2002, 2006, 2007, 2008, 2009, 2010 Free Software
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

m4sugarlibdir = $(pkgdatadir)/m4sugar
dist_m4sugarlib_DATA = m4sugar.m4 foreach.m4 m4sh.m4
nodist_m4sugarlib_DATA = version.m4 m4sugar.m4f m4sh.m4f
CLEANFILES = $(nodist_m4sugarlib_DATA)

# Get the release year from ../ChangeLog.
RELEASE_YEAR = \
  `sed 's/^\([0-9][0-9][0-9][0-9]\).*/\1/;q' $(top_srcdir)/ChangeLog`

## ------------ ##
## version.m4.  ##
## ------------ ##

# The `:;' works around a redirected compound command bash exit status bug.
version.m4: Makefile
	:;{ \
	  echo '# This file is part of -*- Autoconf -*-.' && \
	  echo '# Version of Autoconf.' && \
	  echo '# Copyright (C) 1999, 2000, 2001, 2002, 2006, 2007, 2009' && \
	  echo '# Free Software Foundation, Inc.' && \
	  echo  &&\
	  echo 'm4_define([m4_PACKAGE_NAME],      [$(PACKAGE_NAME)])' && \
	  echo 'm4_define([m4_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])' && \
	  echo 'm4_define([m4_PACKAGE_VERSION],   [$(PACKAGE_VERSION)])' && \
	  echo 'm4_define([m4_PACKAGE_STRING],    [$(PACKAGE_STRING)])' && \
	  echo 'm4_define([m4_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])' && \
	  echo 'm4_define([m4_PACKAGE_URL],       [$(PACKAGE_URL)])' && \
	  echo 'm4_define([m4_PACKAGE_YEAR],      ['$(RELEASE_YEAR)'])'; \
	} > $@-t
	mv $@-t $@


## --------------- ##
## Building TAGS.  ##
## --------------- ##

TAGS_FILES = $(dist_m4sugarlib_DATA)

ETAGS_ARGS = $(ETAGS_FOR_AUTOCONF)


## -------- ##
## Checks.  ##
## -------- ##

check-local: check-forbidden-patterns
forbidden_patterns = -e '^_*EOF' -e ' cmp '
forbidden_patterns_files = $(dist_m4sugarlib_DATA)



## ------------------ ##
## The frozen files.  ##
## ------------------ ##

m4sugar.m4f: $(m4sugar_m4f_dependencies)
m4sh.m4f: $(m4sh_m4f_dependencies)
include ../freeze.mk
