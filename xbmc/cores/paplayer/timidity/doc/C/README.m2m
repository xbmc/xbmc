m2m: mod->midi file converter for TiMidity++




BRIEF SYNOPSIS:

This adds the new -OM output mode to TiMidity++, which will read in a mod file
and output a midi file.  All parameters needed for the conversion are
contained in a .m2m file of the same base name as the mod.  If this file can
not be found, it will generate one for you.  Chord assignment and
transposition values can be very difficult and tedious to assign by hand.  It
is STRONGLY recommended that you let the program generate the initial .m2m
file, so that it can do most (if not all) of this for you.  You will still
need to assign drums, correct banks/programs, and tweak volume amps by hand.




EXAMPLE USAGE:

timidity -c c:\timidity\timidity.cfg -OM -V 2 -idt foo.mod

This will try to read in foo.m2m as the config file for the conversion, and
will output a file called foo.mid.  -V 2 tells it to generate midi for use
on a device that uses an X^2 volume curve (all GM/GS/XG hardware).  You can
use the -o flag to specify any other output name you wish.

If you don't have timidity installed, so that you don't have a valid
timidity.cfg file, just create a 0 byte file and use it instead of the real
thing.  Since you're playing mod files, timidity doesn't need to load any
midi instruments, so you don't need to have a set of patches or a real
timidity.cfg file :)




BACKGROUND:

MOD files are a lot like MIDI files.  Both formats are basicly a series of
events that control how notes get played with which instruments.  MODs
package the instruments along with the events into a single file, while MIDI
relies on external sources of instruments.  It is this fundamental
difference that creates the most difficulty in performing a mod->midi file
conversion.  The mod file does not need to know what pitch each sample is
tuned to, if it is a drum, or if it is a chord.  MOD players simply play the
packaged sample at the requested pitch, assuming all samples are tuned to
the same fixed frequency, whether they actually are or not.  Thus, if you
were to do a direct mod->midi event conversion, you would wind up with midi
instruments playing in the wrong keys, snare drums being treated as normal
melodic instruments, and single notes where there should be chords. 
Transposition, drum related channel movements, and chord emissions are the
most noticable obstacles to overcome when performing an accurace mod->midi
conversoin.

Paolo Bonzini has already done half of my job for me.  He contributed a good
amount of code that turns TiMidity++ into a first rate mod player.  This,
alone, would not have helped me very much; it was how he implemented it.  
Rather than handle the events like every other mod player known to man,
TiMidity++ converts them into standard midi events, loads the mod instruments
in as special patches, and then renders them just like it would any normal
midi file.  The mod event parsing, instrument parsing, and direct event
conversion was already done!  All I had to do was handle the problems I
mentioned above, along with many more minor ones I haven't mentioned, before
writing the internal TiMidity++ events out to a midi file.  See the comments
at the top of m2m.c if you are interested in some of the other issues that
needed to be addressed during the conversion process.  Although some of
these other issues were non-trivial to deal with, and pitch bends beyond 4
octaves may still sound a bit odd, they are nothing that the average user
needs to know about or keep in mind when trying to succesfully convert a mod
file.  The only thing you need to know is that, in order to address the
conversion problems disscussed above, some information about each sample in
the mod must be specified in a config file (.m2m) associated with each mod
file.  The format of this file is given below.




M2M CONFIG FILE FORMAT:

Comment lines must begin with a #.  Blank lines (no spaces or any other
character besides a newline or carriage return) are allowed.  All other lines
must specify ALL FIVE of the fields described below.  Each field is separated
by white space.



FIELD 1: Sample Number

This is the number of the sample that you are defining information for. 
The first sample in the mod file is 1 (not zero).



FIELD 2: Bank/Program, drum flag, chord, silent flag

This field specifies several different properties of the sample.  Optional
paramaters are given surrounded by parentheses.  The format for this field
is:

(!)(bank/)program(chord)(*)

If the field begins with an exclaimation mark, ! , then no notes will be
issued for this sample.  This can be used to silence samples that you can
not assign to a general midi instrument, such as speech, complicated drum
tracks, or any sound effect that you can not create a close approximation to
using GS sfx banks.

The bank portion of the field specifies an optional bank selection.  This is
the number of the bank to use, followed by a / to separate it from the
program number.

The program number is the midi instrument you are assigning to the sample. 
If the sample is a drum, this is the note that the drum is mapped to in the
drum set.

The optional chord field specifies what type of chord the sample is composed
of.  There are 4 types of chords, each of which has 3 subtypes.  The
supported chord types are (M)ajor, (m)inor, (d)iminished minor, and (f)ifth.
Each chord is specified by the letter surrounded by parantheses in the
previous line.  The subtype of the chord describes how much the chord is
"rotated" from a standard chord, which can be 0, 1, or 2.  As an example of
what I mean by "rotated", a major chord is composed of the following note
semitone offsets: 0,4,7.  If you were to rotate the chord one to the left, it
would be: -5,0,4.  Two to the left is: -8,-5,0.  If no subtype is given,
zero rotation is assumed.

The final part specifies if the sample is a drum.  Put a * at the end of the
field to indicate this.  Chord assignments will be ignored if the drum flag
is set.

Examples:
8/48M     bank 8, program 48 (Orchestra Strings), with a normal major chord
!8/48M    silence this sample
8/48M2    same as the first example, only the chord is rotated down twice
48        normal Marcato Strings in tone bank 0
16/38*    Power drum set, Snare1
38*       Snare1 on the regular drum set 0



FIELD 3: Transposition

This is how much to transpose the original note specified in the mod file. 
If the sample is tuned at middle C (pitch 60), it will need to be transposed
+24 semitones for the midi instrument to play on the correct pitch.  Samples
marked as drums will not be transposed, since they are fixed to a single
note on the drum channel.  You must still enter a value for the
transposition field, even if it is ignored by the drums, so that the config
file parser will not crash.



FIELD 4: Fine Tuning

All pitch bend events for this sample will be adjusted by the given fraction
of a pitch.  This is sometimes necessary for highly out of tune samples.
Some MOD composers, instead of tuning their samples correctly, use pitch
bends to tune the samples.  When you play this music with correctly tuned
samples, these pitch bends detune the note and it sounds out of tune.  So the
fine tuning value is used to compensate for these detuning pitchbends.

It is also common to find out of tune samples that were NOT tuned with
pitchbends, so adding in a pitch bend adjustment would only make them sound
worse in a midi file.  To disable fine tuning, an optional ! can be placed
before the fine tuning value.  This is the DEFAULT SETTING in the automatic
config file generator.  If you find that a mod requires fine tuning for a
sample, simply delete the ! and redo the conversion.

This feature is not yet fully implemented.  Only existing pitch bend events
are affected, so no new pitch bend events are issued.  This is not usually a
problem, however, since most cases where this feature needs to be applied
involve mods that issue pitch bends before the affected notes, since they
were intended to tune the samples to begin with.  I plan to eventually
implement insertion of new pitch bend events, so that this will be a true
fine tuning feature.

 

FIELD 5: %Volume

Each sample can be amplified by scaling the expression events.  100 is the
default amount, which is 100% of the original volume.  50 would decrease it
to half of the original volume, while 150 would be 1.5 times the original
volume.  Don't forget that the maximum expression value is 127, so any
expression events that get scaled higher than this will cap off at 127 and
you won't hear any difference.  It is mainly used for quieting instruments
that are too loud in the midi file, or for amplifying instruments whoose
expression values are too low to begin with.

Any fields beyond the first 5 will not be parsed.  You can type anything
here that you want.  You do not have to place a # before comment text, but
it is conventional to do so.




FREQUENCY ANALYSIS:

So, how do you figure out how much to transpose each sample and what chord
it is?  Load it up in a program that can perform an FFT on the sample and
display the frequency peaks.  The first peak is usually, but not always, the
fundamental pitch of the sample.  If the sample is a chord, take the first 3
major peaks and assign the chord from these.  Then enter the appropriate
chord and transposition values in the .m2m file and see if it sounds correct.
It is VERY time consuming to do all of this by hand....  So, I wrote routines
to do all of the assignments for you :)  It is not 100% accurate, but it's
pretty darn close.  And when it does miss a pitch or a chord, it always
assigns it the correct LOOKING answer.  That is, if I were to visually
inspect the FFT data, I would pick the same pitch the algorithm does.  I'm
no expert at this, but after spending so many hours testing this on many
different difficult to assign pitches, I think I'm pretty good at it now :) 
The only way I can see to improve it is to build in some sort of
psychoacoustical model that takes into account how the human ear percieves
the sound.  And I don't think I want to do that at the moment....  It does an
above average job at dealing with samples that have more than one pitch or
chord in them, but don't be surprised if a noisy or multi-tonal sample
doesn't get assigned correctly.  Garbage in, garbage out :)  The automatic
assignment is very good for the vast majority of samples and should DEFINATELY
be tried first before you start changing things by hand.  When it does mess
up, it's usually only off by a single semitone or an octave multiple, so it's
easy to tweak from there.

Before I wrote the automatic frequency analysis routines, I knew very little
about the field.  Pitch detection is a very old problem in the audio signal
processing literature.  I looked up references in the library dating from
the 1960's.  The stuff from back then is just as relevant as the later
literature, since the methods really haven't improved much since then.  The
two major camps on how to do this are "autocorrelation" and "cepstrum"
analysis.  It turns out that autocorrelation was not the answer to my
problems.  While it works well on "well behaved" samples, it breaks down
very quickly on synth instruments, noisy instruments, and instruments with
multiple fundamental frequencies.  A large number of samples encountered in
mod files exhibit these properties.  No matter what I did to try to tweak it,
and I tried a lot of good things, I just could not make it robust enough to
handle real world samples.  It's a good theory, but it falls apart in
practice.

Cepstrum analysis proved to be much more robust.  But even so, I had to do a
good deal of pitch filtering and peak weighting before I could get it to work
well.  The 2nd FFT analysis kept giving me frequency peaks that didn't exist
in the 1st FFT spectrum.  They were, however, very close to real peaks.  So I
throw away all frequencies that fall below a pitch peak area and maximum
magnitude filter, then force the cepstrum analysis to only choose pitches that
have made it through the filter in the 1st FFT spectrum.  I set a maximum
frequency based on zero point crossing analysis, going out two zero crossings
from the largest amplitude in the sample.  This was necessary to prevent
octave jumping errors.  I found that it is also important to weight the
cepstrum peak areas by the maximum magnitude within the corresponding pitch
peak in the 1st FFT.  This was a desperate attempt to get some especially
troublesome bass samples to assign correctly.  Surprisingly enough, it works
great, giving me a higher success rate on all my samples without inducing any
new misassignments!  The only catch is that the weighting only works well for
< 2 seconds of audio analysis.  Any larger than that and the FFT size gets
so big that the pitch peaks are too diffused, so the maximum magnitudes for
the pitches are too small, and the weighting starts to give wrong answers. 
If anyone wants to analyze >= 2 seconds of data, which isn't neccessary for
assigning pitches to mod/midi instruments, it would be easy to implement a
sliding window average that calls the existing frequency assignment
function.

It appears to work better than any of the other sample analysis software I
have.  If you are interested in more details of how I did the cepstrum
analysis, try looking over the code in freq.c and/or email me for a more
complete description of the algorithm I wound up with.  The new FFT routines
are not mine, but are public domain.  From all the benchmarks I could find,
this is the best FFT implementation for doing what I need to do (and for
future effects processing, should they ever be added to TiMidity++).  See
fft4g.c for info on where to get the original FFT package.




SUGGESTIONS ?:

Feel free to email me with any suggestions you may have on how I can do a
better job of converting the mods, or how I can implement things on the TODO
or WISH lists in m2m.c.  I am considering turning this into a stand alone
program, but until I get more free time and energy, it's going to stay as
just an addon for TiMidity++.




LEGAL STUFF:

TiMidity++ is distributed under the GPL, and since my code is derived from
and makes use of it, I guess it's under the GPL too.  So blah blah blah,
legal stuff, blah blah blah, etc..  You know the drill.




Eric A. Welsh <ewelsh@ccb.wustl.edu>
Center for Molecular Design
Center for Computational Biology
Washington University
St. Louis, MO
