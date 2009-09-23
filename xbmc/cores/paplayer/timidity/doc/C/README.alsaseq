=======================================================================
                      ALSA Sequencer Interface
          Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
=======================================================================

DESCRIPTION
===========

This document describes about the Advanced Linux Sound Architecture
(ALSA) sequencer interface.  The ALSA sequencer interface communicates
between ALSA sequencer core and timidity.  The interface receives
events from sequencer and plays it in (quasi-)real-time.
On this mode, TiMidity works purely as the software real-time MIDI
render, that is as a software MIDI synth engine on ALSA.
There is no scheduling routine in this interface, since all scheduling
is done by ALSA sequencer core.

For invoking ALSA sequencer interface, run timidity as follows:
	% timidity -iA -B2,8 -Os -q0/0 -k0
The fragment size is adjustable.  The smaller number gives better
real-time response.  Then timidity shows new port numbers which were
newly created (128:0 and 128:1 below).
       ---------------------------------------
	% timidity -iA -B2,8 -Os -q0/0 -k0
	TiMidity starting in ALSA server mode
	Opening sequencer port 128:0 128:1
       ---------------------------------------
These ports can be connected with any other sequencer ports.
For example, playing a MIDI file via pmidi (what's an overkill :-),
	% pmidi -p128:0 foo.mid
If a midi file needs two ports, you may connect like this:
	% pmidi -p128:0,128:1 bar.mid
Connecting from external MIDI keyboard may become like this:
	% aconnect 64:0 128:0

INSTALLATION
============

Configure with --enable-alsaseq and --enable-audio=alsa option.
Of course, other audio devices or interfaces can be chosen
additionally, too.

For getting better real-time response, timidity must be run as root
(see below).  Set-UID root is the easiest way to achieve this.  You
may change the owner and the permission of installed timidity binary
as follows:
	# chown root /usr/local/bin/timidity
	# chmod 4755 /usr/local/bin/timidity

Be aware that this might cause a security hole!


REAL-TIME RESPONSE
==================

The interface tries to reset process scheduling to SCHED_FIFO and as
high priority as possible.  The SCHED_FIFO'd program exhibits much
better real-time response.  For example, without SCHED_FIFO, timidity
may cause significant pauses at every time /proc is accessed. 
For enabling this feature, timidity must be invoked by root or
installed with set-uid root.


INSTRUMENT LOADING
==================

Timidity loads instruments dynamically at each time a program change
event is received.  This causes sometimes pauses due to buffer
underrun during playback.  Furthermore, timidity resets the loaded
instruments when the all subscriptions are disconnected.  Thus for
keeping all loaded instruments even after playback is finished, you
need to connect a dummy port (e.g. midi input port) to a timidity port
via aconnect:
	% aconnect 64:0 128:0


RESET PLAYBACK
==============

You may stop all sounds during playback by sending SIGHUP signal to
timiditiy.  The connections will be kept even after reset, but the
events will be no longer processed.  For enabling the audio again, you 
have to reconnect the ports.


VISUALIZATION
=============

If you prefer a bit more fancy visual output, try my tiny program,
aseqview.
	% aseqview -p2 &
Then connect two ports to timidity ports (assumed 129:0 and 129:1 were 
created by aseqview):
	% aconnect 129:0 128:0
	% aconnect 129:1 128:1
The outputs ought to be redirected to 129:0,1 instead of 128:0,1.
	% pmidi -p129:0,129:1 foo.mid


COMPATIBILITY WITH OSS
======================

You may access to timidity also via OSS MIDI emulation on ALSA
sequencer.  Take a look at /proc/asound/seq/oss for checking the
device number to be accessed.
       ---------------------------------------
	% cat /proc/asound/seq/oss
	OSS sequencer emulation version 0.1.8
	ALSA client number 63
	ALSA receiver port 0
	...
	midi 1: [TiMidity port 0] ALSA port 128:0
	  capability write / opened none
	
	midi 2: [TiMidity port 1] ALSA port 128:1
	  capability write / opened none
       ---------------------------------------
In the case above, the MIDI devices 1 and 2 are assigned to timidity.
Now, play with playmidi:
	% playmidi -e -D1 foo.mid


BUGS
====

Well, well, they must be there..


RESOURCES
=========

- ALSA home page
	http://www.alsa-project.org
- My ALSA hack page (including aseqview)
	http://members.tripod.de/iwai/alsa.html
