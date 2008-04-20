#!/bin/sh
#first version March 1998, Andrew Tridgell

DESTDIR=$1
SWATDIR=`echo $2 | sed 's/\/\//\//g'`
SRCDIR=$3/
BOOKDIR="$DESTDIR/$SWATDIR/using_samba"

case $0 in
	*uninstall*)
		echo "Removing SWAT from $DESTDIR/$SWATDIR "
		echo "Removing the Samba Web Administration Tool "
		echo -n "Removed "
		mode='uninstall'
		;;
	*)
		echo "Installing SWAT in $DESTDIR/$SWATDIR "
		echo "Installing the Samba Web Administration Tool "
		echo -n "Installing "
		mode='install'
		;;
esac

LANGS=". `cd $SRCDIR../swat/; /bin/echo lang/??`"
echo "langs are `cd $SRCDIR../swat/lang/; /bin/echo ??` "

if test "$mode" = 'install'; then
 for ln in $LANGS; do 
  SWATLANGDIR="$DESTDIR/$SWATDIR/$ln"
  for d in $SWATLANGDIR $SWATLANGDIR/help $SWATLANGDIR/images \
  $SWATLANGDIR/include $SWATLANGDIR/js; do
   if [ ! -d $d ]; then
    mkdir -p $d
    if [ ! -d $d ]; then
     echo "Failed to make directory $d, does $USER have privileges? "
     exit 1
    fi
   fi
  done
 done
fi

for ln in $LANGS; do

  # images
  for f in $SRCDIR../swat/$ln/images/*.gif; do
    if [ ! -f $f ] ; then
      continue
    fi
    FNAME="$DESTDIR/$SWATDIR/$ln/images/`basename $f`"
    echo $FNAME
    if test "$mode" = 'install'; then
      cp "$f" "$FNAME"
      if test ! -f "$FNAME"; then
        echo "Cannot install $FNAME. Does $USER have privileges? "
        exit 1
      fi
      chmod 0644 "$FNAME"
    elif test "$mode" = 'uninstall'; then
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

  # html help
  for f in $SRCDIR../swat/$ln/help/*.html; do
    if [ ! -f $f ] ; then
      continue
    fi
    FNAME="$DESTDIR/$SWATDIR/$ln/help/`basename $f`"
    echo $FNAME
    if test "$mode" = 'install'; then
      if [ "x$BOOKDIR" = "x" ]; then
        cat $f | sed 's/@BOOKDIR@.*$//' > $f.tmp
      else
        cat $f | sed 's/@BOOKDIR@//' > $f.tmp
      fi
      f=$f.tmp
      cp "$f" "$FNAME"
      rm -f "$f"
      if test ! -f "$FNAME"; then
        echo "Cannot install $FNAME. Does $USER have privileges? "
        exit 1
      fi
      chmod 0644 "$FNAME"
    elif test "$mode" = 'uninstall'; then
      rm -f "$FNAME"
      if test -f "$FNAME"; then
        echo "Cannot remove $FNAME.  Does $USER have privileges? "
        exit 1
      fi
    fi
  done

  # "server-side" includes
  for f in $SRCDIR../swat/$ln/include/*; do
      if [ ! -f $f ] ; then
	continue
      fi
      FNAME="$DESTDIR/$SWATDIR/$ln/include/`basename $f`"
      echo $FNAME
      if test "$mode" = 'install'; then
        cp "$f" "$FNAME"
        if test ! -f "$FNAME"; then
          echo "Cannot install $FNAME. Does $USER have privileges? "
          exit 1
        fi
        chmod 0644 $FNAME
      elif test "$mode" = 'uninstall'; then
        rm -f "$FNAME"
        if test -f "$FNAME"; then
          echo "Cannot remove $FNAME.  Does $USER have privileges? "
          exit 1
        fi
      fi
  done

done

# Install/ remove html documentation (if html documentation tree is here)

if [ -d $SRCDIR../docs/htmldocs/ ]; then

    for dir in htmldocs/manpages htmldocs/Samba3-ByExample  htmldocs/Samba3-Developers-Guide  htmldocs/Samba3-HOWTO
    do 
    
      if [ ! -d $SRCDIR../docs/$dir ]; then
        continue
      fi
      
      INSTALLDIR="$DESTDIR/$SWATDIR/help/`echo $dir | sed 's/htmldocs\///g'`"
      if test ! -d "$INSTALLDIR" -a "$mode" = 'install'; then
        mkdir "$INSTALLDIR"
        if test ! -d "$INSTALLDIR"; then
          echo "Failed to make directory $INSTALLDIR, does $USER have privileges? "
          exit 1
        fi
      fi

      for f in $SRCDIR../docs/$dir/*.html; do
	  FNAME=$INSTALLDIR/`basename $f`
	  echo $FNAME
          if test "$mode" = 'install'; then
            cp "$f" "$FNAME"
            if test ! -f "$FNAME"; then
              echo "Cannot install $FNAME. Does $USER have privileges? "
              exit 1
            fi
            chmod 0644 $FNAME
          elif test "$mode" = 'uninstall'; then
            rm -f "$FNAME"
            if test -f "$FNAME"; then
              echo "Cannot remove $FNAME.  Does $USER have privileges? "
              exit 1
            fi
          fi
      done

      if test -d "$SRCDIR../docs/$dir/images/"; then
          if test ! -d "$INSTALLDIR/images/" -a "$mode" = 'install'; then
              mkdir "$INSTALLDIR/images"
              if test ! -d "$INSTALLDIR/images/"; then
                  echo "Failed to make directory $INSTALLDIR/images, does $USER have privileges? "
                  exit 1
              fi
          fi
          for f in $SRCDIR../docs/$dir/images/*.png; do
              FNAME=$INSTALLDIR/images/`basename $f`
              echo $FNAME
              if test "$mode" = 'install'; then
                cp "$f" "$FNAME"
                if test ! -f "$FNAME"; then
                  echo "Cannot install $FNAME. Does $USER have privileges? "
                  exit 1
                fi
                chmod 0644 $FNAME
              elif test "$mode" = 'uninstall'; then
                rm -f "$FNAME"
                if test -f "$FNAME"; then
                  echo "Cannot remove $FNAME.  Does $USER have privileges? "
                  exit 1
                fi
              fi
          done
      fi
    done
fi

# Install/ remove Using Samba book (but only if it is there)

if [ "x$BOOKDIR" != "x" -a -f $SRCDIR../docs/htmldocs/using_samba/toc.html ]; then

    # Create directories

    for d in $BOOKDIR $BOOKDIR/figs ; do
        if test ! -d "$d" -a "$mode" = 'install'; then
            mkdir $d
            if test ! -d "$d"; then
                echo "Failed to make directory $d, does $USER have privileges? "
                exit 1
            fi
        fi
    done

    # HTML files

    for f in $SRCDIR../docs/htmldocs/using_samba/*.html; do
        FNAME=$BOOKDIR/`basename $f`
        echo $FNAME
        if test "$mode" = 'install'; then
          cp "$f" "$FNAME"
          if test ! -f "$FNAME"; then
            echo "Cannot install $FNAME. Does $USER have privileges? "
            exit 1
          fi
          chmod 0644 $FNAME
        elif test "$mode" = 'uninstall'; then
          rm -f "$FNAME"
          if test -f "$FNAME"; then
            echo "Cannot remove $FNAME.  Does $USER have privileges? "
            exit 1
          fi
        fi
    done

    for f in $SRCDIR../docs/htmldocs/using_samba/*.gif; do
        FNAME=$BOOKDIR/`basename $f`
        echo $FNAME
        if test "$mode" = 'install'; then
          cp "$f" "$FNAME"
          if test ! -f "$FNAME"; then
            echo "Cannot install $FNAME. Does $USER have privileges? "
            exit 1
          fi
          chmod 0644 $FNAME
        elif test "$mode" = 'uninstall'; then
          rm -f "$FNAME"
          if test -f "$FNAME"; then
            echo "Cannot remove $FNAME.  Does $USER have privileges? "
            exit 1
          fi
        fi
    done

    # Figures

    for f in $SRCDIR../docs/htmldocs/using_samba/figs/*.gif; do
        FNAME=$BOOKDIR/figs/`basename $f`
        echo $FNAME
        if test "$mode" = 'install'; then
          cp "$f" "$FNAME"
          if test ! -f "$FNAME"; then
            echo "Cannot install $FNAME. Does $USER have privileges? "
            exit 1
          fi
          chmod 0644 $FNAME
        elif test "$mode" = 'uninstall'; then
          rm -f "$FNAME"
          if test -f "$FNAME"; then
            echo "Cannot remove $FNAME.  Does $USER have privileges? "
            exit 1
          fi
        fi
    done

fi

if test "$mode" = 'install'; then
  cat << EOF
======================================================================
The SWAT files have been installed. Remember to read the documentation
for information on enabling and using SWAT
======================================================================
EOF
else
  cat << EOF
======================================================================
The SWAT files have been removed.  You may restore these files using
the command "make installswat" or "make install" to install binaries, 
man pages, modules, SWAT, and shell scripts.
======================================================================
EOF
fi

exit 0

