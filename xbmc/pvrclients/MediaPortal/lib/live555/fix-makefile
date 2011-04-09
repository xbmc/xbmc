#!/bin/sh
# the next line restarts using tclsh \
exec tclsh8.4 "$0" "$@"

set makefileName [lindex $argv 0]
set tmpfileName /tmp/rsftmp

set inFid [open $makefileName r]
set outFid [open $tmpfileName w]

while {![eof $inFid]} {
	set line [gets $inFid]
	if {[string match *\)\$* $line]} {
		set pos [string first \)\$ $line]
		set prefix [string range $line 0 $pos]
		incr pos
		set suffix [string range $line $pos end]
		set line $prefix\ $suffix
	}

	puts $outFid $line
}

close $inFid
close $outFid
file rename -force $tmpfileName $makefileName
