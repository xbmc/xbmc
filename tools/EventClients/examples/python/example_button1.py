#!/usr/bin/python

# This is a simple example showing how you can send 2 button events
# to XBMC in a queued fashion to shut it down.

# Queued button events are not repeatable.

# The basic idea is to create single packets and shoot them to XBMC
# The provided library implements some of the support commands and
# takes care of creating the actual packet. Using it is as simple
# as creating an object with the required constructor arguments and
# sending it through a socket.

# Currently, only keyboard keys are supported so the key codes used
# below are the same key codes used in guilib/common/SDLKeyboard.cpp

# In effect, anything that can be done with the keyboard can be done
# using the event client.

# import the XBMC client library
# NOTE: The library is not complete yet but is usable at this stage.

import sys
sys.path.append("../../lib/python")

from xbmcclient import *
from socket import *

def main():
    import time
    import sys

    # connect to localhost, port 9777 using a UDP socket
    # this only needs to be done once.
    # by default this is where XBMC will be listening for incoming
    # connections.
    host = "localhost"
    port = 9777
    addr = (host, port)
    sock = socket(AF_INET,SOCK_DGRAM)

    # First packet must be HELO (no it's not a typo) and can contain an icon
    # 'icon_type' can be one of ICON_NONE, ICON_PNG, ICON_JPG or ICON_GIF
    packet = PacketHELO(devicename="Example Remote",
                        icon_type=ICON_PNG,
                        icon_file="../../icons/bluetooth.png")
    packet.send(sock, addr)

    # IMPORTANT: After a HELO packet is sent, the client needs to "ping" XBMC
    # at least once every 60 seconds or else the client will time out.
    # Every valid packet sent to XBMC acts as a ping, however if no valid
    # packets NEED to be sent (eg. the user hasn't pressed a key in 50 seconds)
    # then you can use the PacketPING class to send a ping packet (which is
    # basically just an empty packet). See below.

    # Once a client times out, it will need to reissue the HELO packet.
    # Currently, since this is a unidirectional protocol, there is no way
    # for the client to know if it has timed out.

    # wait for notification window to close (in XBMC)
    time.sleep(5)

    # press 'S'
    packet = PacketBUTTON(code='S', queue=1)
    packet.send(sock, addr)

    # wait for a few seconds
    time.sleep(2)

    # press the enter key (13 = enter)
    packet = PacketBUTTON(code=13, queue=1)
    packet.send(sock, addr)

    # BYE is not required since XBMC would have shut down
    packet = PacketBYE()    # PacketPING if you want to ping
    packet.send(sock, addr)

if __name__=="__main__":
    main()
