#!/bin/sh
#4 July 96 Dan.Shearer@UniSA.edu.au   

INSTALLPERMS=$1
DESTDIR=$2
prefix=`echo $3 | sed 's/\/\//\//g'`
LIBDIR=`echo $4 | sed 's/\/\//\//g'`
shift
shift
shift
shift

if [ ! -d $DESTDIR/$LIBDIR ]; then
  echo "Directory $DESTDIR/$LIBDIR does not exist! "
  echo "Do a "make installmodules" or "make install" first. "
  exit 1
fi

for p in $*; do
  p2=`basename $p`
  if [ -f $DESTDIR/$LIBDIR/$p2 ]; then
    echo "Removing $DESTDIR/$LIBDIR/$p2 "
    rm -f $DESTDIR/$LIBDIR/$p2
    if [ -f $DESTDIR/$LIBDIR/$p2 ]; then
      echo "Cannot remove $DESTDIR/$LIBDIR/$p2 ... does $USER have privileges? "
    fi
  fi
done


cat << EOF
======================================================================
The modules have been uninstalled. You may restore the modules using
the command "make installmodules" or "make install" to install 
binaries, modules, man pages and shell scripts. 
======================================================================
EOF

exit 0
