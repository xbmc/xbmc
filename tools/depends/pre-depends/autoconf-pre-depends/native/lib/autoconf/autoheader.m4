# This file is part of Autoconf.                       -*- Autoconf -*-
# Interface with autoheader.

# Copyright (C) 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001,
# 2002, 2008, 2009, 2010 Free Software Foundation, Inc.

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

# Written by David MacKenzie, with help from
# Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
# Roland McGrath, Noah Friedman, david d zuhn, and many others.


# AH_OUTPUT(KEY, TEXT)
# --------------------
# Pass TEXT to autoheader.
# This macro is `read' only via `autoconf --trace', it outputs nothing.
m4_define([AH_OUTPUT], [])


# AH_VERBATIM(KEY, TEMPLATE)
# --------------------------
# If KEY is direct (i.e., no indirection such as in KEY=$my_func which
# may occur if there is AC_CHECK_FUNCS($my_func)), issue an autoheader
# TEMPLATE associated to the KEY.  Otherwise, do nothing.  TEMPLATE is
# output as is, with no formatting.
#
# Quote for Perl '' strings, which are those used by Autoheader.
m4_define([AH_VERBATIM],
[AS_LITERAL_WORD_IF([$1],
	       [AH_OUTPUT(_m4_expand([$1]), AS_ESCAPE([[$2]], [\']))])])


# AH_TEMPLATE(KEY, DESCRIPTION)
# -----------------------------
# Issue an autoheader template for KEY, i.e., a comment composed of
# DESCRIPTION (properly wrapped), and then #undef KEY.
m4_define([AH_TEMPLATE],
[AH_VERBATIM([$1],
	     m4_text_wrap([$2 */], [   ], [/* ])[
@%:@undef ]_m4_expand([$1]))])


# AH_TOP(TEXT)
# ------------
# Output TEXT at the top of `config.h.in'.
m4_define([AH_TOP],
[m4_define([_AH_COUNTER], m4_incr(_AH_COUNTER))dnl
AH_VERBATIM([0000]_AH_COUNTER, [$1])])


# AH_BOTTOM(TEXT)
# ---------------
# Output TEXT at the bottom of `config.h.in'.
m4_define([AH_BOTTOM],
[m4_define([_AH_COUNTER], m4_incr(_AH_COUNTER))dnl
AH_VERBATIM([zzzz]_AH_COUNTER, [$1])])

# Initialize.
m4_define([_AH_COUNTER], [0])
