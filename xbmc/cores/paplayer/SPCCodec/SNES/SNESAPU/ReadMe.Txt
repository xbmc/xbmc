Super Nintendo Entertainment System(tm) Audio Processor Unit Emulator
Dynamically Linking Library version 3.0
==============================================================================

This is the source code to SNESAPU.DLL, a Super NES APU emulator written for
IA-32 platforms.

NOTE: The SPC700 emulator was rewritten for v3.0.  This version of SNESAPU is
      NOT backward compatible with versions older than 3.0.

SNESAPU is the most accurate emulator available.  Its mixing pipeline is
almost flawless (a few aspects are not precisely emulated due to bugs in the
hardware DSP microcode), and under some circumstances produces output
identical to the original hardware, bit for bit.



Using the source in your own programs
-------------------------------------

NOTE: When distributing SNESAPU.DLL with your own programs, you should check
      the SNESAPU.DLL version at run-time to make sure it's compatible with
      the version your code is compiled for.  This should be done to prevent
      users from switching your distributed copy of SNESAPU.DLL with another
      build that may be incompatible.  See the section Version Information
      below.

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 2.1 of the License, or (at your option)
any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License along
with this library; if not, write to the
Free Software Foundation, Inc.,
59 Temple Place, Suite 330,
Boston, MA  02111-1307  USA



Files
-----

All of the emulation code was written in assembly for NASM v0.98.38 and
has been successfully compiled and executed under both Linux and Windows.

The C++ code is used to create the DLL interface for 32-bit Windows platforms
and is only necessary if you're building SNESAPU.DLL.  The SNESAPU project was
created with Visual C++ v6.0 SP6 with the output set to compile into the
C:\Windows\system32 directory.  You will need to change the custom build steps
in the project to correctly locate nasmw.exe.

All assembly source was written with tab stops set to three characters.
The C source was written with tab stops set to four characters.

NOTES: Because of alignment issues with Win32 object files, extra padding may
       need to be inserted into the DSP emulator.  If the BSS is not page
       aligned, a debug breakpoint will halt execution during initialization.
       See line 236 in DSP.Asm for more information.

       The functions in SNESAPU.DLL are NOT programmed to be thread safe.

SNESAPU - Main program
SPC700  - Executes programs written for the SPC700
DSP     - Performs all mixing and output
APU     - Groups the SPC700 and DSP together and adds extra functionality



Version Information
-------------------

Use the Windows function VerQueryValue() to retrieve a pointer to the
VS_FIXEDFILEINFO structure.  The example projects show how to do this.

The FileVersion fields will contain the current version of SNESAPU, while
the ProductVersion fields will contain the oldest version this bulid of
SNESAPU is backward compatible with.

The product version only increases when changes are made to the SNESAPU
interface.  Additional emulation options or changes made to internal data
structures do not affect the interface.  i.e. if a new interpolation function
is added, future versions will remain backward compatible because SetAPUOpt()
ignores invalid options.

The four version fields are used as follows:
    version major, 0 to 99
    version minor, 0 to 99
    sub version, 0 to 26 (If this value is != 0, add 0x60 to get the letter)
    build number, 0 to 255

For example, if dwFileVersionMS == 0x2000A and dwFileVersionLS == 0x70005,
the version number is 2.10g build 5.



Implementation
--------------

I wanted to create an emulator that had versitilty and was easy to use.
Since those two objectives fall on opposite ends of the spectrum, it was hard
to find a middle ground.  I made the interface as easy as I could while still
retaining the functionality I wanted.

I'll explain how to create a basic SPC player.  If you want to add additional
features, you'll have to look at the header files for descriptions of the
functions.

First, you need to link the DLL.  If you include SNESAPU.h and SNESAPU.lib in
your project, your program should automatically load the DLL at run-time.
Otherwise, you'll need to use the LoadLibrary and GetProcAddress functions.

Upon linking the DLL, the emulator will be initialized.  The SPC700 will be
in a a state identical to powerup in the Super NES, and the DSP will be set
to return 16-bit stereo output at 32kHz.  You can begin emulation at this
point, though all you will hear is silence as the SPC700 waits in a loop to
receive data.

Allocate a buffer of 66048 bytes.  Read an SPC file into this buffer, then
pass a pointer to the buffer to LoadSPCFile().  This function will reset the
APU state and copy the necessary data from the file.

To emulate the APU, call EmuAPU().  Emulation can be based on clock cycles
or samples.

Keep track of how much time has gone by by calling GetSPCTime().  One second
is equal to 64,000 counter increases.  The counter is set to 0 by ResetAPU().

Summary:

1) Link the DLL to initialize the emulator
2) Call LoadSPCFile() to copy the SPC into the APU
3) Call EmuAPU() to generate audio output
4) Go to step 3 until you wish to stop
5) To load another song, goto step 2
6) Unlink the DLL



DSP Registers
-------------

The DSPRAM structure may look a bit intimidating, but it actually correctly
maps to the DSP registers and is quite easy to use.  Here are some examples
of how to access the registers:

Find out the instrument playing on voice 3 -

i = dsp.voice[3].srcn;


Find out the delay of the echo in milliseconds -

i = dsp.edl << 4;


Find out if a song uses echo filtering -

i = dsp.fir[0].c;
if (i == 0x7F) i = 0;

for (j=1; j<8; j++)
    i |= dsp.fir[j].c;

if (i) filter = true;



Known Emulation Inaccuracies
----------------------------

- Songs from the game Vortex to not play correctly

- 16-bit memory accesses do not wrap around
  Instructions like MOV YA,0FFh or JMP [!0FFFFh+X] (where X = 0) will load the
  second byte from the next contiguous location in physical RAM instead of
  wrapping around an 8- or 16-bit memory address.  However, memory address
  calculations correctly wrap around, i.e. MOV A,0F0h+X (where X = 11h) will
  properly load the byte at DP 01h.

- Read checking is not performed on address calculations, i.e. MOV A,[0FDh]+Y
  (where Y = 0) will not clear counters 0 and 1.

- Read checking is not performed on AND1 when C = 0



Thanks
------

The SNESAPU.DLL would not exist without these people:

Datschge, CaitSith, Gary Henderson, Marius Fodor, nZero, Nitro, Sycraft, and
the contributors to SNESmusic.org who keep finding all of the bugs

SNESAPU is copyright (C)2001-2006 Alpha-II Productions (www.alpha-ii.com)

"Super NES" and "Super Nintendo Entertainment System" are registered
trademarks of Nintendo (www.nintendo.com)