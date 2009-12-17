#!/bin/sh
# Complete check of a stateless encoding.
# Usage: check-stateless SRCDIR CHARSET
srcdir="$1"
charset="$2"
set -e

# charset, modified for use in filenames.
charsetf=`echo "$charset" | sed -e 's,:,-,g'`

# iconv in one direction.
./table-from "$charset" > tmp-"$charsetf".TXT

# iconv in the other direction.
./table-to "$charset" | sort > tmp-"$charsetf".INVERSE.TXT

# Check 1: charmap and iconv forward should be identical.
cmp "${srcdir}"/"$charsetf".TXT tmp-"$charsetf".TXT 2> /dev/null

# Check 2: the difference between the charmap and iconv backward.
sed -e '/	.* 0x/d' < "${srcdir}"/"$charsetf".TXT > tmp-noprecomposed-"$charsetf".TXT
if test -f "${srcdir}"/"$charsetf".IRREVERSIBLE.TXT; then
  cat tmp-noprecomposed-"$charsetf".TXT "${srcdir}"/"$charsetf".IRREVERSIBLE.TXT | sort | uniq -u > tmp-orig-"$charsetf".INVERSE.TXT
else
  cp tmp-noprecomposed-"$charsetf".TXT tmp-orig-"$charsetf".INVERSE.TXT
fi
cmp tmp-orig-"$charsetf".INVERSE.TXT tmp-"$charsetf".INVERSE.TXT 2> /dev/null

rm -f tmp-"$charsetf".TXT tmp-"$charsetf".INVERSE.TXT tmp-noprecomposed-"$charsetf".TXT tmp-orig-"$charsetf".INVERSE.TXT
exit 0
# For a new encoding:
# You can create the "$charsetf".TXT like this:
#   ./table-from "$charset" > "$charsetf".TXT
# You can create the "$charsetf".IRREVERSIBLE.TXT like this:
#   ./table-to "$charset" | sort > "$charsetf".INVERSE.TXT
#   diff "$charsetf".TXT "$charsetf".INVERSE.TXT | grep '^[<>]' | sed -e 's,^. ,,' > "$charsetf".IRREVERSIBLE.TXT
