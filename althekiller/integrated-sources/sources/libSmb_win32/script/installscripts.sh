#!/bin/sh
# this script courtesy of James_K._Foote.PARC@xerox.com
# 5 July 96 Dan.Shearer@UniSA.Edu.Au  Don't hardcode script names, get from Make

INSTALLPERMS=$1
BINDIR=`echo $2 | sed 's/\/\//\//g'`

shift
shift

echo Installing scripts in $BINDIR

for d in $BINDIR; do
 if [ ! -d $d ]; then
  mkdir $d
  if [ ! -d $d ]; then
    echo Failed to make directory $d
    echo Have you run installbin first?
    exit 1
  fi
 fi
done

for p in $*; do
  p2=`basename $p`
  echo Installing $BINDIR/$p2
  if [ -f $BINDIR/$p2 ]; then
    rm -f $BINDIR/$p2.old
    mv $BINDIR/$p2 $BINDIR/$p2.old
  fi
  cp $p $BINDIR/
  chmod $INSTALLPERMS $BINDIR/$p2
  if [ ! -f $BINDIR/$p2 ]; then
    echo Cannot copy $p2... does $USER have privileges?
  fi
done

cat << EOF
======================================================================
The scripts have been installed. You may uninstall them using
the command "make uninstallscripts" or "make install" to install binaries,
man pages and shell scripts. You may recover the previous version (if any
by "make revert".
======================================================================
EOF

exit 0
