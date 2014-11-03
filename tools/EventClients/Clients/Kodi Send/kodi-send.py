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

import sys
import getopt
from socket import *
try:
    from kodi.xbmcclient import *
except:
    sys.path.append('../../lib/python')
    from xbmcclient import *

def usage():
    print "kodi-send [OPTION] --action=ACTION"
    print 'Example'
    print '\tkodi-send --host=192.168.0.1 --port=9777 --action="Quit"'
    print "Options"
    print "\t-?, --help\t\t\tWill bring up this message"
    print "\t--host=HOST\t\t\tChoose what HOST to connect to (default=localhost)"
    print "\t--port=PORT\t\t\tChoose what PORT to connect to (default=9777)"
    print '\t--action=ACTION\t\t\tSends an action to XBMC, this option can be added multiple times to create a macro'
    pass

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "?pa:v", ["help", "host=", "port=", "action="])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    ip = "localhost"
    port = 9777
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
        elif o in ("-a", "--action"):
            actions.append(a)
        else:
            assert False, "unhandled option"
    
    addr = (ip, port)
    sock = socket(AF_INET,SOCK_DGRAM)
    
    if len(actions) is 0:
        usage()
        sys.exit(0)
    
    for action in actions:
        print 'Sending action:', action
        packet = PacketACTION(actionmessage=action, actiontype=ACTION_BUTTON)
        packet.send(sock, addr)

if __name__=="__main__":
    main()
