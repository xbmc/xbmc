Event Client Examples and PS3 Sixaxis and Blu-Ray Remote Support
----------------------------------------------------------------

This directory contains 6 sample programs that demonstrate XBMC's
event server (which is still in development). The programs are in
Python and C++. XBMC also needs to be running (obviously) so that it
can receive the events.

- example_button1.py       | example_button1.cpp
- example_button2.py       | example_button2.cpp
- example_notification.py  | example_notification.cpp

The first 2 show how button / key presses can be sent to XBMC.
The third one shows how to display notifications in XBMC.

- xbmcclient.py
- xbmcclient.h

These are the Python module and C++ header that you can use when
writing your own programs. It is not yet complete and likely to change
but is still usable.

Implementation details can be found in the comments in the sample
programs.


PS3 Controller and PS3 Blu-Ray Remote Support
---------------------------------------------

There is also initial support for the PS3 controller (sixaxis) and the
PS3 Blu-Ray remote.

Pairing of the PS3 Blu-Ray Remote
---------------------------------

The remote needs to be paired initially with the 'ps3_remote.py'
program in this directory which you can continue using if you do not
want to run 'ps3d.py' as root. The disadvantage of using
'ps3_remote.py' is that pairing is required on every run. Once initial
pairing is done, 'ps3d.py', when run as root, will automatically
detect incoming connections from both the PS3 remote and the Sixaxis
controller.

Pairing of the PS3 Sixaxis Controller (TODO)
--------------------------------------------

The pairing of the PS3 controller is not yet handled automatically. It
can however be done using the program "sixaxis.c" available from:

http://www.pabr.org/sixlinux/sixlinux.en.html

Once pairing for eiher or both has been done, run the ps3d.py program
as root after disabling any existing HID servers that might currently
be running. The program requires root prvilieges since it listens on
Bluetooth L2CAP PSMs 17 and 19.

Using the PS3 Sixaxis Controller
--------------------------------

Currently, all that is supported with the Sixaxis controller is to be able
emulate the mouse behavior. Hold down the PS button and wave the controller
around and watch the mouse in XBMC mouse. Tilt it from left to right (along
your Z axis) to control horizontal motion. Tilt it towards and away from you
along (along your X axis) to control vertical mouse movement.

That's all for now.

WiiRemote Support
-----------------

The executable depends on libcwiid and libbluetooth and is compiled using
# g++ WiiRemote.cpp -lcwiid -o WiiRemote
The WiiRemote will emulate mouse by default but can be disabled by running with --disable-mouseemulation
The sensitivity of the mouseemulation can be set using the --deadzone_x or --deadzone_y where the number is
the percentage of the space is considered "dead", higher means more sensative.
Other commands can be listed with --help

The WiiRemote is mappable with keymap.xml where button id's are the following:
1 = Up
2 = Down
3 = Left
4 = Right
5 = A
6 = B
7 = Minus
8 = Home
9 = Plus
10 = 1
11 = 2
The name is by standard WiiRemote but this can be changed with the --joystick-name
