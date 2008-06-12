#!/bin/sh
#4 July 96 Dan.Shearer@UniSA.edu.au   

INSTALLPERMS=$1
DESTDIR=$2
prefix=`echo $3 | sed 's/\/\//\//g'`
BINDIR=`echo $4 | sed 's/\/\//\//g'`
SBINDIR=${exec_prefix}/sbin
shift
shift
shift
shift

if [ ! -d $DESTDIR/$BINDIR ]; then
  echo "Directory $DESTDIR/$BINDIR does not exist! "
  echo "Do a "make installbin" or "make install" first. "
  exit 1
fi

for p in $*; do
  p2=`basename $p`
  if [ -f $DESTDIR/$BINDIR/$p2 ]; then
    echo "Removing $DESTDIR/$BINDIR/$p2 "
    rm -f $DESTDIR/$BINDIR/$p2
    if [ -f $DESTDIR/$BINDIR/$p2 ]; then
      echo "Cannot remove $DESTDIR/$BINDIR/$p2 ... does $USER have privileges? "
    fi
  fi

  # this is a special case, mount needs this in a specific location
  if test "$p2" = smbmount -a -f "$DESTDIR/sbin/mount.smbfs"; then
    echo "Removing $DESTDIR/sbin/mount.smbfs "
    rm -f "$DESTDIR/${SBINDIR}/sbin/mount.smbfs"
  fi
done


cat << EOF
======================================================================
The binaries have been uninstalled. You may restore the binaries using
the command "make installbin" or "make install" to install binaries, 
man pages, modules and shell scripts. You can restore a previous
version of the binaries (if there were any) using "make revert".
======================================================================
EOF

exit 0
