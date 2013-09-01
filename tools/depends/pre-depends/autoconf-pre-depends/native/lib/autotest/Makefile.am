# Make Autotest library.

# Copyright (C) 2001, 2002, 2009, 2010 Free Software Foundation, Inc.

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

autotestlibdir = $(pkgdatadir)/autotest
dist_autotestlib_DATA = autotest.m4 general.m4 specific.m4
nodist_autotestlib_DATA = autotest.m4f
CLEANFILES = $(nodist_autotestlib_DATA)

## --------------- ##
## Building TAGS.  ##
## --------------- ##

TAGS_FILES = $(dist_autotestlib_DATA)

ETAGS_ARGS = $(ETAGS_FOR_AUTOCONF)


## -------- ##
## Checks.  ##
## -------- ##

check-local: check-forbidden-patterns
forbidden_patterns = -e '^_*EOF' -e ' cmp '
forbidden_patterns_files = $(dist_autotestlib_DATA)


## ------------------ ##
## The frozen files.  ##
## ------------------ ##

autotest.m4f: $(autotest_m4f_dependencies)
include ../freeze.mk
