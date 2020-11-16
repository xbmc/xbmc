#!/usr/bin/env python3

# This is a simple example showing how you can send a key press event
# to XBMC using the XBMCClient class

import os
from socket import *
import sys
import time

if os.path.exists("../../lib/python"):
    # try loading modules from source directory
    sys.path.append("../../lib/python")

    from xbmcclient import *

    ICON_PATH = "../../icons/"
else:
    # fallback to system wide modules

    from kodi.xbmcclient import *
    from kodi.defs import *

def main():

    host = "localhost"
    port = 9777

    # Create an XBMCClient object and connect
    xbmc = XBMCClient("Example Remote", ICON_PATH + "/bluetooth.png")
    xbmc.connect()

    # wait for notification window to close (in XBMC) (optional)
    time.sleep(5)

    # send a up key press using the xbox gamepad map "XG" and button
    # name "dpadup" ( see PacketBUTTON doc for more details)
    xbmc.send_button(map="XG", button="dpadup")

    # wait for a few seconds to see its effect
    time.sleep(5)

    # send a right key press using the keyboard map "KB" and button
    # name "right"
    xbmc.send_keyboard_button("right")

    # wait for a few seconds to see its effect
    time.sleep(5)

    # that's enough, release the button.
    xbmc.release_button()

    # ok we're done, close the connection
    # Note that closing the connection clears any repeat key that is
    # active. So in this example, the actual release button event above
    # need not have been sent.
    xbmc.close()

if __name__=="__main__":
    main()
