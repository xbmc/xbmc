#!/bin/sh
if test $# -ne 2 ; then
	echo "Usage: $0 <charmap file> <CHARSET NAME>"
	exit 1
fi

CHARMAP=$1
CHARSETNAME=$2

echo "/* "
echo " * Conversion table for $CHARSETNAME charset "
echo " * "
echo " * Conversion tables are generated using $CHARMAP table "
echo " * and source/script/gen-8bit-gap.sh script "
echo " * "
echo " * This program is free software; you can redistribute it and/or modify "
echo " * it under the terms of the GNU General Public License as published by "
echo " * the Free Software Foundation; either version 2 of the License, or "
echo " * (at your option) any later version. "
echo " *  "
echo " * This program is distributed in the hope that it will be useful,"
echo " * but WITHOUT ANY WARRANTY; without even the implied warranty of "
echo " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
echo " * GNU General Public License for more details. "
echo " *  "
echo " * You should have received a copy of the GNU General Public License "
echo " * along with this program; if not, write to the Free Software "
echo " * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. "
echo " */"

echo '#include "includes.h"'
echo
echo "static const uint16 to_ucs2[256] = {"
cat "$CHARMAP" | gawk -f ./script/gen-8bit-gap.awk
echo "};"
echo
echo "static const struct charset_gap_table from_idx[] = {"
sed -ne 's/^<U\(....\).*/\1/p' \
    "$CHARMAP" | sort -u | gawk -f ./script/gap.awk
echo "  { 0xffff, 0xffff, 0 }"
echo "};"
echo
echo "static const unsigned char from_ucs2[] = {"
sed -ne 's/^<U\(....\)>[[:space:]]*.x\(..\).*/\1 \2/p' \
    "$CHARMAP" | sort -u | gawk -f ./script/gaptab.awk
echo "};"
echo 
echo "SMB_GENERATE_CHARSET_MODULE_8_BIT_GAP($CHARSETNAME)"
echo
