#!/bin/sh
#5 July 96 Dan.Shearer@unisa.edu.au  removed hardcoded values
#
# 13 Aug 2001  Rafal Szczesniak <mimir@spin.ict.pwr.wroc.pl>
#   modified to accomodate international man pages (inspired
#   by Japanese edition's approach)

MANDIR=`echo $1 | sed 's/\/\//\//g'`
SRCDIR=$2/
langs=$3

if [ $# -ge 4 ] ; then
  GROFF=$4                    # sh cmd line, including options 
fi

if test ! -d $SRCDIR../docs/manpages; then
	echo "No manpages present.  SVN development version maybe?"
	exit 0
fi

# Get the configured feature set
test -f "${SRCDIR}/config.log" && \
	eval `grep "^[[:alnum:]]*=.*" "${SRCDIR}/config.log"`

for lang in $langs; do
    if [ "X$lang" = XC ]; then
	echo Installing default man pages in $MANDIR/
	lang=.
    else
	echo Installing \"$lang\" man pages in $MANDIR/lang/$lang
    fi

    langdir=$MANDIR/$lang
    for d in $MANDIR $langdir $langdir/man1 $langdir/man5 $langdir/man7 $langdir/man8; do
	if [ ! -d $d ]; then
	    mkdir $d
	    if [ ! -d $d ]; then
		echo Failed to make directory $d, does $USER have privileges?
		exit 1
	    fi
	fi
    done

    for sect in 1 5 7 8 ; do
	for m in $langdir/man$sect ; do
	    for s in $SRCDIR../docs/manpages/$lang/*$sect; do
	    MP_BASENAME=`basename $s`

	    # Check if this man page if required by the configured feature set
	    case "${MP_BASENAME}" in
	    	smbsh.1) test -z "${SMBWRAPPER}" && continue ;;
		*) ;;
	    esac

	    FNAME="$m/${MP_BASENAME}"

	    # Test for writability.  Involves 
	    # blowing away existing files.
 
	    if (rm -f $FNAME && touch $FNAME); then
		if [ "x$GROFF" = x ] ; then
		    cp $s $m            # Copy raw nroff 
		else
		    echo "\t$FNAME"     # groff'ing can be slow, give the user
					#   a warm fuzzy.
		    $GROFF $s > $FNAME  # Process nroff, because man(1) (on
					#   this system) doesn't .
		fi
		chmod 0644 $FNAME
	    else
		echo Cannot create $FNAME... does $USER have privileges?
	    fi
	    done
	done
    done
done
cat << EOF
======================================================================
The man pages have been installed. You may uninstall them using the command
the command "make uninstallman" or make "uninstall" to uninstall binaries,
man pages and shell scripts.
======================================================================
EOF

exit 0

