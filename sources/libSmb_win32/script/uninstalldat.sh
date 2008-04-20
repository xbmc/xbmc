#!/bin/sh
#fist version March 2002, Herb  Lewis

DESTDIR=$1
DATDIR=`echo $2 | sed 's/\/\//\//g'`
SRCDIR=$3/
shift
shift
shift

case $0 in
	*uninstall*)
		if test ! -d "$DESTDIR/$DATDIR"; then
			echo "Directory $DESTDIR/$DATDIR does not exist! "
			echo "Do a "make installmsg" or "make install" first. "
			exit 1
		fi
		mode='uninstall'
		;;
	*) mode='install' ;;    
esac

for f in $SRCDIR/codepages/*.dat; do
	FNAME="$DESTDIR/$DATDIR/`basename $f`"
	if test "$mode" = 'install'; then
		echo "Installing $f as $FNAME "
		cp "$f" "$FNAME"
		if test ! -f "$FNAME"; then
			echo "Cannot install $FNAME.  Does $USER have privileges? "
			exit 1
		fi
		chmod 0644 "$FNAME"
	elif test "$mode" = 'uninstall'; then
		echo "Removing $FNAME "
		rm -f "$FNAME"
		if test -f "$FNAME"; then
			echo "Cannot remove $FNAME.  Does $USER have privileges? "
			exit 1
		fi
	else
		echo "Unknown mode, $mode.  Script called as $0 "
		exit 1
	fi
done

if test "$mode" = 'install'; then
	cat << EOF
======================================================================
The dat files have been installed.  You may uninstall the dat files
using the command "make uninstalldat" or "make uninstall" to uninstall
binaries, man pages, dat files, and shell scripts.
======================================================================
EOF
else
	cat << EOF
======================================================================
The dat files have been removed.  You may restore these files using
the command "make installdat" or "make install" to install binaries, 
man pages, modules, dat files, and shell scripts.
======================================================================
EOF
fi

exit 0

