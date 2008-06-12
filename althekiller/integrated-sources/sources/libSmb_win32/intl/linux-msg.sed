# po2msg.sed - Convert Uniforum style .po file to Linux style .msg file
# Copyright (C) 1995 Free Software Foundation, Inc.
# Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#
# The first directive in the .msg should be the definition of the
# message set number.  We use always set number 1.
#
1 {
  i\
$set 1 # Automatically created by po2msg.sed
  h
  s/.*/0/
  x
}
#
# Mitch's old catalog format does not allow comments.
#
# We copy the original message as a comment into the .msg file.
#
/^msgid/ {
  s/msgid[ 	]*"//
#
# This does not work now with the new format.
# /"$/! {
#   s/\\$//
#   s/$/ ... (more lines following)"/
# }
  x
# The following nice solution is by
# Bruno <Haible@ma2s2.mathematik.uni-karlsruhe.de>
  td
# Increment a decimal number in pattern space.
# First hide trailing `9' digits.
  :d
  s/9\(_*\)$/_\1/
  td
# Assure at least one digit is available.
  s/^\(_*\)$/0\1/
# Increment the last digit.
  s/8\(_*\)$/9\1/
  s/7\(_*\)$/8\1/
  s/6\(_*\)$/7\1/
  s/5\(_*\)$/6\1/
  s/4\(_*\)$/5\1/
  s/3\(_*\)$/4\1/
  s/2\(_*\)$/3\1/
  s/1\(_*\)$/2\1/
  s/0\(_*\)$/1\1/
# Convert the hidden `9' digits to `0's.
  s/_/0/g
  x
  G
  s/\(.*\)"\n\([0-9]*\)/$ #\2 Original Message:(\1)/p
}
#
# The .msg file contains, other then the .po file, only the translations
# but each given a unique ID.  Starting from 1 and incrementing by 1 for
# each message we assign them to the messages.
# It is important that the .po file used to generate the cat-id-tbl.c file
# (with po-to-tbl) is the same as the one used here.  (At least the order
# of declarations must not be changed.)
#
/^msgstr/ {
  s/msgstr[ 	]*"\(.*\)"/# \1/
# Clear substitution flag.
  tb
# Append the next line.
  :b
  N
# Look whether second part is continuation line.
  s/\(.*\n\)"\(.*\)"/\1\2/
# Yes, then branch.
  ta
  P
  D
# Note that D includes a jump to the start!!
# We found a continuation line.  But before printing insert '\'.
  :a
  s/\(.*\)\(\n.*\)/\1\\\2/
  P
# We cannot use D here.
  s/.*\n\(.*\)/\1/
  tb
}
d
