#!/usr/bin/env python3

# This is a simple example showing how you can send a key press event
# to XBMC using the XBMCClient class

import os
from socket import *
import sys

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

    # send a up key press using the xbox gamepad map "XG" and button
    # name "dpadup" ( see PacketBUTTON doc for more details)
    try:
        xbmc.send_action(sys.argv[2], ACTION_BUTTON)
    except:
        try:
            xbmc.send_action(sys.argv[1], ACTION_EXECBUILTIN)
        except Exception as e:
            print(str(e))
            xbmc.send_action("ActivateWindow(ShutdownMenu)")


    # ok we're done, close the connection
    # Note that closing the connection clears any repeat key that is
    # active. So in this example, the actual release button event above
    # need not have been sent.
    xbmc.close()

if __name__=="__main__":
    main()
