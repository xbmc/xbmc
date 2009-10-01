======================================================================
	TiMidity++ Installation guide

					Masanao Izumo
					<iz@onicos.co.jp>
					Mar.01.2004
					version 2.13.0 or later
======================================================================

This document describes how to install TiMidity++ for your UNIX-like
machine.

You can configure and make timidity.exe on the Cygwin environment of
Windows 95/98/Me/NT/2000/XP/2003.  If you are in Windows, install
Cygwin (or mingw) if you do not have them.

Today's Macintosh has FreeBSD userland, so things described here would
fly.  Methods for older Macintosh ("Classic") are not described here.

======================================================================
Basic Installation
======================================================================

TiMidity++ uses GNU autotools to build.  So the simplest way to
compile this package is:

1. "cd" to the directory containing TiMidity++'s source code and type
   "./configure" to configure the package for your system.  If you are
   using csh on an old version of System V, you might need to type
   "/bin/sh configure" instead to prevent csh from trying to execute
   configure itself.  Running configure takes a while.  While running,
   it prints some messages telling which features it is checking for.
2. Type "make" to compile the package.
   NOTE: this make method requires GNU make.  So if your system has it
         as gamke, type "gmake" instead.
3. Type "make install" to install the programs and any data files and
   documentation.

======================================================================
More complecated way
======================================================================

The full installation process is:

1. configre
2. edit common.makefile, Makefile, timidity.h if necessery
3. make
4. installation
5. set up voice data

Each processes are explained in following sections.  Note that % is
the shell prompt.

======================================================================
Configure
======================================================================

First, execute the following command:

% /bin/sh configure --help

Many options of configure will be displayed.  Most of them, such as
--help, --prefix=PREFIX, and so on are the regular ones.  They exisits
on most package that uses autoconf and you do not have to worry about
their behavior.

There also exists some options that is typical to TiMidity++.  Main of
these are the following:

--enable-debug
  Enables debug.  Things will be compiled with debugging methods/
  informations.

--without-x
  TiMidity++ uses X by default.  So you must specify this option to
  prevent linker from linking X libraries.

--enable-audio[=audio_list]
  Enables TiMidity++ to play MIDI files.  If --enable-audio=no,
  TiMidity++ acts as a MIDI-to-WAVE converter.

  You can specify one or more audio-device listed below.

  * default: Automatically select audio device.
  * oss: OSS /dev/dsp
  * sun: SunOS /dev/audio
  * hpux: hp-ux /dev/audio
  * irix: IRIX audio library
  * mme: OSF/1 MME
  * sb_dsp: BSD/OS 2.0 /dev/sb_dsp
  * w32: Windows MMS
  * darwin: darwin(Mac OS X)'s CoreAudio frameowrk
  * alsa: ALSA pcm device
  * alib: hp-ux network audio (Alib)
  * nas: NAS
  * portaudio: PortAudio
  * jack: JACK
  * arts: aRts
  * esd: EsounD
  * vorbis: ogg vorbis
  * gogo: mp3 Gogo-No-Coder (Windows only)

--enable-interface[=interface_list]
--enable-dynamic[=interface_list]
  Specify which interface to use.  If you use --enable-dynamic instead
  of --enable-interface, the interfaces specified will be linked
  dynamically and the binary size would become a bit smaller.

  You can select one or more interfaces listed below.

  * ncurses: ncurses interface.
  * slang: S-Lang interface.
  * motif: Motif interface.  Motif interface also works under Lestiff.
  * tcltk: Tcl/Tk interface.
  * emacs: Emacs front-end.  Type M-x timidity to invoke.
  * vt100: The full-screen interface using vt100 terminal control codes.
  * xaw: X Athena Widget interface.
  * xskin: X skin interface.
  * gtk: GTK+ interface.
  * w32gui: Build as Windows GUI binary.
  * winsyn: Build as TiMidity++ Windows Synthesizer server.
  * alsaseq: Build as ALSA sequencer client.

  Note that
  --enable-interface=INTERFACE1,INTERFACE2,...
  equals as
  --enable-INTERFACE1=yes --enable-INTERFACE2=yes ...
  and for the same way,
  --enable-dynamic=INTERFACE1,INTERFACE2,...
  equals as
  --enable-INTERFACE1=dynamic --enable-INTERFACE2=dynamic ...

--enable-network
  Enables network support.  This will allow TiMidity++ to open a MIDI
  file via network.  You can specify the location of MIDI files by
  http://foo.com.tw/bar/baz.mid - like format.

--enable-spectrogram
  With this option specified, TiMidity++ can open a window on X and
  show sound-spectrogram there.

--enable-wrd
  WRD is a Japanese local lyric-contents format.  This option enables
  WRD interface.

* Environment variables and flags to pass to configure

Some MIDI files eat too much CPU power.  If you choose correct
optimizing method, TiMidity++ can play such MIDI files smoothly.

You can tell configure which optimizing method to use by following
environmental variables:

CC
  the C compiler command e.g. "/usr/bin/gcc"

CFLAGS
  flags to pass to ${CC} e.g. "-O2 -pipe"

LDFLAGS
  flags to pass to linker e.g. "-L/usr/gnu/lib"

CPPFLAGS
  flags to pass to preprocessor e.g. "-traditional-cpp"

Your compiler may have many optimization flags.  For example, in case
of ultrasparc/gcc, you can specify:

% env CFLAGS='-O3 -Wall -mv8 -funroll-all-loops -fomit-frame-pointer \
	-mcpu=ultrasparc' /bin/sh configure [configure-options]...

and the binary will (hopefully) run faster.

======================================================================
Edit some files
======================================================================

If make fails, or if you want to change some parameters, edit
common.makefile, Makefile, or timidity.h manually.

* Parameters in timidity.h

There are some options that are hard-coded into timidity binary.  They
are # define-ed in timidity.h.  You have to change things there if you
want to change these flags.

** CONFIG_FILE

Edit CONFIG_FILE to your convenience.  By default,

#define CONFIG_FILE DEFAULT_PATH "/timidity.cfg"

are recommended.  DEFAULT_PATH is the same as TIMID_DIR in Makefile.

If you want to place it to another path, specify as the following:

#define CONFIG_FILE "/etc/timidity.cfg"

** DECOMPRESSOR_LIST

The file extractor (please ignore in Windows).  By default:

#define DECOMPRESSOR_LIST { \
		".gz", "gunzip -c %s", \
		".bz2", "bunzip2 -c %s", \
		".Z", "zcat %s", \
		".zip", "unzip -p %s", \
		".lha", "lha -pq %s", \
		".lzh", "lha -pq %s", \
		".shn", "shorten -x %s -", \
		0 }

TiMidity++ can handle some of archive format directly.  But other
format will use this extractor.

** PATCH_CONVERTERS

Configuration of of patch file converter (please ignore in Windows).
By default:

#define PATCH_CONVERTERS { \
		".wav", "wav2pat %s", \
		0 }

** PATCH_EXT_LIST

Configuration of extensions of GUS/patch file.  If specified in this
configuration, the extension can omit in all *.cfg.  By default:

#define PATCH_EXT_LIST { \
		".pat", \
		".shn", ".pat.shn", \
		".gz", ".pat.gz", \
		".bz2", ".pat.bz2", \
		0 }

** DEFAULT_PROGRAM

Configuration of default instrument.  By default:

#define DEFAULT_PROGRAM 0

If no Program Change event, this program name are adopted.  Usually 0
is Piano.

** DEFAULT_DRUMCHANNELS

Configuration of drum channel.  By default:

#define DEFAULT_DRUMCHANNELS {10, -1}

Numbers are the list of drum channels, and -1 is the terminator.  For
example, if you wish to default drum channel be 10 and 16,

#define DEFAULT_DRUMCHANNELS {10, 16, -1}

This channel can change in command line option.

** FLOAT_T

Type of floating point number.  Choose one of these:

* typedef double FLOAT_T;
* typedef float FLOAT_T;

Many machine which has FPU results faster operations with double than
that with float.  But some machine results contrary.

** (MAX|MIN)_OUTPUT_RATE

Minimum/maximum range of playing sample rate.  By default:

#define MIN_OUTPUT_RATE 4000
#define MAX_OUTPUT_RATE 65000

** DEFAULT_AMPLIFICATION

Default value of master volume.  By default:

#define DEFAULT_AMPLIFICATION 70

This number is the percentage of max volume.  This default value will
be nice in any occasions.  This number can specify in command line
option (-A).

** DEFAULT_RATE

Default sampling rate.  By default:

#define DEFAULT_RATE 44100

If you have much CPU power, DAT quality GUS/patch and want to listen
funny sound,

#define DEFAULT_RATE 48000

is good solution.

** DEFAULT_VOICES

Configuration of default polyphony numbers.  By default:

#define DEFAULT_VOICES 256

DEFAULT_VOICE is the polyphony number in boot-time.  This value is
configurable by the command line option (-p) from 1 to until memory is
allowed.  If your machine has much CPU power,

#define DEFAULT_VOICES 512

enables good harmony.

** AUDIO_BUFFER_BITS

Size of internal buffer.  By default:

#define AUDIO_BUFFER_BITS 12

I guess this values no need to change.

** CONTROLS_PER_SECOND

TiMidity++ do not calculate every envelope changes, but calculate some
samples at one time.  Small controls yields better quality sound, but
also eat much CPU time.  By default:

#define CONTROLS_PER_SECOND 1000

This can be changed from command line.  Leave as it is.

** DEFAULT_RESAMPLATION

Type of interpolation engine.  By default:

#define DEFAULT_RESAMPLATION resample_gauss

This definition cause TiMidity++ to Gauss-like interpolation in re-
sampling, and the quality of sound would be nice.  But it eats CPU
powers.  I recommend define it if your machine has much power.  Other
choices are (sorted by their speed):

#define DEFAULT_RESAMPLATION resample_none
#define DEFAULT_RESAMPLATION resample_linear
#define DEFAULT_RESAMPLATION resample_lagrange
#define DEFAULT_RESAMPLATION resample_cspline
#define DEFAULT_RESAMPLATION resample_gauss
#define DEFAULT_RESAMPLATION resample_newton

Interpolation methods are changeable from the command line.  If you
want to prevent users from doing so, uncomment next line and define as
this:

#define FIXED_RESAMPLATION

** USE_DSP_EFFECT

Configuration of USE_DSP_EFFECT to refine chorus, delay, EQ and
insertion effect.  Default enabled.

** LOOKUP_HACK

Configuration of LOOKUP_HACK.  By default, this features are undefined
like this:

/* #define LOOKUP_HACK */
/* #define LOOKUP_INTERPOLATION */

This option saves a little CPU power, but sound quality would decrease
noticeably.  If your machine suffers from lack of CPU power, enable
it.

** SMOOTH_MIXING

Defining this greatly reduces popping due to large volume/pan changes.
This is definitely worth the slight increase in CPU usage.

** FAST_DECAY

Configuration of FAST_DECAY.  By default:

/* #define FAST_DECAY */

This option makes envelopes twice as fast and saves CPU power.  But
since the release time of voices is shorten, the sound would be poor.
This feature is controllable in command line option.

** FRACTION_BITS

TiMidity++ uses fixed-point calculation.  Its default is

#define FRACTION_BITS 12

and you don't have to change this value.

** ADJUST_SAMPLE_VOLUMES

Configuration of adjusting amplitude of GUS/patch.  By default:

#define ADJUST_SAMPLE_VOLUMES

This option makes TiMidity to adjust amplitudes of each GUS/patch to
same volume.

** DENGEROUS_RENICE

By default this feature is disabled:

/* #define DANGEROUS_RENICE -15 */

If you want to increase process priority of TiMidity++ by using setuid
root enable this option.  This option is only available in UNIX.  Once
you enabled this option, you should install timidity with the follow-
ing procedure:

# chown root /usr/local/bin/timidity
# chmod u+s /usr/local/bin/timidity

Note: You should not set setuid to timidity if DANGEROUS_RENICE isn't
      enabled.

** MAX_DIE_TIME

If this value is too small, click noise would be come.  Default is:

#define MAX_DIE_TIME 20

and I recommend this value leave to this.

** LOOKUP_SINE

#define LOOKUP_SINE

On some machines (especially PCs without math coprocessors), looking
up sine values in a table will be significantly faster than computing
them on the fly.  I recommend define it.

** PRECALC_LOOPS

Configuration of optimizing re-sampling.  By default:

#define PRECALC_LOOPS

These may not in fact be faster on your particular machine and
compiler.

** USE_LDEXP

Configuration of use of ldexp().  By default this feature is disabled:

/* #define USE_LDEXP */

If your machine can multiply floating point number with ldexp() faster
than other method, enable this option.

** DEFAULT_CACHE_DATA_SIZE

Size of pre-re-sampling cache.  By default:

#define DEFAULT_CACHE_DATA_SIZE (2*1024*1024)

This can be changed from command line, so you don't have to change
here.

* Configurations about network

TiMidity++ can access any files via networks with URL.  This feature
are configurable in Makefile.  If you have enabled this feature in
Makefile (configure --enable-network), configure the following macros:

** MAIL_DOMAIN

specifies domain name of your name address.  If your name address is
"iz@onicos.co.jp" set the macro:

#define MAIL_DOMAIN "@onicos.co.jp"

** MAIL_NAME

specifies mail name of yours if in Windows.  In UNIX, uncomment it.
For example, your name address is "iz@onicos.co.jp" set the macro:

#define MAIL_NAME "iz"

** TMPDIR

Configuration of temporary directory.  By default, this option is
disabled:

/* #define TMPDIR "/var/tmp" */

In UNIX, if this option is disabled TiMidity++ creates temporary files
in the path specified by the environment variable TMPDIR.  If environ-
ment variable TMPDIR also isn't defined, TiMidity++ creates temporary
files in /tmp.  In Windows, TMPDIR variable are ignored.  So you
should specify the temporary path with this macro.

** GS_DRUMPART

Recognizing GS drum part by GS exclusive message.

#define GS_DRUMPART

enables to recognize GS exclusive message to set drum part.

/* #define GS_DRUMPART */

disables this feature.

* Japanese-text-handling related options

There are some options for Japanese handling.

** JAPANESE

If your system is in Japanese environment, define

#define JAPANESE

otherwise comment it out like

/* #define JAPANESE */

** OUTPUT_TEXT_CODE

specifies output text code (in Japanese environment).  You should
specify appropriate code name to OUTPUT_TEXT_CODE macro.  The follow-
ing strings are available:

AUTO
  Auto conversion by `LANG' environment variable (UNIX only)
ASCII
  Convert unreadable characters to '.' (0x2e)
NOCNV
  No conversion
1251
  Convert from windows-1251 to koi8-r
EUC
  eucJP
JIS
  JIS
SJIS
  shift-JIS

In Japanized UNIX system, all of above are available.  In Windows,
"ASCII", "NOCNV", "SJIS" are available.  If your environment cannot
handle Japanese, specify "ASCII" or "NOCNV" alternatively.

** MODULATION_WHEEL_ALLOW
** PORTAMENTO_ALLOW
** NRPN_VIBRATO_ALLOW
** REVERB_CONTROL_ALLOW
** FREEVERB_CONTROL_ALLOW
** CHORUS_CONTROL_ALLOW
** SURROUND_CHORUS_ALLOW
** GM_CHANNEL_PRESSURE_ALLOW
** VOICE_CHAMBERLIN_LPF_ALLOW
** VOICE_MOOG_LPF_ALLOW
** MODULATION_ENVELOPE_ALLOW
** ALWAYS_TRACE_TEXT_META_EVENT
** OVERLAP_VOICE_ALLOW
** TEMPER_CONTROL_ALLOW

Controllers of MIDI actions.  By default:

#define MODULATION_WHEEL_ALLOW
#define PORTAMENTO_ALLOW
#define NRPN_VIBRATO_ALLOW
/* #define REVERB_CONTROL_ALLOW */
#define FREEVERB_CONTROL_ALLOW
#define CHORUS_CONTROL_ALLOW
/* #define SURROUND_CHORUS_ALLOW */
/* #define GM_CHANNEL_PRESSURE_ALLOW */
#define VOICE_CHAMBERLIN_LPF_ALLOW
/* #define VOICE_MOOG_LPF_ALLOW */
/* #define MODULATION_ENVELOPE_ALLOW */
/* #define ALWAYS_TRACE_TEXT_META_EVENT */
#define OVERLAP_VOICE_ALLOW
#define TEMPER_CONTROL_ALLOW

These values are configurable in command line options.  So you may
leave these in default value.

======================================================================
Make
======================================================================

Make section has nothing particular to write.  Just say "make"

...Oops, almost forgot, TiMidity++'s Makefile needs GNU version of
make.  If you do not have, get one first.  If you have one in a
different name than "make", type its true name instead.

Installation

On UNIX and clones, you can type "make install" to install all files.
Or you can select following targets:

install.bin
  installs executable filles
install.tk
  installs Tcl/Tk interface
install.el
  installs Emacs interface
install.man
  installs man files
install
  installs everything

I strongly recommend you to check the install destinations and files
by setteing -n flag like

% make -n

======================================================================
Search for voice data
======================================================================

TiMidity++ uses Either GUS/patch, or SoundFont(, or both) as the voice
data to play.  You must get a SoundFont or GUS/patch files, and make
the configuration file.  You must make the configuration file (*.cfg).
By default, timidity.cfg is /usr/local/share/timidity/timidity.cfg (or
C:\WINDOWS\TIMIDITY.CFG on Windows).  And please check the following
sites for many voice(patch) data:

* http://www.onicos.com/staff/iz/timidity/link.html#gus
* http://www.onicos.com/staff/iz/timidity/dist/cfg/ (Some sample *.cfg's)
* http://www.i.h.kyoto-u.ac.jp/~shom/timidity/ (10M and 4M patches)
* ftp://ftp.cdrom.com/pub/gus/sound/patches/files/ (GUS site)

If you got funny voice archive, extract it to appropriate directory
and configure *.cfg files with the name and path of these voice datas.
