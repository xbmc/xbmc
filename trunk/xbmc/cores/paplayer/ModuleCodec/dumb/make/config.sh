#!/bin/sh

# This file does an interactive configuration for users of Unix-like systems.
# It creates a config.txt file for inclusion in the Makefile. This script
# should be run indirectly through the 'make config' target (or the 'make'
# target the first time).

if [ ! -f make/dumbask ]; then
  echo "You should not be running this directly! Use 'make' or 'make config'."
  exit
fi

echo 'include make/unix.inc' > make/config.tmp

echo 'ALL_TARGETS := core core-examples core-headers' >> make/config.tmp

if make/dumbask 'Would you like support for Allegro (Y/N)? ' YN; then
  echo 'ALL_TARGETS += allegro allegro-examples allegro-headers' >> make/config.tmp
fi


if [ ! -z $DEFAULT_PREFIX ]; then
echo "Please specify an installation prefix (default $DEFAULT_PREFIX)."
echo -n '> '
read PREFIX
if [ -z $PREFIX ]; then PREFIX=$DEFAULT_PREFIX; fi
echo "PREFIX := `echo "$PREFIX" | \
  sed -e 's/\${\([A-Za-z_][A-Za-z0-9_]*\)}/$(\1)/g' \
      -e 's/\$\([A-Za-z_][A-Za-z0-9_]*\)/$(\1)/g'`" >> make/config.tmp
fi

mv -f make/config.tmp make/config.txt

echo 'Configuration complete.'
echo "Run 'make config' to change it in the future."
echo -n 'Press Enter to continue ... '
read dummy
