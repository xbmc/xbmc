======================================================================
MIDI Tuning Standard summary
======================================================================

Besides GS/XG scale tuning which adjusts the pitch of 12 tones in an
octave individually, TiMidity++ supports MIDI Tuning Standard in
Universal SysEx.  MIDI Tuning Standard has the following advantages
compared with GS/XG scale tuning:

  - Support for microtonal sound other than 12 tones
  - The pitch can be adjusted in 1/100 cent or less accuracy
  - Temperaments can be rationally setup based on the tonality

For details, please refer to the recommended practice.

(1) Bulk Tuning Dump Request (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 00 tt F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
00		sub-ID#2 = "bulk tuning dump request (Non Real-Time)"
tt		tuning program number (0 - 127)
F7		EOX
----------------------------------------------------------------------

(2) Bulk Tuning Dump (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 01 tt <tuning name> [xx yy zz] ... chksum F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
01		sub-ID#2 = "bulk tuning dump (Non Real-Time)"
tt		tuning program number (0 - 127)
<tuning name>	16 ASCII characters
[xx yy zz]	frequency data for one note (repeated 128 times)
chksum		checksum (XOR of all bytes excluding F0, F7, and chksum)
F7		EOX
----------------------------------------------------------------------

(3) Single Note Tuning Change (Real-Time)
----------------------------------------------------------------------
F0 7F <device ID> 08 02 tt ll [kk xx yy zz] F7

F0 7F		Universal Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
02		sub-ID#2 = "single note tuning change (Real-Time)"
tt		tuning program number (0 - 127)
ll		number of changes (1 change = 1 set of [kk xx yy zz])
[kk		MIDI key number
 xx yy zz]	frequency data for that key (repeated 'll' number of times)
F7		EOX
----------------------------------------------------------------------

(4) Bulk Tuning Dump Request (Bank) (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 03 bb tt F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI tuning standard"
03		sub-ID#2 = "bulk tuning dump request (Bank) (Non Real-Time)"
bb		tuning bank number (0 - 127)
		(described as 1-128 in MIDI Tuning Specification)
tt		tuning program number (0 - 127)
F7		EOX
----------------------------------------------------------------------

(5) Key-Based Tuning Dump (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 04 bb tt <tuning name> [xx yy zz] ... chksum F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI tuning standard"
04		sub-ID#2 = "key-based tuning dump (Non Real-Time)"
bb		tuning bank number (0 - 127)
		(described as 1-128 in MIDI Tuning Specification)
tt		tuning program number (0 - 127)
<tuning name>	16 ASCII characters
[xx yy zz]	frequency data for one note (repeated 128 times)
chksum		checksum (XOR of all bytes excluding F0, F7, and chksum)
F7		EOX
----------------------------------------------------------------------

(6) Scale/Octave Tuning Dump 1-Byte Form (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 05 bb tt <tuning name> [xx] ... chksum F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI tuning standard"
05		sub-ID#2 = "scale/octave tuning dump 1-byte form
				(Non Real-Time)"
bb		tuning bank number (0 - 127)
		(described as 1-128 in MIDI Tuning Specification)
tt		tuning program number (0 - 127)
<tuning name>	16 ASCII characters
[xx]		frequency data for C,C#,... B (12 bytes total)
			00H means -64 Cent
			40H means +/- 0 Cent
			7FH means +63 Cent
chksum		checksum (XOR of all bytes excluding F0, F7, and chksum)
F7		EOX
----------------------------------------------------------------------

(7) Scale/Octave Tuning Dump 2-Byte Form (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 06 bb tt <tuning name> [xx yy] ... chksum F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI tuning standard"
06		sub-ID#2 = "scale/octave tuning dump 2-byte form
				(Non Real-Time)"
bb		tuning bank number (0 - 127)
		(described as 1-128 in MIDI Tuning Specification)
tt		tuning program number (0 - 127)
<tuning name>	16 ASCII characters
[xx yy]		frequency data for C,C#,... B (24 bytes total)
			00H 00H means -100 cents (8,192 steps of .012207 cents)
			40H 00H means 0 cents (equal temperament)
			7FH 7FH means +100 cents (8,191 steps of .012207 cents)
chksum		checksum (XOR of all bytes excluding F0, F7, and chksum)
F7		EOX
----------------------------------------------------------------------

(8) Single Note Tuning Change (Bank) (Real-Time)
----------------------------------------------------------------------
F0 7F <device ID> 08 07 bb tt ll [kk xx yy zz] ... F7

F0 7F		Universal Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI tuning standard"
07		sub-ID#2 = "single note tuning change (Bank) (Real-Time)"
bb		tuning bank number (0 - 127)
		(described as 1-128 in MIDI Tuning Specification)
tt		tuning program number (0 - 127)
ll		number of changes (1 change = 1 set of [kk xx yy zz])
[kk		MIDI key number
 xx yy zz]	frequency data for that key (repeated 'll' number of times)
F7		EOX
----------------------------------------------------------------------

(9) Single Note Tuning Change (Bank) (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 07 bb tt ll [kk xx yy zz] ... F7

F0 7E		Universal Non-Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI tuning standard"
07		sub-ID#2 = "single note tuning change (Bank) (Non Real-Time)"
bb		tuning bank number (0 - 127)
		(described as 1-128 in MIDI Tuning Specification)
tt		tuning program number (0 - 127)
ll		number of changes (1 change = 1 set of [kk xx yy zz])
[kk		MIDI key number
 xx yy zz]	frequency data for that key (repeated 'll'number of times)
F7		EOX
----------------------------------------------------------------------

(10) Scale/Octave Tuning 1-Byte Form (Real-Time)
----------------------------------------------------------------------
F0 7F <device ID> 08 08 ff gg hh [ss] ... F7

F0 7F		Universal Real-Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
08		sub-ID#2 = "scale/octave tuning 1-byte form (Real-Time)"
ff		channel/options byte 1
			bits 0 to 1 = channel 15 to 16
			bits 2 to 6 = reserved for future expansion
gg		channel byte 2 - bits 0 to 6 = channel 8 to 14
hh		channel byte 3 - bits 0 to 6 = channel 1 to 7
[ss]		12 byte tuning offset of 12 semitones from C to B
			00H means -64 cents
			40H means 0 cents (equal temperament)
			7FH means +63 cents
F7		EOX
----------------------------------------------------------------------

(11) Scale/Octave Tuning 1-Byte Form (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 08 ff gg hh [ss] ... F7

F0 7E		Universal Non Real-Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
08		sub-ID#2 = "scale/octave tuning 1-byte form (Non Real-Time)"
ff		channel/options byte 1
			bits 0 to 1 = channel 15 to 16
			bits 2 to 6 = reserved for future expansion
gg		channel byte 2 - bits 0 to 6 = channel 8 to 14
hh		channel byte 3 - bits 0 to 6 = channel 1 to 7
[ss]		12 byte tuning offset of 12 semitones from C to B
			00H means -64 cents
			40H means 0 cents (equal temperament)
			7FH means +63 cents
F7		EOX
----------------------------------------------------------------------

(12) Scale/Octave Tuning 2-Byte Form (Real-Time)
----------------------------------------------------------------------
F0 7F <device ID> 08 09 ff gg hh [ss tt] ... F7

F0 7F		Universal Real-Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
09		sub-ID#2 = "scale/octave tuning 2-byte form (Real-Time)"
ff		channel/options byte 1
			bits 0 to 1 = channel 15 to 16
			bits 2 to 6 = reserved for future expansion
gg		channel byte 2 - bits 0 to 6 = channel 8 to 14
hh		channel byte 3 - bits 0 to 6 = channel 1 to 7
[ss tt]		24 byte tuning offset of 2 bytes per semitone from C to B
			00H 00H means -100 cents (8,192 steps of .012207 cents)
			40H 00H means 0 cents (equal temperament)
			7FH 7FH means +100 cents (8,191 steps of .012207 cents)
F7		EOX
----------------------------------------------------------------------

(13) Scale/Octave Tuning 2-Byte Form (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 09 ff gg hh [ss tt] ... F7

F0 7E		Universal Non Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
09		sub-ID#2 = "scale/octave tuning 2-byte form (Non Real-Time)"
ff		channel/options byte 1
			bits 0 to 1 = channel 15 to 16
			bits 2 to 6 = reserved for future expansion
gg		channel byte 2 - bits 0 to 6 = channel 8 to 14
hh		channel byte 3 - bits 0 to 6 = channel 1 to 7
[ss tt]		24 byte tuning offset of 2 bytes per semitone from C to B
			00H 00H means -100 cents (8,192 steps of .012207 cents)
			40H 00H means 0 cents (equal temperament)
			7FH 7FH means +100 cents (8,191 steps of .012207 cents)
F7		EOX
----------------------------------------------------------------------

(14) Temperament Tonality Control Tuning (Real-Time)
----------------------------------------------------------------------
F0 7F <device ID> 08 0A sf mi F7

F0 7F		Universal Real-Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
0A		sub-ID#2 = "temperament tonality control tuning
				(Real-Time)"
sf		number of sharp/flat (1 byte)
			39H means 7 flats
			3FH means 1 flat
			40H means key of C
			41H means 1 sharp
			47H means 7 sharps
mi		major/minor (1 byte)
			00H means major key
			01H means minor key
			02H means passing major key
			03H means passing minor key
F7		EOX
----------------------------------------------------------------------

(15) Temperament Tonality Control Tuning (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 0A sf mi F7

F0 7E		Universal Non Real-Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
0A		sub-ID#2 = "temperament tonality control tuning
				(Non Real-Time)"
sf		number of sharp/flat (1 byte)
			39H means 7 flats
			3FH means 1 flat
			40H means key of C
			41H means 1 sharp
			47H means 7 sharps
mi		major/minor (1 byte)
			00H means major key
			01H means minor key
			02H means passing major key
			03H means passing minor key
F7		EOX
----------------------------------------------------------------------

(16) Temperament Type Control Tuning (Real-Time)
----------------------------------------------------------------------
F0 7F <device ID> 08 0B ff gg hh tt F7

F0 7F		Universal Real-Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
0B		sub-ID#2 = "temperament type control tuning (Real-Time)"
ff		channel/options byte 1
			bits 0 to 1 = channel 15 to 16
			bit 2 = port A/B
			bits 3 to 6 = reserved for future expansion
gg		channel byte 2 - bits 0 to 6 = channel 8 to 14
hh		channel byte 3 - bits 0 to 6 = channel 1 to 7
tt		temperament type (1 byte)
			00H means equal temperament
			01H means Pythagoras tuning
			02H means mean-tone tuning
			03H means pure intonation
			40H means user-defined temperament #0
			41H means user-defined temperament #1
			42H means user-defined temperament #2
			43H means user-defined temperament #3
F7		EOX
----------------------------------------------------------------------

(17) Temperament Type Control Tuning (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 0B ff gg hh tt F7

F0 7E		Universal Non Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
0B		sub-ID#2 = "temperament type control tuning (Non Real-Time)"
ff		channel/options byte 1
			bits 0 to 1 = channel 15 to 16
			bit 2 = port A/B
			bits 3 to 6 = reserved for future expansion
gg		channel byte 2 - bits 0 to 6 = channel 8 to 14
hh		channel byte 3 - bits 0 to 6 = channel 1 to 7
tt		temperament type (1 byte)
			00H means equal temperament
			01H means Pythagoras tuning
			02H means mean-tone tuning
			03H means pure intonation
			40H means user-defined temperament #0
			41H means user-defined temperament #1
			42H means user-defined temperament #2
			43H means user-defined temperament #3
F7		EOX
----------------------------------------------------------------------

(18) User-defined Temperament Entry (Non Real-Time)
----------------------------------------------------------------------
F0 7E <device ID> 08 0C tt <temper name>
		ll [fh fl bh bl aa bb cc dd ee ff] ... F7

F0 7E		Universal Non Real Time SysEx header
<device ID>	ID of target device (7F = all devices)
08		sub-ID#1 = "MIDI Tuning Standard"
0C		sub-ID#2 = "user-defined temperament entry (Non Real-Time)"
tt		temperament program number (0 - 63)
<temper name>	16 ASCII characters
ll		number of formula (1 formula =
				1 set of [fh fl bh bl aa bb cc dd ee ff])
[fh		applying pitch bit mask byte 1
			bits 0 to 3 = circle of fifth forward 8 to 11
			bits 4 to 5 = reserved for future expansion
			bit 6 = major flag (reversal)
 fl		applying pitch bit mask byte 2
			bits 0 to 6 = circle of fifth forward 1 to 7
 bh		applying pitch bit mask byte 3
			bits 0 to 3 = circle of fifth backward 8 to 11
			bits 4 to 5 = reserved for future expansion
			bit 6 = minor flag (reversal)
 bl		applying pitch bit mask byte 4
			bits 0 to 6 = circle of fifth backward 1 to 7
 aa bb		fraction (aa/bb)
 cc dd ee ff]	power ((cc/dd)^(ee/ff)) (repeated 'll' number of times)
F7		EOX
----------------------------------------------------------------------

======================================================================
The major/minor in the temperament tonality
======================================================================

The basic chords used in general music in C major are not only C, G, F
but also Am, Em, Dm which appear frequently.  There may also be Cm,
Gm, Fm, A, E, D, and so on.  Since these chords are not supported only
in pure intonation (C major), players need to change temperaments
according to progress of music.

To solve the issue, TiMidity++ prepares

(1) pure intonation (C major)
	based on the pitch of C in Pythagoras tuning (C major)
(2) pure intonation (A minor)
	based on the pitch of A in Pythagoras tuning (A minor)
(3) pure intonation (passing C major)
	based on the pitch of A in Pythagoras tuning (C major)
(4) pure intonation (passing A minor)
	based on the pitch of C in Pythagoras tuning (A minor)

I will explain more precisely.  The following table gives the lattice
(Cartesian model) of the scale system:

-----------------------------------------------------------------------------
D--   A--   E--   B--   F#--  C#--  G#--  D#--  A#--  E#--  B#--  F##-- C##--
Bb-   F-    C-    G-    D-    A-    E-    B-    F#-   C#-   G#-   D#-   A#-  
Gb    Db    Ab    Eb    Bb    F     C     G     D     A     E     B     F#   
Ebb+  Bbb+  Fb+   Cb+   Gb+   Db+   Ab+   Eb+   Bb+   F+    C+    G+    D+   
Cbb++ Gbb++ Dbb++ Abb++ Ebb++ Bbb++ Fb++  Cb++  Gb++  Db++  Ab++  Eb++  Bb++ 
-----------------------------------------------------------------------------

The notation "ABCDEFG" is according to Pythagoras tuning.  The
notation "+", "-", "++" and "--" mean 1sc higher, 1sc lower, 2sc
higher and 2sc lower respectively.

A certain pure intonation is given as 12 sounds arranged by the
rectangle of 4x3 from the lattice.  For example, C tuning, A tuning,
A- tuning and C+ tuning are given as following tables respectively:

[C tuning (C major)]
----------------------
A-    E-    B-    F#- 
F     C     G     D   
Db+   Ab+   Eb+   Bb+ 
----------------------

[A tuning (A minor)]
----------------------
F#-   C#-   G#-   D#- 
D     A     E     B   
Bb+   F+    C+    G+  
----------------------

[C+ tuning (passing C major)]
----------------------
A     E     B     F#  
F+    C+    G+    D+  
Db++  Ab++  Eb++  Bb++
----------------------

[A- tuning (passing A minor)]
----------------------
F#--  C#--  G#--  D#--
D-    A-    E-    B-  
Bb    F     C     G   
----------------------

I think it is nice to select the tuning combination whose pitch of
parallel key is slightly lower for major music, and slightly higher
for minor music.

======================================================================
Preset temperament of Temperament Type Control Tuning
======================================================================

First, Pythagoras tuning (major) chromatic scale is expressed by the
following recurrence relations.  Here, the index [] is a offset of the
tonic.  The operation results are surely settled between 1 and 2, so
they will be made into half or double if necessary.

[Pythagoras tuning (major) chromatic scale]
pytha_maj[ 0] = 1                       # C  1
pytha_maj[ 7] = pytha_maj[ 0] * 3/2     # G  3/2
pytha_maj[ 2] = pytha_maj[ 7] * 3/2     # D  9/8
pytha_maj[ 9] = pytha_maj[ 2] * 3/2     # A  27/16
pytha_maj[ 4] = pytha_maj[ 9] * 3/2     # E  81/64
pytha_maj[11] = pytha_maj[ 4] * 3/2     # B  243/128
pytha_maj[ 6] = pytha_maj[11] * 3/2     # F# 729/512
--
pytha_maj[ 5] = pytha_maj[ 0] * 2/3     # F  4/3
pytha_maj[10] = pytha_maj[ 5] * 2/3     # Bb 16/9
pytha_maj[ 3] = pytha_maj[10] * 2/3     # Eb 32/27
pytha_maj[ 8] = pytha_maj[ 3] * 2/3     # Ab 128/81
pytha_maj[ 1] = pytha_maj[ 8] * 2/3     # Db 256/243

On the other hand, pure intonation (major) chromatic scale can be
expressed by the following recurrence relations.  Here, sc means a
syntonic comma (81/80).

[pure intonation (major) chromatic scale]
pure_maj[ 0] = 1                        # C  1
pure_maj[ 7] = pure_maj[ 0] * 3/2       # G  3/2
pure_maj[ 2] = pure_maj[ 7] * 3/2       # D  9/8
pure_maj[ 9] = pure_maj[ 2] * 3/2 / sc  # A  5/3
pure_maj[ 4] = pure_maj[ 9] * 3/2       # E  5/4
pure_maj[11] = pure_maj[ 4] * 3/2       # B  15/8
pure_maj[ 6] = pure_maj[11] * 3/2       # F# 45/32
--
pure_maj[ 5] = pure_maj[ 0] * 2/3       # F  4/3
pure_maj[10] = pure_maj[ 5] * 2/3 * sc  # Bb 9/5
pure_maj[ 3] = pure_maj[10] * 2/3       # Eb 6/5
pure_maj[ 8] = pure_maj[ 3] * 2/3       # Ab 8/5
pure_maj[ 1] = pure_maj[ 8] * 2/3       # Db 16/15

It can be understood that pure intonation is similar to Pythagoras
tuning fundamentally except descending with 1sc at A and rising with
1sc at B flat while going up and down respectively from the tonic in
the circle of fifths.

Similarly, expressed Pythagoras tuning and pure intonation (minor)
chromatic scale by the following recurrence relations.  Although the
fractions written to right-hand side is terrible values, the
recurrence relations themselves are very simple.

[Pythagoras tuning (minor) chromatic scale]
pytha_min[ 0] = 1                       # C  1
pytha_min[ 7] = pytha_min[ 0] * 3/2     # G  3/2
pytha_min[ 2] = pytha_min[ 7] * 3/2     # D  9/8
pytha_min[ 9] = pytha_min[ 2] * 3/2     # A  27/16
pytha_min[ 4] = pytha_min[ 9] * 3/2     # E  81/64
pytha_min[11] = pytha_min[ 4] * 3/2     # B  243/128
pytha_min[ 6] = pytha_min[11] * 3/2     # F# 729/512
pytha_min[ 1] = pytha_min[ 6] * 3/2     # C# 2187/2048
pytha_min[ 8] = pytha_min[ 1] * 3/2     # G# 6561/4096
pytha_min[ 3] = pytha_min[ 8] * 3/2     # D# 19683/16384
--
pytha_min[ 5] = pytha_min[ 0] * 2/3     # F  4/3
pytha_min[10] = pytha_min[ 5] * 2/3     # Bb 16/9

[pure intonation (minor) chromatic scale]
pure_min[ 0] = 1                  * sc  # C  1     * sc
pure_min[ 7] = pure_min[ 0] * 3/2       # G  3/2   * sc
pure_min[ 2] = pure_min[ 7] * 3/2 / sc  # D  10/9  * sc
pure_min[ 9] = pure_min[ 2] * 3/2       # A  5/3   * sc
pure_min[ 4] = pure_min[ 9] * 3/2       # E  5/4   * sc
pure_min[11] = pure_min[ 4] * 3/2       # B  15/8  * sc
pure_min[ 6] = pure_min[11] * 3/2 / sc  # F# 25/18 * sc
pure_min[ 1] = pure_min[ 6] * 3/2       # C# 25/24 * sc
pure_min[ 8] = pure_min[ 1] * 3/2       # G# 25/16 * sc
pure_min[ 3] = pure_min[ 8] * 3/2       # D# 75/64 * sc
--
pure_min[ 5] = pure_min[ 0] * 2/3       # F  4/3   * sc
pure_min[10] = pure_min[ 5] * 2/3       # Bb 16/9  * sc

The differences from the major tuning are that the boundary of
Pythagoras tuning goes up three positions, that the positions of
descending with syntonic comma are changed, and that pure intonation
is adjusted 1sc higher so that melodic parts' tonic (Pythagoras
tuning) and harmonic parts' tonic (pure intonation) are overlapped.

By the way, mean-tone tuning is also prepared besides Pythagoras
tuning and pure intonation as preset temperament of TiMidity++.  While
mean-tone tuning (major) is based on the general one whose major
thirds are pure, mean-tone tuning (minor) is based on Salinas tuning
whose minor thirds are pure.  Both mean-tone tuning (major) chromatic
scale and mean-tone tuning (minor) chromatic scale can be expressed by
the following recurrence relations.

[mean-tone tuning (major) chromatic scale]
mt_maj[ 0] = 1                          # C  1
mt_maj[ 7] = mt_maj[ 0] * 5^(1/4)       # G  5^(1/4)
mt_maj[ 2] = mt_maj[ 7] * 5^(1/4)       # D  5^(1/2) / 2
mt_maj[ 9] = mt_maj[ 2] * 5^(1/4)       # A  5^(3/4) / 2
mt_maj[ 4] = mt_maj[ 9] * 5^(1/4)       # E  5/4
mt_maj[11] = mt_maj[ 4] * 5^(1/4)       # B  5^(5/4) / 4
mt_maj[ 6] = mt_maj[11] * 5^(1/4)       # F# 5^(3/2) / 8
--
mt_maj[ 5] = mt_maj[ 0] / 5^(1/4)       # F  2 / 5^(1/4)
mt_maj[10] = mt_maj[ 5] / 5^(1/4)       # Bb 4 / 5^(1/2)
mt_maj[ 3] = mt_maj[10] / 5^(1/4)       # Eb 4 / 5^(3/4)
mt_maj[ 8] = mt_maj[ 3] / 5^(1/4)       # Ab 8/5
mt_maj[ 1] = mt_maj[ 8] / 5^(1/4)       # Db 8 / 5^(5/4)

[mean-tone tuning (minor) chromatic scale]
mt_min[ 0] = 1          * sc            # C  1                 * sc
mt_min[ 7] = mt_min[ 0] * (10/3)^(1/3)  # G  (10/3)^(1/3)      * sc
mt_min[ 2] = mt_min[ 7] * (10/3)^(1/3)  # D  (10/3)^(2/3) /  2 * sc
mt_min[ 9] = mt_min[ 2] * (10/3)^(1/3)  # A  5/3               * sc
mt_min[ 4] = mt_min[ 9] * (10/3)^(1/3)  # E  (10/3)^(4/3) /  4 * sc
mt_min[11] = mt_min[ 4] * (10/3)^(1/3)  # B  (10/3)^(5/3) /  4 * sc
mt_min[ 6] = mt_min[11] * (10/3)^(1/3)  # F# 25/18             * sc
mt_min[ 1] = mt_min[ 6] * (10/3)^(1/3)  # C# (10/3)^(7/3) / 16 * sc
mt_min[ 8] = mt_min[ 1] * (10/3)^(1/3)  # G# (10/3)^(8/3) / 16 * sc
mt_min[ 3] = mt_min[ 8] * (10/3)^(1/3)  # D# 125/108           * sc
--
mt_min[ 5] = mt_min[ 0] / (10/3)^(1/3)  # F  2 / (10/3)^(1/3)  * sc
mt_min[10] = mt_min[ 5] / (10/3)^(1/3)  # Bb 4 / (10/3)^(2/3)  * sc

The point that the boundary of mean-tone tuning goes up three
positions, and that mean-tone tuning is adjusted 1sc higher, are the
same situation as Pythagoras tuning and pure intonation.

Now, I think that mean-tone tuning could use for a harmony-melody
because of the characteristic that is more harmony-like than
Pythagoras tuning, and a scale is not uneven like pure intonation.

======================================================================
User-defined temperament entry
======================================================================

The function of user-defined temperament entry (experimental) is
implemented in TiMidity++.  This corresponds to (18) of MIDI Tuning
Standard summary (see the top of this document).  For example, it can
generate various temperaments by the following SysEx's.

[equal temperament]
f0 7e 00 08 0c 00                                   ; temper prog number
65 71 75 61 6c 00 00 00 00 00 00 00 00 00 00 00     ; "equal"
01                                                  ; number of formula
0f 7f 00 00 01 01 02 01 07 0c                       ; (both) 2^(7/12)
f7

[Pythagoras tuning]
f0 7e 00 08 0c 01                                   ; temper prog number
50 79 74 68 61 67 6f 72 61 73 00 00 00 00 00 00     ; "Pythagoras"
02                                                  ; number of formula
00 3f 40 1f 03 02 01 01 00 01                       ; (maj) 3/2
43 7f 00 03 03 02 01 01 00 01                       ; (min) 3/2
f7

[mean-tone tuning]
f0 7e 00 08 0c 02                                   ; temper prog number
6d 65 61 6e 2d 74 6f 6e 65 00 00 00 00 00 00 00     ; "mean-tone"
02                                                  ; number of formula
00 3f 40 1f 01 01 05 01 01 04                       ; (maj) 5^(1/4)
43 7f 00 03 01 01 0a 03 01 03                       ; (min) (10/3)^(1/3)
f7

[pure intonation]
f0 7e 00 08 0c 03                                   ; temper prog number
70 75 72 65 20 69 6e 74 6f 6e 61 74 69 6f 6e 00     ; "pure intonation"
04                                                  ; number of formula
00 3f 40 1f 03 02 01 01 00 01                       ; (maj) 3/2
00 04 40 02 05 01 02 03 04 01                       ; (maj) 5*(2/3)^4
43 7f 00 03 03 02 01 01 00 01                       ; (min) 3/2
40 22 00 00 05 01 02 03 04 01                       ; (min) 5*(2/3)^4
f7

[Kirnberger-3]
f0 7e 00 08 0c 00                                   ; temper prog number
4b 69 72 6e 62 65 72 67 65 72 2d 33 00 00 00 00     ; "Kirnberger-3"
02                                                  ; number of formula
00 0f 00 00 01 01 05 01 01 04                       ; (both) 5^(1/4)
00 30 00 1f 03 02 01 01 00 01                       ; (both) 3/2
f7

[Hirashima temperament]
f0 7e 00 08 0c 01                                   ; temper prog number
48 69 72 61 73 68 69 6d 61 00 00 00 00 00 00 00     ; "Hirashima"
02                                                  ; number of formula
00 1f 00 03 01 01 05 01 01 04                       ; (both) 5^(1/4)
00 00 00 3c 03 02 01 01 00 01                       ; (both) 3/2
f7

[Werckmeister-3]
f0 7e 00 08 0c 02                                   ; temper prog number
57 65 72 63 6b 6d 65 69 73 74 65 72 2d 33 00 00     ; "Werckmeister-3"
02                                                  ; number of formula
00 07 00 00 01 09 02 01 0f 04                       ; (both) 2^(15/4)/9
00 18 00 3f 03 02 01 01 00 01                       ; (both) 3/2
f7

[well-temperament]
f0 7e 00 08 0c 03                                   ; temper prog number
77 65 6c 6c 2d 74 65 6d 70 65 72 00 00 00 00 00     ; "well-temper"
02                                                  ; number of formula
00 07 00 00 01 09 02 01 0f 04                       ; (both) 2^(15/4)/9
00 00 01 7f 03 02 01 01 00 01                       ; (both) 3/2
f7

----
TAMUKI Shoichi <tamuki@linet.gr.jp>
