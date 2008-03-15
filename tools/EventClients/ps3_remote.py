#!/usr/bin/python

# This is a quick port of brandonj's PS3 remote script to use the event server
# for sending input events.
#
# The original script and documentation regarding the remote can be found at:
#   http://xbmc.org/forum/showthread.php?t=28765
#

from xbmcclient import *
from socket import *

import bluetooth
import os

loop_forever = True

host = "localhost"
port = 9777
addr = (host, port)
sock = socket(AF_INET,SOCK_DGRAM)

def send_key(key):
	packet = PacketBUTTON(code=key, repeat=1)
	packet.send(sock, addr)

def release_key():
	packet = PacketBUTTON(code=0x01, down=0)
	packet.send(sock, addr)

while loop_forever is True:

        target_name = "BD Remote Control"
        target_address = None
        remote = bluetooth.BluetoothSocket(bluetooth.L2CAP)

        target_connected = False
        while target_connected is False:            

            if not target_address:
                print "Searching for BD Remote Control"
                print "(Press Start + Enter on remote to make discoverable)"
                nearby_devices = bluetooth.discover_devices()
            
                for bdaddr in nearby_devices:
                    if target_name == bluetooth.lookup_name( bdaddr ):
                        target_address = bdaddr
                        break

            if target_address is not None:
                print "Found BD Remote Control with address ", target_address
                print "Attempting to pair with remote"
                    
                try:
                    remote.connect((target_address,19))
                    target_connected = True
                    print "Remote Paired.\a"
                except:
                    print "ERROR - Could Not Connect. Trying again..."
                        
            else:
                print "Could not find BD Remote Control. Trying again..."


        done = False
	packet = PacketHELO(devicename="PS3 Bluetooth Remote",
			    icon_type=ICON_PNG,
			    icon_file="icons/bluetooth.png")
	packet.send(sock, addr)
	
        while not done:
                datalen = 0
                try:
                        data = remote.recv(1024)
                        datalen = len(data)
                except:
                        done = True

                if datalen == 13:
                        keycode = data.encode("hex")[10:12]

                        if keycode == "ff":
				release_key()
                                #print "Key Released or Multiple keys pressed"

                        elif keycode == "16": #EJECT
                                print "Eject"
				send_key('S')
                                #os.system("xte 'key s'") 

                        elif keycode == "64": #AUDIO
                                print "Audio"
				send_key('W')
                                #os.system("xte 'key w'")

                        elif keycode == "65": #ANGLE
                                print "Angle"
				send_key('Z')
                                #os.system("xte 'key z'") 

                        elif keycode == "63": #SUBTITLE
                                print "Subtitle"
				send_key('N')
                                #os.system("xte 'key n'") 

                        elif keycode == "0f": #CLEAR
                                print "Clear"
				send_key('D')
                                #os.system("xte 'key d'") 

                        elif keycode == "28": #TIME
                                print "Time"
				send_key('T')
                                #os.system("xte 'key t'") 

                        elif keycode == "00": #1
                                print "1"
				send_key('1')
                                #os.system("xte 'key 1'") 

                        elif keycode == "01": #2
                                print "2"
				send_key('2')
                                #os.system("xte 'key 2'") 

                        elif keycode == "02": #3
                                print "3"
				send_key('3')
                                #os.system("xte 'key 3'") 

                        elif keycode == "03": #4
                                print "4"
				send_key('4')
                                #os.system("xte 'key 4'") 

                        elif keycode == "04": #5
                                print "5"
				send_key('5')
                                #os.system("xte 'key 5'") 

                        elif keycode == "05": #6
                                print "6"
				send_key('6')
                                #os.system("xte 'key 6'") 

                        elif keycode == "06": #7
                                print "7"
				send_key('7')
                                #os.system("xte 'key 7'") 

                        elif keycode == "07": #8
                                print "8"
				send_key('8')
                                #os.system("xte 'key 8'") 

                        elif keycode == "08": #9
                                print "9"
				send_key('9')
                                #os.system("xte 'key 9'") 

                        elif keycode == "09": #0
                                print "0"
				send_key('0')
                                #os.system("xte 'key 0'") 

                        elif keycode == "81": #RED
                                print "Red"

                        elif keycode == "82": #GREEN
                                print "Green"

                        elif keycode == "80": #BLUE
                                print "Blue"

                        elif keycode == "83": #YELLOW
                                print "Yellow"

                        elif keycode == "70": #DISPLAY
                                print "Display"
				send_key('I')
                                #os.system("xte 'key i'") 

                        elif keycode == "1a": #TOP MENU
                                print "Top Menu"

                        elif keycode == "40": #POP UP/MENU
                                print "Pop Up/Menu"

                        elif keycode == "0e": #RETURN
                                print "Return"
				send_key(0x08)
                                #os.system("xte 'key BackSpace'") 

                        elif keycode == "5c": #OPTIONS/TRIANGLE
                                print "Options/Triangle"
				send_key('S')
                                #os.system("xte 'key s'")

                        elif keycode == "5d": #BACK/CIRCLE
                                print "Back/Circle"
				send_key(0x1b)
                                #os.system("xte 'key Escape'") 

                        elif keycode == "5e": #X
                                print "X"
                                #os.system("xte 'key x'") 

                        elif keycode == "5f": #VIEW/SQUARE
                                print "View/Square"
				send_key('V')
                                #os.system("xte 'key v'") 

                        elif keycode == "54": #UP
                                print "Up"
				send_key(0x26)
                                #os.system("xte 'key Up'") 

                        elif keycode == "55": #RIGHT
                                print "Right"
				send_key(0x27)
                                #os.system("xte 'key Right'") 

                        elif keycode == "56": #DOWN
                                print "Down"
				send_key(0x28)
                                #os.system("xte 'key Down'") 

                        elif keycode == "57": #LEFT
                                print "Left"
				send_key(0x25)
                                #os.system("xte 'key Left'") 

                        elif keycode == "0b": #ENTER
                                print "Enter"
				send_key(0x0d)
                                #os.system("xte 'key Return'")

                        elif keycode == "5a": #L1
                                print "L1"
				send_key('+')
                                #os.system("xte 'key +'")

                        elif keycode == "58": #L2
                                print "L2"
				send_key('-')
                                #os.system("xte 'key -'")

                        elif keycode == "51": #L3
                                print "L3"

                        elif keycode == "5b": #R1
                                print "R1"
				send_key(0x21)
                                #os.system("xte 'key Page_Up'") 

                        elif keycode == "59": #R2
                                print "R2"
				send_key(0x22)
                                #os.system("xte 'key Page_Down'") 

                        elif keycode == "52": #R3
				send_key('C')
                                print "R3"

                        elif keycode == "43": #PLAYSTATION
				send_key('S')
                                print "Playstation"

                        elif keycode == "50": #SELECT
                                print "Select"
				send_key('[')
                                #os.system("xte 'key ['")

                        elif keycode == "53": #START
                                print "Start"
				send_key(']')
                                #os.system("xte 'key]'")

                        elif keycode == "33": #<-SCAN
                                print "<-Scan"
				send_key('R')
                                #os.system("xte 'key r'")

                        elif keycode == "34": #  SCAN->
                                print "Scan->"
				send_key('F')
                                #os.system("xte 'key f'")

                        elif keycode == "30": #PREV
                                print "Prev"
				send_key(',')
                                #os.system("xte 'key ,'")

                        elif keycode == "31": #NEXT
                                print "Next"
				send_key('.')
                                #os.system("xte 'key .'")

                        elif keycode == "60": #<-SLOW/STEP
                                print "<-Slow/Step"

                        elif keycode == "61": #  SLOW/STEP->
                                print "Slow/Step->"

                        elif keycode == "32": #PLAY
                                print "Play"
				send_key('p')
                                #os.system("xte 'key p'")

                        elif keycode == "38": #STOP
                                print "Stop"
				send_key('x')
                                #os.system("xte 'key x'")

                        elif keycode == "39": #PAUSE
                                print "Pause"
				send_key(0x20)
                                #os.system("xte 'key Space'")

                        else:
                                print "Unknown data"
                else:
                        print "Unknown data"

        print "Disconnected."
        try:
                remote.close()
        except:
                print "Cannot close."

