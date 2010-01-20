#!/bin/sh

# Prints a table of the charsets (system dependent name, portable name, and
# X11 name) for all locales.

${CC-cc} -o locale_codeset locale_codeset.c
${CC-cc} -o locale_charset -I../include locale_charset.c \
    -DHAVE_CONFIG_H -I.. -DLIBDIR='"'`cd ../lib && pwd`'"' \
    ../lib/localcharset.c
${CC-cc} -o locale_x11encoding locale_x11encoding.c \
    -I/usr/X11R6/include \
    -L/usr/X11R6/lib -lX11
#${CC-cc} -o locale_x11encoding locale_x11encoding.c \
#    -I/packages/gnu/XFree86/include \
#    -L/packages/gnu/XFree86/lib -lX11 \
#    -Wl,-rpath,/packages/gnu/XFree86/lib

printf '%-15s%-17s%-17s %-17s %-17s\n\n' \
       "locale name" "locale charmap" "nl_langinfo(CODESET)" "locale_charset()" "X11 encoding"
for lc in `./all-locales | sort | uniq`
do
  charmap=`LC_ALL=$lc ./locale_charmap 2>/dev/null || echo '<error>'`
  codeset=`LC_ALL=$lc ./locale_codeset 2>/dev/null || echo '<error>'`
  charset=`LC_ALL=$lc ./locale_charset 2>/dev/null || echo '<error>'`
  x11encoding=`LC_ALL=$lc ./locale_x11encoding 2>/dev/null || echo '<error>'`
  printf '%-15s  %-17s %-17s %-17s %-17s\n' \
         "$lc" "$charmap" "$codeset" "$charset" "$x11encoding"
done
