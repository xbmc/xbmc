#!/bin/bash
if which pulse-session; then
  XBMC="pulse-session xbmc --standalone \"$@\""
else
  XBMC="xbmc --standalone \"$@\""
fi

LOOP=1
CRASHCOUNT=0
LASTSUCCESSFULSTART=$(date +%s)
ulimit -c unlimited
while (( $LOOP ))
do
  $XBMC
  RET=$?
  NOW=$(date +%s)
  if (( ($RET >= 64 && $RET <=66) || $RET == 0 )); then # clean exit
    LOOP=0
  else # crash
    DIFF=$((NOW-LASTSUCCESSFULSTART))
    if (($DIFF > 60 )); then # Not on startup, ignore
      LASTSUCESSFULSTART=$NOW
      CRASHCOUNT=0
    else # at startup, look sharp
      CRASHCOUNT=$((CRASHCOUNT+1))
      if (($CRASHCOUNT >= 3)); then # Too many, bail out
        LOOP=0
        echo "XBMC has exited uncleanly 3 times in the ${DIFF}s. Something is probably wrong"
      fi
    fi
  fi
done

