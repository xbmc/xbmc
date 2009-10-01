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

#----------------------------------------------------------------
# file selection dialog
# written by T.IWAI
#----------------------------------------------------------------

#
# filebrowser window-path current-directory filter-pattern create-flag
#
# The selected or input file name is returned.
#
proc filebrowser {w {curdir ""} {filter ""} {singlefile 0} {creatable 0}} {
    global fs

    set fs(curdir) $curdir
    set fs(filter) $filter
    set fs(creatable) $creatable
    set fs(found) ""

    fs:init $w
    fs:update $w

    set oldFocus [focus]
    grab $w
    focus $w
    tkwait window $w
    focus $oldFocus

    return $fs(found)
}


#
# create a filebrowser dialog
#
proc fs:init {w} {
    global fs tk_priv
    set f [my-dialog $w "File Selector" 0 1 [list\
	    [list {  OK  } "fs:select $w"]\
	    [list "Cancel" "destroy $w"]\
	    [list "Rescan" "fs:update $w"]\
	    [list { Select All } "fs:selall $w"]]]

    frame $f.filter
    label $f.filter.label -text "Filter" -relief flat
    entry $f.filter.entry -width 60 -relief sunken -textvariable fs(filter)
    bind $f.filter.entry <Return> "focus $w; fs:update $w"
    pack $f.filter.label $f.filter.entry -side top -anchor w

    frame $f.df
    set fs(dirlist) [my-listbox $f.df.dir "Directories" 30x8]
    set fs(filelist) [my-listbox $f.df.file "Files" 30x8 1 1]
    pack $f.df.dir $f.df.file -side left -ipadx 2m

    frame $f.name
    label $f.name.label -text "Name" -relief flat
    entry $f.name.entry -width 60 -relief sunken -textvariable fs(curdir)
    bind $f.name.entry <Return> "focus $w; fs:update $w"
    pack $f.name.label $f.name.entry -side top -anchor w

    pack $f.filter $f.df $f.name -side top -pady 3m -fill x -padx 3m

    if {$tk_priv(new_tcltk)} {
	bind $fs(filelist) <Button-1> "$fs(dirlist) select clear 0 end"
	bind $fs(filelist) <Button-1> {+%W select anchor [%W nearest %y]}
	bind $fs(dirlist) <Button-1> "$fs(filelist) select clear 0 end"
	bind $fs(dirlist) <Button-1>  {+%W select anchor [%W nearest %y]}
    } else {
	bind $fs(filelist) <Button-1> "$fs(dirlist) select clear"
	bind $fs(filelist) <Button-1> {+%W select from [%W nearest %y]}
	bind $fs(dirlist) <Button-1> "$fs(filelist) select clear"
	bind $fs(dirlist) <Button-1>  {+%W select from [%W nearest %y]}
    }


    bind $f.df.file.list <Double-1> [list fs:select $w]
    bind $f.df.dir.list <Double-1> [list fs:changedir $w]
}


#
# set up selection on the dir/file listboxes
#
proc fs:init-lbox {} {
    global fs tk_priv
    if {$tk_priv(new_tcltk)} {
	$fs(dirlist) select clear 0 end
	$fs(filelist) select set 0
    } else {
	$fs(dirlist) select clear
	$fs(filelist) select from 0
	$fs(filelist) select to 0
    }
    if {[lindex [$fs(filelist) curselection] 0] == ""} {
	if {$tk_priv(new_tcltk)} {
	    $fs(filelist) select clear 0 end
	    $fs(dirlist) select set 0
	} else {
	    $fs(filelist) select clear
	    $fs(dirlist) select from 0
	    $fs(dirlist) select to 0
	}
    }
}

#
# get the current listbox path
#
proc fs:get-cur-lbox {} {
    global fs
    if {[lindex [$fs(filelist) curselection] 0] != ""} {
	return $fs(filelist)
    } elseif {[lindex [$fs(dirlist) curselection] 0] != ""} {
	return $fs(dirlist)
    } else {
	return ""
    }
}

#
# select the file or directory
#
proc fs:select {w} {
    global fs
    set curw [fs:get-cur-lbox]
    if {$curw == $fs(filelist)} {
	set idxlist [$fs(filelist) curselection]
	if {[llength $idxlist] > 0} {
	    set fs(found) {}
	    foreach idx $idxlist {
		set i [$fs(filelist) get $idx]
		if {$fs(curdir) != ""} {
		    lappend fs(found) $fs(curdir)/$i
		} else {
		    lappend fs(found) $i
		}
	    }
	    destroy $w
	}
    } elseif {$curw == $fs(dirlist)} {
	fs:changedir $w
    }
}

#
# select all files
#
proc fs:selall {w} {
    global fs
    set size [$fs(filelist) size]
    if {$size > 0} {
	set fs(found) {}
	for {set idx 0} {$idx < $size} {incr idx} {
	    set i [$fs(filelist) get $idx]
	    if {$fs(curdir) != ""} {
		lappend fs(found) $fs(curdir)/$i
	    } else {
		lappend fs(found) $i
	    }
	}
	destroy $w
    }
}

#
# go up to the parent directory
#
proc fs:updir {} {
    global fs
    if [regexp "^/.+" $fs(curdir)] {
	if {[regsub "/\[^/\]+$" $fs(curdir) "" newdir] && $newdir != ""} {
	    set fs(curdir) $newdir
	} else {
	    set fs(curdir) "/"
	}
    } else {
	if [regsub "/\[^/\]+$" $fs(curdir) "" newdir] {
	    set fs(curdir) $newdir
	} elseif [regexp "~.\[^/\]+" $fs(curdir)] {
	    set fs(curdir) [glob -nocomplain $fs(curdir)]
	    fs:updir
	} elseif {$fs(curdir) != "" && $fs(curdir) != "."} {
	    set fs(curdir) ""
	} elseif {$fs(curdir) == "" || $fs(curdir) == "."} {
	    set fs(curdir) [pwd]
	    fs:updir
	}
    }
}

#
# change to the selected directory
#
proc fs:changedir {w} {
    global fs
    set idx [lindex [$fs(dirlist) curselection] 0]
    if {$idx != ""} {
	set i [$fs(dirlist) get $idx]
	global fs
	if {$i == ".."} {
	    fs:updir
	} else {
	    if {$fs(curdir) != ""} {
		set fs(curdir) $fs(curdir)/$i
	    } else {
		set fs(curdir) $i
	    }
	}
	fs:update $w
    }
}

#
# scan files and directories
#
proc fs:update {w} {
    global fs

    set sortOption -ascii
    if ![file isdirectory $fs(curdir)] {
	if {[file exists $fs(curdir)] || $fs(creatable)} {
	    set fs(found) $fs(curdir)
	    destroy $w
	    return
	}
    }

    set dir $fs(dirlist)
    set file $fs(filelist)

    $dir delete 0 end
    $file delete 0 end

    if {$fs(filter) != ""} {
	set filter $fs(filter)
    } else {
	set filter "*"
    }

    set lookall "*"
    if {$fs(curdir) != ""} {
	set patbase "$fs(curdir)/"
    } else {
	set patbase ""
    }

    foreach i [lsort $sortOption [glob -nocomplain $patbase$filter]] {
	if ![regexp "^.*/(\[^/\]+)$" $i full base] {
	    set base $i
	}
	if {$base != "" && ![file isdirectory $i]} {
	    $file insert end $base
	}
    }

    set prev ".."
    $dir insert end $prev
    foreach i [lsort $sortOption [glob -nocomplain $patbase$lookall $patbase$filter]] {
	if {$i == $prev} {continue}
	if ![regexp "^.*/(\[^/\]+)$" $i full base] {
	    set base $i
	}
	if {$base != "" && [file isdirectory $i]} {
	    $dir insert end $base
	}
    }

    fs:init-lbox
}

