#!/bin/sh
# $Id: check_paranoia.sh.in,v 1.6 2005/01/22 19:39:16 rocky Exp $
# Compare our cd-paranoia with an installed cdparanoia

if test "/usr/bin/cmp" != no -a ""X = X ; then
  ../src/cd-paranoia/cd-paranoia -d ./cdda.cue -v -r -- "1-"
  dd bs=16 skip=17 if=./cdda.raw of=./cdda-1.raw
  dd bs=16 if=./cdda.bin of=cdda-2.raw count=44377
  if /usr/bin/cmp ./cdda-1.raw ./cdda-2.raw ; then
    echo "** Raw cdda.bin extraction okay"
  else
    echo "** Raw cdda.bin extraction differ"
    exit 3
  fi
  mv cdda.raw cdda-good.raw
  ../src/cd-paranoia/cd-paranoia -d ./cdda.cue -x 64 -v -r -- "1-"
  mv cdda.raw cdda-underrun.raw
  ../src/cd-paranoia/cd-paranoia -d ./cdda.cue -r -- "1-"
  if /usr/bin/cmp ./cdda-underrun.raw ./cdda-good.raw ; then
    echo "** Under-run correction okay"
  else
    echo "** Under-run correction problem"
    exit 3
  fi
  # Start out with small jitter
  ../src/cd-paranoia/cd-paranoia -d ./cdda.cue -x 5 -v -r -- "1-"
  mv cdda.raw cdda-jitter.raw
  if /usr/bin/cmp ./cdda-jitter.raw ./cdda-good.raw ; then
    echo "** Small jitter correction okay"
  else
    echo "** Small jitter correction problem"
    exit 3
  fi
  # A more massive set of failures: underrun + medium jitter
  ../src/cd-paranoia/cd-paranoia -d ./cdda.cue -x 70 -v -r -- "1-"
  mv cdda.raw cdda-jitter.raw
  if /usr/bin/cmp ./cdda-jitter.raw ./cdda-good.raw ; then
    echo "** under-run + jitter correction okay"
  else
    echo "** under-run + jitter correction problem"
    exit 3
  fi
  ### FIXME: large jitter is known to fail. Investigate.
  exit 0
else 
  echo "Don't see libcdio cd-paranoia program. Test skipped."
  exit 77
fi
fi
#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***

