#! /bin/sh
# topline.sh.
# update top lines in docs.

# get version number and current date
ver=$(grep PACKAGE_VERSION config.h | sed 's/.*VERSION *"\(.*\)".*/\1/')
if test -z "$ver"; then
  echo "Cannot get version from config.h" 1>&2
  exit 1
fi
date=$(date +"%Y-%m-%d")
# update
shopt -s nullglob
if test -z "$@"; then
  files=$(grep -l '^#=====' * 2>/dev/null)
else
  files=$@
fi
for fnm in $files; do
  echo "Updating $fnm..." 1>&2
  { echo "#============================================================================"
    echo "# Enca v$ver ($date)  guess and convert encoding of text files"
    echo "# Copyright (C) 2000-2003 David Necas (Yeti) <yeti@physics.muni.cz>"
    echo "# Copyright (C) 2009 Michal Cihar <michal@cihar.com>"
    echo "#============================================================================"
    sed -e '1,5 d' "$fnm"
  } > tmp$$
  mv -f tmp$$ $fnm
done
rm -f tmp$$
