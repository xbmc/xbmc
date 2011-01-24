#!/bin/sh
# Simple check of transliteration facilities.
# Usage: check-translit SRCDIR FILE FROMCODE TOCODE
srcdir="$1"
file="$2"
fromcode="$3"
tocode="$4"
set -e
../src/iconv_no_i18n -f "$fromcode" -t "$tocode"//TRANSLIT < "${srcdir}"/"$file"."$fromcode" > tmp
cmp "${srcdir}"/"$file"."$tocode" tmp
rm -f tmp
exit 0
