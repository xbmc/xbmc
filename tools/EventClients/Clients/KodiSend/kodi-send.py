#!/usr/bin/python
#
# XBMC Media Center
# XBMC Send
# Copyright (c) 2009 team-xbmc
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

import sys, os
import getopt
from socket import *
from time import sleep
try:
    from kodi.xbmcclient import *
except:
    sys.path.append(os.path.join(os.path.realpath(os.path.dirname(__file__)), '../../lib/python'))
    from xbmcclient import *

TYPE_ACTION = 'action'
TYPE_BUTTON = 'button'
TYPE_DELAY = 'delay'

def usage():
    print("Usage")
    print("\tkodi-send [OPTION] --action=ACTION")
    print("\tkodi-send [OPTION] --button=BUTTON")
    print('Example')
    print('\tkodi-send --host=192.168.0.1 --port=9777 --action="Quit"')
    print("Options")
    print("\t-?, --help\t\t\tWill bring up this message")
    print("\t--host=HOST\t\t\tChoose what HOST to connect to (default=localhost)")
    print("\t--port=PORT\t\t\tChoose what PORT to connect to (default=9777)")
    print("\t--keymap=KEYMAP\t\t\tChoose which KEYMAP to use for key presses (default=KB)")
    print('\t--button=BUTTON\t\t\tSends a key press event to Kodi, this option can be added multiple times to create a macro')
    print('\t--action=ACTION\t\t\tSends an action to XBMC, this option can be added multiple times to create a macro')
    print('\t-d T, --delay=T\t\t\tWaits for T ms, this option can be added multiple times to create a macro')
    pass

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "?pad:v", ["help", "host=", "port=", "keymap=", "button=", "action=", "delay="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(str(err)) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    ip = "localhost"
    port = 9777
    keymap = "KB"
    actions = []
    verbose = False
    for o, a in opts:
        if o in ("-?", "--help"):
            usage()
            sys.exit()
        elif o == "--host":
            ip = a
        elif o == "--port":
            port = int(a)
        elif o == "--keymap":
            keymap = a
        elif o == "--button":
            actions.append({'type': TYPE_BUTTON, 'content': a})
        elif o in ("-a", "--action"):
            actions.append({'type': TYPE_ACTION, 'content': a})
        elif o in ("-d", "--delay"):
            actions.append({'type': TYPE_DELAY, 'content': int(a)})
        else:
            assert False, "unhandled option"

    addr = (ip, port)
    sock = socket(AF_INET,SOCK_DGRAM)

    if len(actions) is 0:
        usage()
        sys.exit(0)

    for action in actions:
        print('Sending: %s' % action)
        if action['type'] == TYPE_ACTION:
            packet = PacketACTION(actionmessage=action['content'], actiontype=ACTION_BUTTON)
        elif action['type'] == TYPE_BUTTON:
            packet = PacketBUTTON(code=0, repeat=0, down=1, queue=1, map_name=keymap, button_name=action['content'], amount=0)
        elif action['type'] == TYPE_DELAY:
            time.sleep(action['content'] / 1000.0)
            continue
        packet.send(sock, addr, uid=0)

if __name__=="__main__":
    main()
