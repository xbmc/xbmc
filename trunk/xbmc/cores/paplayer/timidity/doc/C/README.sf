================================================================
	** Timidity SoundFont Extension **

	    written by Takashi Iwai
		<iwai@dragon.mm.t.u-tokyo.ac.jp>
		<http://bahamut.mm.t.u-tokyo.ac.jp/~iwai/>

	patch level 1: April 2, 1997
================================================================

* WHAT'S THIS?

This is an extension to use samples in SoundFont files with
timidity-0.2i.  You can employ both SoundFont file together with
ordinary GUS patch files.  Both SBK and SF2 formats are supported.


* USAGE

Two commands are newly added in configuration.

To specify the SoundFont file to be used, just add a line in config
file like:

	 soundfont sffile [order=number]

The first parameter is the file name to be loaded.  The file itself
is stored once after reading all configurations, then converted to
the internal records except wave sample data.

The optional argument specifies the order of searching.
'order=0' means to search the SoundFont file at first, then search
the GUS patches if the appropriate sample is not found.
'order=1' means to search the SoundFont file after GUS patches.

Another command 'font' is supplied to control the behavior of sample
selection.  If you don't want to use some samples in the SoundFont
file, specify the sample via 'exclude' sub-command.

	font exclude bank [preset [keynote]]

The first parameter is MIDI bank number of the sample to be removed.
The optional second parameter is MIDI program number of the sample.
For drum samples, specify 128 as bank, and drumset number as preset,
and keynote number for the drum sample.

You can change the order of individual sample (or bank) via "order"
sub-command.

	font order number bank [preset [keynote]]

The first parameter is the order number (zero or one) to be changed,
and the sequent parameters are as well as in exclude command above.


* BUGS & TODO'S

- noises on some bass drum samples
- support of modulation envelope
- support of cut off / resonance
- support of chorus / reverb


* CHANGES

- pl.1
	+ fix volume envelope calcuation
	+ add font command
	+ fix font-exclude control

