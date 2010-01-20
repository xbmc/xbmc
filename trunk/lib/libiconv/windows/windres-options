#!/bin/sh
# Usage: windres-options [--escape] PACKAGE_VERSION
# Outputs a set of command-line options for 'windres', containing definitions
# for the preprocessor variables
#   PACKAGE_VERSION_STRING
#   PACKAGE_VERSION_MAJOR
#   PACKAGE_VERSION_MINOR
#   PACKAGE_VERSION_SUBMINOR

escape=
if test "$1" = "--escape"; then
  escape=yes
  shift
fi
version="$1" # something like 2.0 or 2.17 or 2.17.3 or 2.17.3-pre3

sed_extract_major='/^[0-9]/{s/^\([0-9]*\).*/\1/p;q;}
i\
0
q
'
sed_extract_minor='/^[0-9][0-9]*[.][0-9]/{s/^[0-9]*[.]\([0-9]*\).*/\1/p;q;}
i\
0
q
'
sed_extract_subminor='/^[0-9][0-9]*[.][0-9][0-9]*[.][0-9]/{s/^[0-9]*[.][0-9]*[.]\([0-9]*\).*/\1/p;q;}
i\
0
q
'

{
  echo "-DPACKAGE_VERSION_STRING=\"${version}\""
  echo "-DPACKAGE_VERSION_MAJOR="`echo "${version}" | sed -n -e "$sed_extract_major"`
  echo "-DPACKAGE_VERSION_MINOR="`echo "${version}" | sed -n -e "$sed_extract_minor"`
  echo "-DPACKAGE_VERSION_SUBMINOR="`echo "${version}" | sed -n -e "$sed_extract_subminor"`
} |
{
  if test -n "$escape"; then
    sed -e 's,\(["\\]\),\\\1,g'
  else
    cat
  fi
}
