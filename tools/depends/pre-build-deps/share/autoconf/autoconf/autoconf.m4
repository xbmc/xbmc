# This file is part of Autoconf.                -*- Autoconf -*-
# Driver that loads the Autoconf macro files.
#
# Copyright (C) 1994, 1999, 2000, 2001, 2002, 2006, 2008, 2009, 2010
# Free Software Foundation, Inc.

# This file is part of Autoconf.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.

# Written by David MacKenzie and many others.
#
# Do not sinclude acsite.m4 here, because it may not be installed
# yet when Autoconf is frozen.
# Do not sinclude ./aclocal.m4 here, to prevent it from being frozen.

# general includes some AU_DEFUN.
m4_include([autoconf/autoupdate.m4])
m4_include([autoconf/autoscan.m4])
m4_include([autoconf/general.m4])
m4_include([autoconf/status.m4])
m4_include([autoconf/autoheader.m4])
m4_include([autoconf/autotest.m4])
m4_include([autoconf/programs.m4])
m4_include([autoconf/lang.m4])
m4_include([autoconf/c.m4])
m4_include([autoconf/erlang.m4])
m4_include([autoconf/fortran.m4])
m4_include([autoconf/functions.m4])
m4_include([autoconf/headers.m4])
m4_include([autoconf/types.m4])
m4_include([autoconf/libs.m4])
m4_include([autoconf/specific.m4])
m4_include([autoconf/oldnames.m4])

# We discourage the use of the non prefixed macro names: M4sugar maps
# all the builtins into `m4_'.  Autoconf has been converted to these
# names too.  But users may still depend upon these, so reestablish
# them.

# In order to copy pushdef stacks, m4_copy temporarily destroys the
# current pushdef stack.  But these builtins are so primitive that:
#   1. they should not have more than one pushdef definition
#   2. undefining the pushdef stack to copy breaks m4_copy
# Hence, we temporarily restore a simpler m4_copy.

m4_pushdef([m4_copy], [m4_define([$2], m4_defn([$1]))])

m4_copy_unm4([m4_builtin])
m4_copy_unm4([m4_changequote])
m4_copy_unm4([m4_decr])
m4_copy_unm4([m4_define])
m4_copy_unm4([m4_defn])
m4_copy_unm4([m4_divert])
m4_copy_unm4([m4_divnum])
m4_copy_unm4([m4_errprint])
m4_copy_unm4([m4_esyscmd])
m4_copy_unm4([m4_ifdef])
m4_copy([m4_if], [ifelse])
m4_copy_unm4([m4_incr])
m4_copy_unm4([m4_index])
m4_copy_unm4([m4_indir])
m4_copy_unm4([m4_len])
m4_copy([m4_bpatsubst], [patsubst])
m4_copy_unm4([m4_popdef])
m4_copy_unm4([m4_pushdef])
m4_copy([m4_bregexp], [regexp])
m4_copy_unm4([m4_sinclude])
m4_copy_unm4([m4_syscmd])
m4_copy_unm4([m4_sysval])
m4_copy_unm4([m4_traceoff])
m4_copy_unm4([m4_traceon])
m4_copy_unm4([m4_translit])
m4_copy_unm4([m4_undefine])
m4_copy_unm4([m4_undivert])

m4_popdef([m4_copy])

# Yet some people have started to use m4_patsubst and m4_regexp.
m4_define([m4_patsubst],
[m4_expand_once([m4_warn([syntax],
		 [do not use m4_patsubst: use patsubst or m4_bpatsubst])])dnl
patsubst($@)])

m4_define([m4_regexp],
[m4_expand_once([m4_warn([syntax],
		 [do not use m4_regexp: use regexp or m4_bregexp])])dnl
regexp($@)])
