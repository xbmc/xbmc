#!/bin/sh
#4 July 96 Dan.Shearer@UniSA.edu.au
#
# 13 Aug 2001  Rafal Szczesniak <mimir@spin.ict.pwr.wroc.pl>
#   modified to accomodate international man pages (inspired
#   by Japanese edition's approach)


MANDIR=`echo $1 | sed 's/\/\//\//g'`
SRCDIR=$2
langs=$3

for lang in $langs; do
  echo Uninstalling \"$lang\" man pages from $MANDIR/$lang

  for sect in 1 5 7 8 ; do
    for m in $MANDIR/$lang/man$sect ; do
      for s in $SRCDIR/../docs/manpages/$lang/*$sect; do
        FNAME=$m/`basename $s`
	if test -f $FNAME; then
	  echo Deleting $FNAME
	  rm -f $FNAME 
	  test -f $FNAME && echo Cannot remove $FNAME... does $USER have privileges?   
        fi
      done
    done
  done
done

cat << EOF
======================================================================
The man pages have been uninstalled. You may install them again using 
the command "make installman" or make "install" to install binaries,
man pages and shell scripts.
======================================================================
EOF
exit 0
