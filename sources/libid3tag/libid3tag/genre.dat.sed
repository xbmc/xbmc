#
# libid3tag - ID3 tag manipulation library
# Copyright (C) 2000-2004 Underbit Technologies, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: genre.dat.sed,v 1.10 2004/01/23 09:41:32 rob Exp $
#

1i\
/* Automatically generated from genre.dat.in */

# generate an array from a string
/^[A-Za-z]/{
H
y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/
s/[^A-Z0-9]/_/g
s/.*/static id3_ucs4_t const genre_&[] =/p
g
s/.*\n//
s/./'&', /g
s/.*/  { &0 };/
}

# write the final table of arrays
${
p
i\
\
static id3_ucs4_t const *const genre_table[] = {
g
s/^\(\n\)\(.*\)$/\2\1/
y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/
s/[^A-Z0-9\n]/_/g
s/\([^\n]*\)\(\n\)/  genre_\1,\2/g
s/,\n$//
a\
};
}

# print the pattern space (assumes -n)
p
