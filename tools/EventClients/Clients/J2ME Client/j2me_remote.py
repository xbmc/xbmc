#!/usr/bin/python
#
# XBMC Media Center
# J2ME Remote
# Copyright (c) 2008 topfs2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import sys
try:
    from kodi.xbmcclient import *
    from kodi.bt.bt import *
    from kodi.defs import *
except:
    sys.path.append('../../lib/python')
    from xbmcclient import *
    from bt.bt import *
    ICON_PATH = "../../icons/"

from socket import *
import os
from threading import Thread

host = "localhost"
port = 9777
addr = (host, port)
sock = socket(AF_INET,SOCK_DGRAM)

def send_key(key):
    packet = PacketBUTTON(map_name="JS0:J2ME", code=key, repeat=0, queue=0)
    packet.send(sock, addr)

def send_message(caption, msg):
    packet = PacketNOTIFICATION(caption,
                                msg,
                                ICON_PNG,
                                icon_file=ICON_PATH + "/phone.png")

    packet.send(sock, addr)

def send_ImageToBluetooth(Image):
    imageData = file( Image ).read()
    print "Data len ", len( imageData )
    client_sock.send( format.uint32( len( imageData) ) )
    client_sock.sendall(imageData)
    print "Sent file"

class Ping(Thread):
    def __init__ (self):
        Thread.__init__(self)
    def run(self):
        import time
        while 1: # ping the server every 19s
            packet = PacketPING()
            packet.send(sock, addr)
            time.sleep (19)

def main():
    import time

    # connect to localhost, port 9777 using a UDP socket
    # this only needs to be done once.
    # by default this is where XBMC will be listening for incoming
    # connections.
    server_sock=bt_create_rfcomm_socket()
    server_sock.listen(2)

    portBT = server_sock.getsockname()[1]

    uuid = "ACDC"
    bt_advertise(socket=server_sock, name="XBMC Remote", uuid=uuid)

    print "Waiting for connection on RFCOMM channel %d" % portBT

    packet = PacketHELO(devicename="J2ME Remote",
                        icon_type=ICON_PNG,
                        icon_file=ICON_PATH + "/phone.png")
    packet.send(sock, addr)
    Ping().start()
    while(True):
        try:
            client_sock, client_info = server_sock.accept()
            print "Accepted connection from ", client_info
            client_addr, client_port  = client_info
            runFlag  = True
            firstRun = True
            print "Accepted connection from ", client_addr
        except KeyboardInterrupt:
            packet = PacketLOG(LOGNOTICE, "J2ME: User interrupted")
            packet.send(sock, addr)
            bt_stop_advertising(socket = server_sock)
            server_sock.close()
            sys.exit(0)
        try:
            while runFlag:
                if firstRun:
                    client_name = client_sock.recv(1024)
                    firstRun = False
                    send_message("J2ME: Phone Connected", client_name)
                    packet = PacketLOG(LOGNOTICE, "Phone Connected")
                    packet.send(sock, addr)
                data = client_sock.recv(3)
                if(data[0] == "0"):
                    if(data[1] == "0"):
                        runFlag = False
                        packet = PacketLOG(LOGNOTICE, "J2ME: Recieved disconnect command")
                        packet.send(sock, addr)
                        client_sock.close()
                    if(data[1] == "1"):
                        print "Shuting down server"
                        client_sock.close()
                        bt_stop_advertising(socket = server_sock)
                        server_sock.close()
                        packet = PacketBYE()
                        packet.send(sock, addr)
                        sys.exit(0)

                elif(data[0] == "2"):
                    print "<button id=\"%i\">" % ord( data[1] )
                    send_key(ord( data[1] ) )
                else:
                  print "Unkown received data [%s]" % data
        except IOError:
            print "Lost connection too %s" % client_name
            runFlag = False
            send_message("Phone Disconnected", client_name)
            pass
        except KeyboardInterrupt:
            packet = PacketLOG(LOGNOTICE, "J2ME: User interrupted")
            packet.send(sock, addr)
            client_sock.close()
            bt_stop_advertising(socket = server_sock)
            server_sock.close()
            packet = PacketBYE()    # PacketPING if you want to ping
            packet.send(sock, addr)
            sys.exit(0)

def usage():
    print """
Java Bluetooth Phone Remote Control Client for XBMC v0.1

Usage for xbmcphone.py
    --address ADDRESS  (default: localhost)
    --port PORT        (default: 9777)
    --help             (Brings up this message)
"""

if __name__=="__main__":
    try:
        i = 0
        while (i < len(sys.argv)):
            if   (sys.argv[i] == "--address"):
                host = sys.argv[i + 1]
            elif (sys.argv[i] == "--port"):
                port = sys.argv[i + 1]
            elif (sys.argv[i] == "--help"):
                usage()
                sys.exit(0)
            i = i + 1
        main()
    except:
        sys.exit(0)
