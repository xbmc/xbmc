REM $Id: bad-msf-2.cue,v 1.1 2004/07/10 01:18:02 rocky Exp $
REM bad MSF in second field - seconds should be less than 60

FILE "cdda.bin" BINARY

TRACK 01 AUDIO
    INDEX 01 00:90:00
