This directory contains 3 sample programs that demonstrate XBMC's
event server (which is still in development). The programs are all in
Python, so you need Python installed to run them. XBMC also needs to
be running (obviously) so that it can receive the events.

- example_button1.py
- example_button2.py
- example_notification.py

The first 2 show how button / key presses can be sent to XBMC.
The third one shows how to display notifications in XBMC.

- xbmcclient.py

This is the Python module that you will want to use when writing
your own programs. It is not yet complete and likely to change
but is still usable.

Implementation details can be found in the comments in the sample
programs.