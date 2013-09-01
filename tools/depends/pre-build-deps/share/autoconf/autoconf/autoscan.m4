# This file is part of Autoconf.                       -*- Autoconf -*-
# Interface with autoscan.

# Copyright (C) 2002, 2009, 2010 Free Software Foundation, Inc.

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

# Written by Akim Demaille.

# The prefix `AN' is chosen after `AutoscaN'.

# AN_OUTPUT(KIND, WORD, MACROS)
# -----------------------------
# Declare that the WORD, used as a KIND, requires triggering the MACROS.
m4_define([AN_OUTPUT], [])


# AN_FUNCTION(NAME, MACROS)
# AN_HEADER(NAME, MACROS)
# AN_IDENTIFIER(NAME, MACROS)
# AN_LIBRARY(NAME, MACROS)
# AN_MAKEVAR(NAME, MACROS)
# AN_PROGRAM(NAME, MACROS)
# ---------------------------
# If the FUNCTION/HEADER etc. is used in the package, then the MACROS
# should be invoked from configure.ac.
m4_define([AN_FUNCTION],   [AN_OUTPUT([function], $@)])
m4_define([AN_HEADER],     [AN_OUTPUT([header], $@)])
m4_define([AN_IDENTIFIER], [AN_OUTPUT([identifier], $@)])
m4_define([AN_LIBRARY],    [AN_OUTPUT([library], $@)])
m4_define([AN_MAKEVAR],    [AN_OUTPUT([makevar], $@)])
m4_define([AN_PROGRAM],    [AN_OUTPUT([program], $@)])
