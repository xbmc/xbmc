/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * readme.txt - General information on DUMB.          / / \  \
 *                                                   | <  /   \_
 *                                                   |  \/ /\   /
 *                                                    \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */


********************
*** Introduction ***
********************


Thank you for downloading DUMB v0.9.3! You should have the following
documentation:

   readme.txt    - This file
   licence.txt   - Conditions for the use of this software
   release.txt   - Release notes and changes for this and past releases
   docs/
     howto.txt   - Step-by-step instructions on adding DUMB to your project
     faq.txt     - Frequently asked questions and answers to them
     dumb.txt    - DUMB library reference
     deprec.txt  - Information about deprecated parts of the API
     ptr.txt     - Quick introduction to pointers for those who need it
     fnptr.txt   - Explanation of function pointers for those who need it
     modplug.txt - Our official position regarding ModPlug Tracker

This file will help you get DUMB set up. If you have not yet done so, please
read licence.txt and release.txt before proceeding. After you've got DUMB set
up, please refer to the files in the docs/ directory at your convenience. I
recommend you start with howto.txt.


****************
*** Features ***
****************


Here is the statutory feature list:

- Freeware

- Supports playback of IT, XM, S3M and MOD files

- Faithful to the original trackers, especially IT; if it plays your module
  wrongly, please tell me so I can fix the bug! (But please don't complain
  about differences between DUMB and ModPlug Tracker; see docs/modplug.txt)

- Accurate support for low-pass resonant filters for IT files

- Very accurate timing and pitching; completely deterministic playback

- Click removal

- Facility to embed music files in other files (e.g. Allegro datafiles)

- Three resampling quality settings: aliasing, linear interpolation and cubic
  interpolation

- Number of samples playing at once can be limited to reduce processor usage,
  but samples will come back in when other louder ones stop

- All notes will be present and correct even if you start a piece of music in
  the middle

- Option to take longer loading but seek fast to any point before the music
  first loops (seeking time increases beyond this point)

- Audio generated can be used in any way; DUMB does not necessarily send it
  straight to a sound output system

- Can be used with Allegro, can be used without (if you'd like to help make
  DUMB more approachable to people who aren't using Allegro, please contact
  me)

- Makefile provided for DJGPP, MinGW, Linux, BeOS and Mac OS X

- Project files provided for MSVC 6

- Autotools-based configure script available as a separate download for
  masochists

- Code should port anywhere that has a 32-bit C compiler; instructions on
  compiling it manually are available further down


*********************
*** What you need ***
*********************


To use DUMB, you need a 32-bit C compiler (GCC and MSVC are fine). If you
have Allegro, DUMB can integrate with its audio streams and datafiles, making
your life easier. If you do not wish to use Allegro, you will have to do some
work to get music playing back. The 'dumbplay' example program requires
Allegro.

   Allegro - http://alleg.sf.net/


**********************************************
*** How to set DUMB up with DJGPP or MinGW ***
**********************************************


You should have got the .zip version. If for some reason you got the .tar.gz
version instead, you may have to convert make/config.bat to DOS text file
format. WinZip does this automatically by default. Otherwise, loading it into
MS EDIT and saving it again should do the trick (but do not do this to the
Makefiles as it destroys tabs). You will have to do the same for any files
you want to view in Windows Notepad. If you have problems, just go and
download the .zip instead.

Make sure you preserved the directory structure when you extracted DUMB from
the archive. Most unzipping programs will do this by default, but pkunzip
requires you to pass -d. If not, please delete DUMB and extract it again
properly.

If you are using Windows, open an MS-DOS Prompt or a Windows Command Line.
Change to the directory into which you unzipped DUMB.

If you are using MinGW (and you haven't renamed 'mingw32-make'), type:

   mingw32-make

Otherwise, type the following:

   make

DUMB will ask you whether you wish to compile for DJGPP or MinGW. Then it
will ask you whether you want support for Allegro. (You have to have made and
installed Allegro's optimised library for this to work.) Finally, it will
compile optimised and debugging builds of DUMB, along with the example
programs. When it has finished, run one of the following to install the
libraries:

   make install
   mingw32-make install

All done! If you ever need the configuration again (e.g. if you compiled for
DJGPP before and you want to compile for MinGW now), run one of the
following:

   make config
   mingw32-make config

See the comments in the Makefile for other targets.

Note: the Makefile will only work properly if you have COMSPEC or ComSpec set
to point to command.com or cmd.exe. If you set it to point to a Unix-style
shell, the Makefile won't work.

Please let me know if you have any trouble.

As an alternative, MSYS users may attempt to use the configure script,
available in dumb-0.9.3-autotools.tar.gz. This has been found to work without
Allegro, and is untested with Allegro. I should appreciate feedback from
anyone else who tries this. I do not recommend its use, partly because it
creates dynamically linked libraries and I don't know how to stop it from
doing that (see the section on compiling DUMB manually), and partly because
autotools are plain evil.

Scroll down for information on the example programs. Refer to docs/howto.txt
when you are ready to start programming with DUMB. If you use DUMB in a game,
let me know - I might decide to place a link to your game on DUMB's website!


******************************************************
*** How to set DUMB up with Microsoft Visual C++ 6 ***
******************************************************


If you have a newer version of Microsoft Visual C++ or Visual Something that
supports C++, please try these instructions and let me know if it works.

You should have got the .zip version. If for some reason you got the .tar.gz
version instead, you may have to convert some files to DOS text file format.
WinZip does this automatically by default. Otherwise, loading such files into
MS EDIT and saving them again should do the trick. You will have to do this
for any files you want to view in Windows Notepad. If you have problems, just
go and download the .zip instead.

Make sure you preserved the directory structure when you extracted DUMB from
the archive. Most unzipping programs will do this by default, but pkunzip
requires you to pass -d. If not, please delete DUMB and extract it again
properly.

DUMB comes with a workspace Microsoft Visual C++ 6, containing projects for
the DUMB core, the Allegro interface library and each of the examples. The
first thing you might want to do is load the workspace up and have a look
around. You will find it in the dumb\vc6 directory under the name dumb.dsw.
Note that the aldumb and dumbplay projects require Allegro, so they won't
work if you don't have Allegro. Nevertheless, dumbplay is the best-commented
of the examples, so do have a look.

When you are ready to add DUMB to your project, follow these instructions:

1. Open your project in VC++.
2. Select Project|Insert Project into Workspace...
3. Navigate to the dumb\vc6\dumb directory and select dumb.dsp.
   Alternatively, if you know that you are statically linking with a library
   that uses the statically linked multithreaded runtime (/MT), you may wish
   to select dumb_static.dsp in the dumb_static subdirectory instead.
4. Select Build|Set Active Configuration..., and reselect one of your
   project's configurations.
5. Select Project|Dependencies... and ensure your project is dependent on
   DUMB.
6. Select Project|Settings..., Settings for: All Configurations, C/C++ tab,
   Preprocessor category. Add the DUMB include directory to the Additional
   Include Directories box.
7. Ensure that for all the projects in the workspace (or more likely just all
   the projects in a particular dependency chain) the run-time libraries are
   the same. That's in Project|Settings, C/C++ tab, Code generation category,
   Use run-time library dropdown. The settings for Release and Debug are
   separate, so you'll have to change them one at a time. Exactly which run-
   time library you use will depend on what you need; it doesn't appear that
   DUMB has any particular requirements, so set it to whatever you're using
   now. (It will have to be /MD, the multithreaded DLL library, if you are
   statically linking with Allegro. If you are dynamically linking with
   Allegro than it doesn't matter.)
8. If you are using Allegro, do some or all of the above for the aldumb.dsp
   project in the aldumb directory too.

Good thing you only have to do all that once ... or twice ...

If you have the Intel compiler installed, it will - well, should - be used to
compile DUMB. The only setting I [Tom Seddon] added is /QxiM. This allows the
compiler to use PPro and MMX instructions, and so when compiling with Intel
the resultant EXE will require a Pentium II or greater. I don't think this is
unreasonable. After all, it is 2003 :)

[Note from Ben: the Intel compiler is evil! It makes AMD processors look bad!
Patch it or boycott it or something!]

If you don't have the Intel compiler, VC will compile DUMB as normal.

This project file and these instructions were provided by Tom Seddon (I hope
I got his name right; I had to guess it from his e-mail address!). Chad
Austin has since changed the project files around, and I've just attempted to
hack them to incorporate new source files. I've also tried to update the
instructions using guesswork and some knowledge of Visual J++ (you heard me).
The instructions and the project files are to this day untested by me. If you
have problems, check the download page at http://dumb.sf.net/ to see if they
are addressed; failing that, direct queries to me and I'll try to figure them
out.

If you have any comments at all on how the VC6 projects are laid out, or how
the instructions could be improved, I should be really grateful to hear them.
I am a perfectionist, after all. :)

Scroll down for information on the example programs. When you are ready to
start using DUMB, refer to docs/howto.txt. If you use DUMB in a game, let me
know - I might decide to place a link to your game on DUMB's website!


******************************************************
*** How to set DUMB up on Linux, BeOS and Mac OS X ***
******************************************************


You should have got the .tar.gz version. If for some reason you got the .zip
version instead, you may have to strip all characters with ASCII code 13 from
some of the text files. If you have problems, just go and download the
.tar.gz instead.

You have two options. There is a Makefile which should cope with most
systems. The first option is to use this default Makefile, and the procedure
is explained below. The second option is to download
dumb-0.9.3-autotools.tar.gz, extract it over the installation, run
./configure and use the generated Makefile. Users who choose to do this are
left to their own devices but advised to read the information at the end of
this section. I strongly recommend the first option.

If you are not using the configure script, the procedure is as follows.

First, run the following command as a normal user:

   make

You will be asked whether you want Allegro support. Then, unless you are on
BeOS, you will be asked where you'd like DUMB to install its headers,
libraries and examples (which will go in the include/, lib/ and bin/
subdirectories of the prefix you specify). BeOS has fixed locations for these
files. You may use shell variables here, e.g. $HOME or ${HOME}, but ~ will
not work. Once you have specified these pieces of information, the optimised
and debugging builds of DUMB will be compiled, along with the examples. When
it has finished, you can install them with:

   make install

You may need to be root for this to work. It depends on the prefix you chose.

Note: the Makefile will only work if COMSPEC and ComSpec are both undefined.
If either of these is defined, the Makefile will try to build for a Windows
system, and will fail.

Please let me know if you have any trouble.

Scroll down for information on the example programs. Refer to docs/howto.txt
when you are ready to start programming with DUMB. If you use DUMB in a game,
let me know - I might decide to place a link to your game on DUMB's website!

Important information for users of the configure script follows.

The Makefile generated by the configure script creates dynamically linked
libraries, and I don't know how to stop it from doing so. See the section
below on building DUMB manually for why I recommend linking DUMB statically.
However, if you choose to use the configure script, note the following.

The default Makefile is a copy of Makefile.rdy (short for 'ready'), and it
must exist with the name Makefile.rdy in order to work. The configure script
will overwrite Makefile, so if you want the default Makefile back, just run:

   cp Makefile.rdy Makefile

Do not use a symlink, as that would result in Makefile.rdy getting
overwritten next time the configure script is run!

You can also access the usual build system by passing '-f Makefile.rdy' to
Make.


********************************************************
*** How to build DUMB manually if nothing else works ***
********************************************************


Those porting to platforms without floating point support should be aware
that DUMB does use floating point operations but not in the inner loops. They
are used for volume and note pitch calculations, and they are used when
initialising the filter algorithm for given cut-off and resonance values.
Please let me know if this is a problem for you. If there is enough demand, I
may be able to eliminate one or both of these cases.

All of the library source code may be found in the src/ subdirectory. There
are headers in the include/ subdirectory, and src/helpers/resample.c also
#includes some .inc files in its own directory.

There are four subdirectories under src/. For projects not using Allegro, you
will need all the files in src/core/, src/helpers/ and src/it/. If you are
using Allegro, you will want the src/allegro/ subdirectory too. For
consistency with the other build systems, the contents of src/allegro/ should
be compiled into a separate library.

I recommend static-linking DUMB, since the version information is done via
macros and the API has a tendency to change. If you static-link, then once
your program is in binary form, you can be sure that changes to the installed
version of DUMB won't cause it to malfuction. It is my fault that the API has
been so unstable. Sorry!

Compile each .c file separately. As mentioned above, you will need to specify
two places to look for #include files: the include/ directory and the source
file's own directory. You will also need to define the symbol
DUMB_DECLARE_DEPRECATED on the command line.

Do not compile the .inc files separately.

You may need to edit dumb.h and add your own definition for LONG_LONG. It
should be a 64-bit integer. If you do this, please see if you can add a check
for your compiler so that it still works with other compilers.

DUMB has two build modes. If you define the symbol DEBUGMODE, some checks for
programmer error will be incorporated into the library. Otherwise it will be
built without any such checks. (DUMB will however always thoroughly check the
validity of files it is loading. If you ever find a module file that crashes
DUMB, please let me know!)

I recommend building two versions of the library, one with DEBUGMODE defined
and debugging information included, and the other with compiler optimisation
enabled. If you can install DUMB system-wide so that your projects, and other
people's, can simply #include <dumb.h> or <aldumb.h> and link with libraries
by simple name with no path, then that is ideal.

If you successfully port DUMB to a new platform, please let me know!


****************************
*** The example programs ***
****************************


Three example programs are provided. On DOS and Windows, you can find them in
the examples subdirectory. On other systems they will be installed system-
wide.

dumbplay
   This program will only be built if you have Allegro. Pass it the filename
   of an IT, XM, S3M or MOD file, and it will play it. It's not a polished
   player with real-time threading or anything - so don't complain about it
   stuttering while you use other programs - but it does show DUMB's fidelity
   nicely. You can control the playback quality by editing dumb.ini, which
   must be in the current working directory. (This is a flaw for systems
   where the program is installed system-wide, but it is non-fatal.) Have a
   look at the examples/dumb.ini file for further information.

dumbout
   This program does not need Allegro. You can use it to stream an IT, XM,
   S3M or MOD file to raw PCM. This can be used as input to an encoder like
   oggenc (with appropriate command-line options), or it can be sent to a
   .pcm file which can be read by any respectable waveform editor. This
   program is also convenient for timing DUMB. Compare the time it takes to
   render a module with the module's playing time! dumbout doesn't try to
   read any configuration file; the options are set on the command line.

dumb2wav
   This program is much the same as dumbout, but it writes a .wav file with
   the appropriate header. Thanks go to Chad Austin for this useful tool.


*********************************************
*** Downloading music or writing your own ***
*********************************************


If you would like to compose your own music modules, then this section should
help get you started.

The best programs for the job are the trackers that pioneered the file
formats:

   Impulse Tracker - IT files - http://www.lim.com.au/ImpulseTracker/
   Fast Tracker II - XM files - http://www.fasttracker2.com/
   Scream Tracker 3 - S3M files - No official site known, please use Google

MOD files come from the Amiga; I do not know what PC tracker to recommend for
editing these. If you know of one, let me know! In the meantime, I would
recommend using a more advanced file format. However, don't convert your
existing MODs just for the sake of it.

Fast Tracker II is Shareware. It offers a very flashy interface and has a
game embedded, but the IT file format is more powerful and better defined. By
all means try them both and see which you prefer; it is largely a matter of
taste (and, in some cases, religion). Impulse Tracker and Scream Tracker 3
are Freeware, although you can donate to Impulse Tracker and receive a
slightly upgraded version. DUMB is likely to be at its best with IT files.

These editors are DOS programs. Users of DOS-incapable operating systems may
like to try ModPlug Tracker, but should read docs/modplug.txt before using it
for any serious work. If you use a different operating system, or if you know
of any module editors for Windows that are more faithful to the original
trackers' playback, please give me some links so I can put them here!

   ModPlug Tracker - http://www.modplug.com/

If you have an x86 Linux system with VGA-compatible hardware (which covers
all PC graphics cards I've ever seen), you should be able to get Impulse
Tracker running with DOSEMU. You will have to give it access to the VGA ports
and run it in a true console, as it will not work with the X-based VGA
emulation. I personally added the SB16 emulation to DOSEMU, so you can even
use filters! However, it corrupts samples alarmingly often when saving on my
system - probably a DOSEMU issue. If you set this up, I am curious to know
whether it works for you.

   DOSEMU - http://www.dosemu.org/

BEWARE OF WINAMP! Although it's excellent for MP3s, it is notorious for being
one of the worst module players in existence; very many modules play wrongly
with it. There are plug-ins available to improve Winamp's module support, for
example WSP.

   Winamp - http://www.winamp.com/
   WSP - http://www.spytech.cz/index.php?sec=demo

(There is a Winamp plug-in that uses DUMB, but it is unreliable. If anyone
would like to work on it, please get in touch.)

While I am at it I should also point out that Winamp is notorious for
containing security flaws. Install it at your own risk, and if it is your
work computer, check with your boss first!

Samples and instruments are the building blocks of music modules. You can
download samples at

   http://www.tump.net/

If you would like to download module files composed by other people, check
the following sites:

   http://www.modarchive.com/
   http://www.scene.org/
   http://www.tump.net/
   http://www.homemusic.cc/main.php
   http://www.modplug.com/

Once again, if you know of more sites where samples or module files are
available for download, please let me know.

If you wish to use someone's music in your game, please respect the
composer's wishes. In general, you should ask the composer. Music that has
been placed in the Public Domain can be used by anyone for anything, but it
wouldn't do any harm to ask anyway if you know who the author is. In many
cases the author will be thrilled, so don't hesitate!

A note about converting modules from one format to another, or converting
from MIDI: don't do it, unless you are a musician and are prepared to go
through the file and make sure everything sounds the way it should! The
module formats are all slightly different, and MIDI is very different;
converting from one format to another will usually do some damage.

Instead, it is recommended that you allow DUMB to interpret the original file
as it sees fit. DUMB may make mistakes (it does a lot of conversion on
loading), but future versions of DUMB will be able to rectify these mistakes.
On the other hand, if you convert the file, the damage is permanent.


***********************
*** Contact details ***
***********************


If you have trouble with DUMB, or want to contact me for any other reason, my
e-mail address is given below. Please do get in touch, even if I appear to
have disappeared!

If you wish to chat online about something, perhaps on IRC, that can most
likely be arranged. Send me an e-mail.


******************
*** Conclusion ***
******************


This is the conclusion.


Ben Davis
entheh@users.sf.net
