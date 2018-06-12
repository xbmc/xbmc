#!/usr/bin/python

# This is a simple example showing how you can send a key press event
# to XBMC in a non-queued fashion to achieve a button pressed down
# event i.e. a key press that repeats.

# The repeat interval is currently hard coded in XBMC but that might
# change in the future.

# NOTE: Read the comments in 'example_button1.py' for a more detailed
# explanation.

import sys
sys.path.append("../../lib/python")

from xbmcclient import *
from socket import *

def main():
    import time
    import sys

    host = "localhost"
    port = 9777
    addr = (host, port)

    sock = socket(AF_INET,SOCK_DGRAM)

    # First packet must be HELO and can contain an icon
    packet = PacketHELO("Example Remote", ICON_PNG,
                        "../../icons/bluetooth.png")
    packet.send(sock, addr)

    # wait for notification window to close (in XBMC)
    time.sleep(5)

    # send a up key press using the xbox gamepad map "XG" and button
    # name "dpadup" ( see PacketBUTTON doc for more details)
    packet = PacketBUTTON(map_name="XG", button_name="dpadup")
    packet.send(sock, addr)

    # wait for a few seconds to see its effect
    time.sleep(5)

    # send a down key press using the raw keyboard code
    packet = PacketBUTTON(code=0x28)
    packet.send(sock, addr)

    # wait for a few seconds to see its effect
    time.sleep(5)

    # send a right key press using the keyboard map "KB" and button
    # name "right"
    packet = PacketBUTTON(map_name="KB", button_name="right")
    packet.send(sock, addr)

    # wait for a few seconds to see its effect
    time.sleep(5)

    # that's enough, release the button. During release, button code
    # doesn't matter.
    packet = PacketBUTTON(code=0x28, down=0)
    packet.send(sock, addr)

    # ok we're done, close the connection
    # Note that closing the connection clears any repeat key that is
    # active. So in this example, the actual release button event above
    # need not have been sent.
    packet = PacketBYE()
    packet.send(sock, addr)

if __name__=="__main__":
    main()
