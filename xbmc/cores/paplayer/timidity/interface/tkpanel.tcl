#!/usr/local/bin/wishx -f
#
# TiMidity++ -- MIDI to WAVE converter and player
# Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
# Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# TkMidity -- Tcl/Tk Interface for TiMidity
#	written by Takashi IWAI
#
# Tk control panel routine
#

#----------------
# initialize global variables
#
proc InitGlobal {} {

    global Stat tk_patchLevel

    if [catch {expr $Stat(new_tcltk) == 0 || $Stat(new_tcltk) == 1}] {
	set Stat(new_tcltk) 0
	if [regexp "(\[0-9\]+\.\[0-9\]+)" $tk_patchLevel cur] {
	    if {$cur >= 4.0} {
		set Stat(new_tcltk) 1
	    }
	}
    }

    # time table and volume
    set Stat(MaxSecs) 0
    set Stat(LastSec) 0
    set Stat(TotalTimeStr) "/ 0:00"

    # message lines
    set Stat(CurMsgs) 0
    set Stat(MaxMsgs) 500

    # current status
    set Stat(Playing) 0
    set Stat(Paused) 0
    set Stat(Blinking) 0

    # MIDI file list
    set Stat(CurIdx) -1
    set Stat(MaxFiles) 0
    set Stat(FileList) {}
    set Stat(ShuffleList) {}
    set Stat(CurFile) "--------"
    set Stat(NextFile) "--------"

    global Config
    # playing mode
    set Config(Tracing) 0
    set Config(RepeatPlay) 0
    set Config(ShufflePlay) 0
    set Config(AutoStart) 0
    set Config(AutoExit) 0
    set Config(ConfirmExit) 1

    # display configuration
    set Config(Disp:file) 1
    set Config(Disp:time) 1
    set Config(Disp:text) 1
    set Config(Disp:volume) 1
    set Config(Disp:button) 1
    set Config(Disp:trace) 1

    set Config(CurFileMode) 0

    # current volume
    set Config(CurVol) 75

    wm title . "TkMidity"
    wm iconname . "TkMidity"
    global bitmap_path
    wm iconbitmap . @$bitmap_path/timidity.xbm
}


#----------------
# read a message from stdin
#
proc HandleInput {} {
    global Stat Config

    set mlist [gets stdin]
    set msg [lindex $mlist 0]

    if {$msg == "RSET"} {
	if {$Config(Tracing)} {
	    TraceReset
	}
    } elseif {$msg == "TIME"} {
	# set total time
	set csecs [expr [lindex $mlist 1] / 100]
	set mins [expr $csecs / 60]
	set secs [expr $csecs % 60]
	set Stat(TotalTimeStr) [format "/ %d:%02d" $mins $secs]
	set Stat(MaxSecs) $csecs
	set tics [expr $csecs / 8]
	set tics [expr (($tics + 4) / 5) * 5]
	.body.time.scale configure -tickinterval $tics -to $csecs
	SetTime 0

    } elseif {$msg == "MVOL"} {
	# set master volume
	SetVolume [lindex $mlist 1]

    } elseif {$msg == "FILE"} {
	# set playing file
	set Stat(CurFile) [retrieve-filename [lindex $mlist 1]]
	wm title . "TkMidity: $Stat(CurFile)"
	wm iconname . "TkMidity: $Stat(CurFile)"
	if {$Config(CurFileMode) == 0} {
	    .body.curfile configure -text "$Stat(CurFile) / 00:00"
	} else {
	    .body.curfile configure -text "$Stat(CurFile) / -$Stat(TotalTimeStr)"
	}
	AppendMsg "------"

    } elseif {$msg == "LIST"} {
	# set file list
	.body.file.list delete 0 end
	set Stat(MaxFiles) [lindex $mlist 1]
	set Stat(FileList) {}
	for {set i 0} {$i < $Stat(MaxFiles)} {incr i} {
	    set file [gets stdin]
	    .body.file.list insert end $file
	    lappend Stat(FileList) $file
	}

	set Stat(CurIdx) -1
	SelectNumber

    } elseif {$msg == "ALST"} {
	# append file list
	set i $Stat(MaxFiles)
	set Stat(MaxFiles) [expr $i + [lindex $mlist 1]]
	for {} {$i < $Stat(MaxFiles)} {incr i} {
	    set file [gets stdin]
	    .body.file.list insert end $file
	    if {$Config(ShufflePlay)} {
		lappend Stat(ShuffleList) [llength $Stat(FileList)]
	    }
	    lappend Stat(FileList) $file
	}
	.body.file.list select set $Stat(CurIdx)

    } elseif {$msg == "ERRR"} {
	# delete file list
	.body.file.list delete $Stat(CurIdx)
	set Stat(MaxFiles) [expr $Stat(MaxFiles) - 1]
	set tmplist {}
	for {set i 0} {$i < $Stat(CurIdx)} {incr i} {
	    lappend tmplist [lindex $Stat(FileList) $i]
	}
	for {set i $Stat(CurIdx)} {$i < $Stat(MaxFiles)} {incr i} {
	    lappend tmplist [lindex $Stat(FileList) [expr $i + 1]]
	}
	set Stat(FileList) {}
	for {set i 0} {$i < $Stat(MaxFiles)} {incr i} {
	    lappend Stat(FileList) [lindex $tmplist $i]
	}
	if {$Stat(CurIdx) >= $Stat(MaxFiles)} {
	    if {$Config(RepeatPlay)} {
		set Stat(CurIdx) 0
	    } elseif {$Config(AutoExit)} {
		QuitCmd
	    } else {
		StopCmd
	    }
	}
	SelectNumber

    } elseif {$msg == "PREV"} {
	# previous file
	set Stat(CurIdx) [expr $Stat(CurIdx) - 1]
	if {$Stat(CurIdx) < 0} {set Stat(CurIdx) 0}
	SelectNumber

    } elseif {$msg == "NEXT"||$msg == "TEND"} {
	# next file
	incr Stat(CurIdx)
	if {$Stat(CurIdx) >= $Stat(MaxFiles)} {
	    if {$Config(RepeatPlay)} {
		set Stat(CurIdx) 0
	    } elseif {$Config(AutoExit)} {
		QuitCmd
	    } else {
		StopCmd
	    }
	}
	SelectNumber

    } elseif {$msg == "CMSG"} {
	# put message
	set type [lindex $mlist 1]
	set str [gets stdin]
	AppendMsg $str

    } elseif {$msg == "LYRC"} {
	# put message
	set type [lindex $mlist 1]
	set str [gets stdin]
	.body.text.buf insert end $str

    } elseif {$msg == "CERR"} {
	error [format "%s: %s" $Stat(NextFile) [gets stdin]]
	WriteMsg "NEXT"

    } elseif {$msg == "QUIT"} {
	# quit
	exit
    } elseif {$msg == "RSTA"} {
	# restart file
	SelectNumber
    }
}


#----------------
# make shuffled list
#
proc MakeShuffleList {} {
    global Stat
    set Stat(ShuffleList) {}
    for {set i 0} {$i < $Stat(MaxFiles)} {incr i} {
	lappend Stat(ShuffleList) $i
    }
    set len [expr $Stat(MaxFiles) - 1]
    while {$len > 0} {
	set pos [my-random [expr $len + 1]]
	set tmp [lindex $Stat(ShuffleList) $pos]
	set Stat(ShuffleList) [lreplace $Stat(ShuffleList) $pos $pos \
					[lindex $Stat(ShuffleList) $len]]
	set Stat(ShuffleList) [lreplace $Stat(ShuffleList) $len $len $tmp]
	set len [expr $len - 1]
    }
}

#
# append a string to message buffer
#
proc AppendMsg {str} {
    global Stat

    incr Stat(CurMsgs)
    if {$Stat(CurMsgs) >= $Stat(MaxMsgs)} { ClearMsg }
    .body.text.buf insert end $str\n
    .body.text.buf yview -pickplace end
}

#
# clear message buffer
#
proc ClearMsg {} {
    global Stat
    .body.text.buf delete 0.0 end
    .body.text.buf yview 0
    set Stat(CurMsgs) 0
}


#----------------
# select the file in listbox and load it
#
proc SelectNumber {} {
    global Stat Config
    if {$Stat(new_tcltk)} {
	.body.file.list select clear 0 end
    } else {
	.body.file.list select clear
    }
    set idx -1
    if {$Stat(CurIdx) >= 0 && $Stat(CurIdx) < [llength $Stat(FileList)]} {
	if {$Config(ShufflePlay)} {
	    if {$Stat(ShuffleList) == {}} {
		MakeShuffleList
	    }
	    set idx [lindex $Stat(ShuffleList) $Stat(CurIdx)]
	} else {
	    set idx $Stat(CurIdx)
	}
	set thefile [lindex $Stat(FileList) $idx]
	set Stat(NextFile) $thefile
    }

    if {$idx >= 0} {
	if {$Stat(new_tcltk)} {
#	    .body.file.list select set $idx
	} else {
#	    .body.file.list select from $idx
#	    .body.file.list select to $idx
	}
	.body.curfile configure -text\
		"Playing: [lindex $Stat(FileList) $idx]"
	LoadCmd $idx
	set Stat(Playing) 1
    } else {
	SetTime 0
	.body.curfile configure -text "-------- / 00:00"
	set Stat(Playing) 0
	set Stat(Paused) 0
    }
    DispButtonPlay
}


#
# update current time
#
proc SetTime {val} {
    global Stat Config
    if {$Stat(CurIdx) == -1} {
	return
    }
    set Stat(LastSec) $val
    if {$Config(CurFileMode) == 0} {
	set curt [sec2time $val]
	.body.curfile configure\
		-text "$Stat(CurFile) / $curt"
    } else {
	set curt [sec2time [expr $val - $Stat(MaxSecs)]]
	.body.curfile configure\
		-text "$Stat(CurFile) / $curt"
    }
    set curt [sec2time $val]
    .body.time.label configure\
	    -text "$curt / $Stat(TotalTimeStr)"
    .body.time.scale set $val
    DispButtonPlay
}


#
# colorize buttons with each state
#
proc DispButtonPlay {} {
    global Stat
    if {$Stat(Playing)} {
	if {$Stat(Blinking)} {
	    set color green
	    set Stat(Blinking) 0
	} else {
	    set color red
	    set Stat(Blinking) 1
	}
	.body.button.play configure -fg $color -activeforeground $color
    } else {
	.body.button.play configure -fg black -activeforeground black
    }

    if {$Stat(Playing) && $Stat(Paused)} {
	.body.button.pause configure -fg red -activeforeground red
    } else {
	.body.button.pause configure -fg black -activeforeground black
    }
}


#
# update current volume
#
proc SetVolume {val} {
    global Config
    set Config(CurVol) $val
    .body.volume.scale set $val
}


#----------------
# write message
# messages are: PREV, NEXT, QUIT, FWRD, BACK, RSET, STOP
#	LOAD\n<filename>, JUMP <time>, VOLM <volume>
#

proc WriteMsg {str} {
    puts stdout $str
    flush stdout
}


#----------------
# callback commands
#

#
# jump to specified time
#
proc JumpCmd {val} {
    global Stat
    if {$val != $Stat(LastSec)} {
	WriteMsg [format "JUMP %d" [expr $val * 100]]
    }
}

#
# change volume amplitude
#
proc VolumeCmd {val {force 0}} {
    global Config
    if {$val < 0} {set val 0}
    if {$val > 200} {set val 200}
    if {$force != 0 || $val != $Config(CurVol)} {
	WriteMsg [format "VOLM %d" $val]
    }
}

#
# load the specified file
#
proc LoadCmd {idx} {
    global Stat Config
    WriteMsg "LOAD"
    WriteMsg [lindex $Stat(FileList) $idx]
    AppendMsg ""
}

#
# play the first file
#
proc PlayCmd {} {
    global Stat
    if {$Stat(Playing) == 0} {
	WriteMsg "NEXT"
    }
}

proc PauseCheck {} {
    global Stat
    if {$Stat(Paused)} {
	set Stat(Paused) 0
    }
    DispButtonPlay
}

#
# pause music
#
proc PauseCmd {} {
    global Stat
    if {$Stat(Playing)} {
	if {$Stat(Paused)} {
	    set Stat(Paused) 0
	} else {
	    set Stat(Paused) 1
	}
	DispButtonPlay
	WriteMsg "STOP"
    }
}

#
# stop playing
#
proc StopCmd {} {
    global Stat Config
    if {$Stat(Playing)} {
	WriteMsg "QUIT"
	WriteMsg "XTND"
	set Stat(CurIdx) -1
	SelectNumber
	if {$Config(Tracing)} {
	    TraceReset
	}
	wm title . "TkMidity"
	wm iconname . "TkMidity"
    }
}

#
# quit TkMidity
#
proc QuitCmd {} {
    global Config Stat
    if {$Config(AutoExit) || !$Config(ConfirmExit)} {
	WriteMsg "QUIT"
	WriteMsg "ZAPP"
	return
    }
    set oldpause $Stat(Paused)
    if {!$oldpause} {PauseCmd}
    if {[question "Really Quit TkMidity?" 0]} {
	WriteMsg "QUIT"
	WriteMsg "ZAPP"
	return
    }
    if {!$oldpause} {PauseCmd}
}

#
# play previous file
#
proc PrevCmd {} {
    global Stat Config
    if {$Stat(Playing)} {
	WriteMsg "PREV"
	PauseCheck
    }
    VolumeCmd $Config(CurVol) 1
}

#
# play next file
#
proc NextCmd {} {
    global Stat Config
    if {$Stat(Playing)} {
	WriteMsg "NEXT"
	PauseCheck
    }
    VolumeCmd $Config(CurVol) 1
}

#
# forward/backward 2 secs
#
proc ForwardCmd {} {
    global Stat
    if {$Stat(Playing)} {
	WriteMsg "FWRD"
	PauseCheck
    }
}

proc BackwardCmd {} {
    global Stat
    if {$Stat(Playing)} {
	WriteMsg "BACK"
	PauseCheck
    }
}


#
# volume up/down
#
proc VolUpCmd {} {
    global Stat Config
    if {$Stat(Playing)} {
	VolumeCmd [expr $Config(CurVol) + 5]
    }
}

proc VolDownCmd {} {
    global Stat Config
    if {$Stat(Playing)} {
	VolumeCmd [expr $Config(CurVol) - 5]
    }
}

#----------------
# display configured tables
#
proc DispTables {} {
    global Config
    set allitems {file time text volume button trace}

    foreach i $allitems {
	pack forget .body.$i
	if {$Config(Disp:$i)} {
	    pack .body.$i -side top -fill x
	}
    }
}

#
# save configuration and playing mode
#
proc SaveConfig {} {
    global Config ConfigFile
    set fd [open $ConfigFile w]
    if {$fd != ""} {
	puts $fd "global Config"
	foreach i [array names Config] {
	    puts $fd "set Config($i) $Config($i)"
	}
	close $fd
    }
}

#
# load configuration file
#
proc LoadConfig {} {
    global ConfigFile Stat
    catch {source $ConfigFile}
}

#
# from command line
#
proc InitCmdLine {argc argv} {
    global Config
    set Config(Disp:trace) 0
    for {set i 0} {$i < $argc} {incr i} {
	if {[lindex $argv $i] == "-mode"} {
	    incr i
	    set mode [lindex $argv $i]
	    if {$mode == "trace"} {
		set Config(Tracing) 1
		set Config(Disp:trace) 1
	    } elseif {$mode == "shuffle"} {
		set Config(ShufflePlay) 1
	    } elseif {$mode == "normal"} {
		set Config(ShufflePlay) 0
	    } elseif {$mode == "autostart"} {
		set Config(AutoStart) 1
	    } elseif {$mode == "autoexit"} {
		set Config(AutoExit) 1
	    } elseif {$mode == "repeat"} {
		set Config(RepeatPlay) 1
	    }
	}
    }
}


#
# selection callback of the playing file from listbox
#
proc SelectList {lw pos} {
    global Config Stat
    set idx [$lw nearest $pos]
    if {$idx >= 0 && $idx < $Stat(MaxFiles)} {
	if {$Config(ShufflePlay)} {
	    set found [lsearch -exact $Stat(ShuffleList) $idx]
	    if {$found != -1} {
		set Stat(CurIdx) $found
	    }
	} else {
	    set Stat(CurIdx) $idx
	}
	set Stat(Playing) 1
	PauseCheck
	SelectNumber
    }
}


#
#
#
proc OpenFiles {} {
    global Stat
    set files [filebrowser .browser "" "*.{mid*,kar,lzh,zip}"]
    if {$files != ""} {
	WriteMsg "XPND"
	WriteMsg $files
    }
}

#
#
#
proc CloseFiles {} {
    global Stat
    if {[question "Really Clear List?" 0]} {
	StopCmd
	.body.file.list delete 0 end
	set Stat(MaxFiles) 0
	set Stat(FileList) {}
	set Stat(ShuffleList) {}
    }
}

proc ToggleCurFileMode {} {
    global Config Stat
    if {$Config(CurFileMode) == 0} {
	set Config(CurFileMode) 1
    } else {
	set Config(CurFileMode) 0
    }
    SetTime $Stat(LastSec)
}

#----------------
# create main window
#

proc CreateWindow {} {
    global Config Stat

    # menu bar
    frame .menu -relief raised -bd 1
    pack .menu -side top -expand 1 -fill x

    # File menu
    menubutton .menu.file -text "File" -menu .menu.file.m\
	    -underline 0
    menu .menu.file.m
    .menu.file.m add command -label "Open Files" -underline 0\
	    -command OpenFiles
    .menu.file.m add command -label "Clear List" -underline 0\
	    -command CloseFiles
    .menu.file.m add command -label "Save Config" -underline 0\
	    -command SaveConfig
    .menu.file.m add command -label "About" -underline 0\
	    -command {
	information "TkMidity -- TiMidty Tcl/Tk Version\n  written by T.IWAI"
    }
    .menu.file.m add command -label "Quit" -underline 0\
	    -command QuitCmd

    # Mode menu
    menubutton .menu.mode -text "Mode" -menu .menu.mode.m\
	    -underline 0
    menu .menu.mode.m
    .menu.mode.m add check -label "Repeat" -underline 0\
	    -variable Config(RepeatPlay)
    .menu.mode.m add check -label "Shuffle" -underline 0\
	    -variable Config(ShufflePlay) -command {
	if {$Config(ShufflePlay)} {
	    MakeShuffleList
	} else {
	    set Stat(ShuffleList) {}
	}
    }
    .menu.mode.m add check -label "Auto Start" -underline 5\
	    -variable Config(AutoStart)
    .menu.mode.m add check -label "Auto Exit" -underline 5\
	    -variable Config(AutoExit)
    .menu.mode.m add check -label "Confirm Quit" -underline 0\
	    -variable Config(ConfirmExit)

    # Display menu
    menubutton .menu.disp -text "Display" -menu .menu.disp.m\
	    -underline 0
    menu .menu.disp.m
    .menu.disp.m add check -label "File List" -underline 0\
	    -variable Config(Disp:file) -command "DispTables"
    .menu.disp.m add check -label "Time" -underline 0\
	    -variable Config(Disp:time) -command "DispTables"
    .menu.disp.m add check -label "Messages" -underline 0\
	    -variable Config(Disp:text) -command "DispTables"
    .menu.disp.m add check -label "Volume" -underline 0\
	    -variable Config(Disp:volume) -command "DispTables"
    .menu.disp.m add check -label "Buttons" -underline 0\
	    -variable Config(Disp:button) -command "DispTables"
    if {$Config(Tracing)} {
	.menu.disp.m add check -label "Trace" -underline 1\
		-variable Config(Disp:trace) -command "DispTables"
    }

    pack .menu.file .menu.mode .menu.disp -side left

    # display body
    if {$Stat(new_tcltk)} {
	frame .body -relief flat
    } else {
	frame .body -relief raised -bd 1
    }
    pack .body -side top -expand 1 -fill both

    # current playing file
    button .body.curfile -text "-------- / 00:00" -relief ridge\
	    -command "ToggleCurFileMode"
    pack .body.curfile -side top -expand 1 -fill x

    # playing files list
    frame .body.file -relief raised -bd 1
    scrollbar .body.file.bar -relief sunken\
	    -command ".body.file.list yview"
    pack .body.file.bar -side right -fill y
    if {$Stat(new_tcltk)} {
	listbox .body.file.list -width 36 -height 10 -relief sunken -bd 2\
		-yscroll ".body.file.bar set"
    } else {
	listbox .body.file.list -geometry 36x10 -relief sunken -bd 2\
		-yscroll ".body.file.bar set"
    }
    bind .body.file.list <Button-1> {SelectList %W %y}

    pack .body.file.list -side top -expand 1 -fill both

    # time label and scale
    frame .body.time -relief raised -bd 1
    label .body.time.label -text "0:00 / 0:00"
    pack .body.time.label -side top
    scale .body.time.scale -orient horizontal -length 280\
	    -from 0 -to 100 -tickinterval 10
    bind .body.time.scale <ButtonRelease-1> {JumpCmd [%W get]}
    pack .body.time.scale -side bottom -expand 1 -fill x

    # text browser
    frame .body.text -relief raised -bd 1
    scrollbar .body.text.bar -relief sunken\
	    -command ".body.text.buf yview"
    pack .body.text.bar -side right -fill y
    text .body.text.buf -width 36 -height 12 -relief sunken -bd 2\
	    -wrap word -yscroll ".body.text.bar set"
    bind .body.text.buf <Button-1> { }
    bind .body.text.buf <Any-Key> { }
    pack .body.text.buf -side top -expand 1 -fill both
    button .body.text.clear -text "Clear"\
	    -command ClearMsg
    pack .body.text.clear -side bottom

    # volume label and scale
    frame .body.volume -relief raised -bd 1
    label .body.volume.label -text "Volume:"
    pack .body.volume.label -side top
    scale .body.volume.scale -orient horizontal -length 280\
	    -from 0 -to 200 -tickinterval 25\
	    -showvalue true -label "Volume"\
	    -command VolumeCmd
    pack .body.volume.scale -side bottom -expand 1 -fill x
    .body.volume.scale set $Config(CurVol)

    # buttons
    global bitmap_path
    frame .body.button -relief raised -bd 1
    pack .body.button -side top -expand 1 -fill x
    button .body.button.play -bitmap @$bitmap_path/play.xbm\
	    -command "PlayCmd"
    button .body.button.stop -bitmap @$bitmap_path/stop.xbm\
	    -command "StopCmd"
    button .body.button.prev -bitmap @$bitmap_path/prev.xbm\
	    -command "PrevCmd"
    button .body.button.back -bitmap @$bitmap_path/back.xbm\
	    -command "BackwardCmd"
    button .body.button.fwrd -bitmap @$bitmap_path/fwrd.xbm\
	    -command "ForwardCmd"
    button .body.button.next -bitmap @$bitmap_path/next.xbm\
	    -command "NextCmd"
    button .body.button.pause -bitmap @$bitmap_path/pause.xbm\
	    -command "PauseCmd"
    button .body.button.quit -bitmap @$bitmap_path/quit.xbm\
	    -command "QuitCmd"
    pack .body.button.play .body.button.pause\
	    .body.button.prev .body.button.back\
	    .body.button.stop\
	    .body.button.fwrd .body.button.next\
	    .body.button.quit\
	    -side left -ipadx 4 -pady 2 -fill x

    if {$Config(Tracing)} {
	# trace display
	TraceCreate
    }
    TraceUpdate

    # pack all items
    DispTables

    focus .
    tk_menuBar .menu .menu.file .menu.mode .menu.disp
    bind . <Key-Right> "ForwardCmd"
    bind . <Key-n> "NextCmd"
    bind . <Key-Left> "BackwardCmd"
    bind . <Key-p> "PrevCmd"
    bind . <Key-s> "PauseCmd"
    bind . <Key-Down> "VolDownCmd"
    bind . <Key-v> "VolDownCmd"
    bind . <Key-Up> "VolUpCmd"
    bind . <Key-V> "VolUpCmd"
    bind . <Key-space> "PauseCmd"
    bind . <Return> "PlayCmd"
    bind . <Key-c> "StopCmd"
    bind . <Key-q> "QuitCmd"

    VolumeCmd $Config(CurVol) 1
}
