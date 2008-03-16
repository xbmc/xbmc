#!/usr/bin/python

# This is a quick port of brandonj's PS3 remote script to use the event server
# for sending input events.
#
# The original script and documentation regarding the remote can be found at:
#   http://xbmc.org/forum/showthread.php?t=28765
#
#
# TODO:
#    1. Send keepalive ping at least once every 60 seconds to prevent timeouts
#    2. Permanent pairing
#    3. Detect if XBMC has been restarted (non trivial until broadcasting is
#       implemented, until then maybe the HELO packet could be used instead of
#       PING as keepalive
#

from xbmcclient import *
from socket import *

import bluetooth
import os
import time

loop_forever = True

host = "192.168.1.119"
port = 9777
addr = (host, port)
sock = socket(AF_INET,SOCK_DGRAM)

g_keymap = {
    "16": 's' ,#EJECT
    "64": 'w' ,#AUDIO
    "65": 'z' ,#ANGLE
    "63": 'n' ,#SUBTITLE
    "0f": 'd' ,#CLEAR
    "28": 't' ,#TIME

    "00": '1' ,#1
    "01": '2' ,#2
    "02": '3' ,#3
    "03": '4' ,#4
    "04": '5' ,#5
    "05": '6' ,#6
    "06": '7' ,#7
    "07": '8' ,#8
    "08": '9' ,#9
    "09": '0' ,#0

    "81": None ,#RED
    "82": None ,#GREEN
    "80": None ,#BLUE
    "83": None ,#YELLOW

    "70": 'I'      ,#DISPLAY
    "1a": None     ,#TOP MENU
    "40": 'menu'   ,#POP UP/MENU
    "0e": 'escape' ,#RETURN

    "5c": 'menu'   ,#OPTIONS/TRIANGLE
    "5d": 'escape' ,#BACK/CIRCLE
    "5e": 'tab'    ,#X
    "5f": 'V'      ,#VIEW/SQUARE

    "54": 'up'     ,#UP
    "55": 'right'  ,#RIGHT
    "56": 'down'   ,#DOWN
    "57": 'left'   ,#LEFT
    "0b": 'return' ,#ENTER

    "5a": 'plus'     ,#L1
    "58": 'minus'    ,#L2
    "51": None       ,#L3
    "5b": 'pageup'   ,#R1
    "59": 'pagedown' ,#R2
    "52": 'c'        ,#R3

    "43": 's'                  ,#PLAYSTATION
    "50": 'opensquarebracket'  ,#SELECT
    "53": 'closesquarebracket' ,#START

    "33": 'r'      ,#<-SCAN
    "34": 'f'      ,#  SCAN->
    "30": 'comma'  ,#PREV
    "31": 'period' ,#NEXT
    "60": None     ,#<-SLOW/STEP
    "61": None     ,#  SLOW/STEP->
    "32": 'P'      ,#PLAY
    "38": 'x'      ,#STOP
    "39": 'space'  #PAUSE
    }

def send_key(key):
    packet = PacketBUTTON(map_name="KB", button_name=key)
    packet.send(sock, addr)

def release_key():
    packet = PacketBUTTON(code=0x01, down=0)
    packet.send(sock, addr)

def send_message(caption, msg):
    packet = PacketNOTIFICATION(
        caption,
        msg,
        ICON_PNG,
        "icons/bluetooth.png")
    packet.send(sock, addr)

def save_remote_address(addr):
    try:
        f = open(".ps3_remote_address", "wb")
        f.write(str(addr))
        f.close()
    except:
        pass
    return

def load_remote_address():
    try:
        f = open(".ps3_remote_address", "r")
        a = f.read()
        f.close()
        a.strip()
        return a
    except:
        pass
    return None

while loop_forever is True:

    target_name = "BD Remote Control"
    target_address = load_remote_address()
    remote = bluetooth.BluetoothSocket(bluetooth.L2CAP)

    target_connected = False

    packet = PacketHELO(devicename="PS3 Bluetooth Remote",
                        icon_type=ICON_PNG,
                        icon_file="icons/bluetooth.png")
    packet.send(sock, addr)

    while target_connected is False:
        send_message("Action Required!", "Hold Start+Enter on your remote.")
        print "Searching for %s" % target_name
        print "(Hold Start + Enter on remote to make it discoverable)"
        time.sleep(2)

        if not target_address:
            try:
                nearby_devices = bluetooth.discover_devices()
            except:
                print "Error performing bluetooth discovery"
                send_message("Error", "Unable to find devices.")
                time.sleep(5)
                continue

            for bdaddr in nearby_devices:
                bname = bluetooth.lookup_name( bdaddr )
                print "%s (%s) in range" % (bname,bdaddr)
                if target_name == bname:
                    target_address = bdaddr
                    break

        if target_address is not None:
            print "Found %s with address %s" % (target_name, target_address)
            if not load_remote_address():
                send_message("Found Device", "Pairing %s, please wait." % target_name)
                print "Attempting to pair with remote"

            try:
                save_remote_address(target_address)
                remote.connect((target_address,19))
                target_connected = True
                print "Remote Paired.\a"
                send_message("Pairing Success",
                             "Your remote was successfully paired and is ready to be used.")
            except:
                del remote
                remote = bluetooth.BluetoothSocket(bluetooth.L2CAP)
                target_address = None
                send_message("Pairing Failed",
                             "An error occurred while attempting to pair.")
                print "ERROR - Could Not Connect. Trying again..."
                time.sleep(2)
        else:
            send_message("Error", "No remotes were found.")
            print "Could not find BD Remote Control. Trying again..."
            time.sleep(2)

    done = False

    while not done:
        # re-send HELO packet in case we timed out
        packet = PacketHELO(devicename="Bluetooth Remote Reconnected",
                            icon_type=ICON_NONE)
        
        packet.send(sock, addr)

        datalen = 0
        try:
            data = remote.recv(1024)
            datalen = len(data)
        except:
            time.sleep(2)
            done = True

        if datalen == 13:
            keycode = data.encode("hex")[10:12]

            if keycode == "ff":
                release_key()
                continue
            try:
                if g_keymap[keycode]:
                    send_key(g_keymap[keycode])
            except Exception, e:
                print "Unknown data: %s" % str(e)
        else:
            print "Unknown data"

    print "Disconnected."
    try:
        remote.close()
    except:
        print "Cannot close."

