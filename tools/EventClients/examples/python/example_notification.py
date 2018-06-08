#!/usr/bin/python

# This is a simple example showing how you can show a notification
# window with a custom icon inside XBMC. It could be used by mail
# monitoring apps, calendar apps, etc.

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

    packet = PacketHELO("Email Notifier", ICON_NONE)
    packet.send(sock, addr)

    # wait for 5 seconds
    time.sleep (5)

    packet = PacketNOTIFICATION("New Mail!",            # caption
                                "RE: Check this out",   # message
                                ICON_PNG,               # optional icon type
                                "../../icons/mail.png") # icon file (local)
    packet.send(sock, addr)

    packet = PacketBYE()
    packet.send(sock, addr)

if __name__=="__main__":
    main()
