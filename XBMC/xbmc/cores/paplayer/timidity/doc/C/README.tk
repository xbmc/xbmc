---------------------------------------------------------------------
This is the README file of TkMidity Ver.1.5, Tcl/Tk interface for
TiMidity MIDI Converter/Player.

TkMidity realizes the Tk interface panel communicating with true
TiMidity program. By using Tk, you can enjoy a beautiful Motif-like
window without Motif libraries.


* WHAT'S NEW in 1.5

- Trace window using timer callback
- Forward/backward buttons
- A couple of bug fixes..


* CONTENTS

This archive contains the following files:

README.tk	- this file
tk_c.c		- tk-interface control source file
tkmidity.ptcl	- main tcl/tk source to be preprocessed
tkpanel.tcl	- main control panel tcl/tk script
browser.tcl	- file browser
misc.tcl	- miscellaneous subroutines
tkbitmaps/*.xbm	- bitmap files for TkMidity


* USAGE

There are four modes newly featured to TiMidity; repeat, shuffle,
auto-start and auto-exit modes. Repeat mode plays musics after all
files are finished repeatedly. Shuffle mode means the random pick-up
playing. When Auto-start is on, the TkMidity begins playing music as
soon as program starts. Auto-exit means to quit TkMidity automatically
after all songs are over. Each setting can be saved by Save Config
menu.

You can change the display configuration in Displays menu. This
configuration also can be saved on the start-up file by Save Config
menu.

From ver.1.3, File Open/Close menues and direct keyboard
controls are supported. You can append arbitrary files from file
browser. The shortcut key actions are as follows:

	[Enter]		: Start Playing
	[Space]		: Pause / Start Again
	[c]		: Stop Playing
	[q]		: Quit TkMidity
	[p] or [Left]	: Previous File
	[n] or [Right]	: Next File
	[v] or [Down]	: Volume Down (5%)
	[V] or [Up]	: Volume Up (5%)
	[F10]		: Menu Mode
	[Alt]+[Any]	: Select Each Menu

From this version (1.4), trace mode window is realized. You can see a
funny movements of volume and panning of each channel if you specify
the option flag in command line (see manual).


* PROGRAM NOTES

This version requires Tcl7.5/Tk4.1 libraries.  Unlike the older tk
interface, timidity links the tcl/tk libraries on its binary, not
using wish program.  Also, shared memory access must be permitted.


* TROUBLE SHOOTING

+The present script verifies the existence of the file before pass to
TiMidity, but occasionally this could happen...

		Takashi Iwai	<iwai@dragon.mm.t.u-tokyo.ac.jp>
				<http://bahamut.mm.t.u-tokyo.ac.jp/~iwai/>
