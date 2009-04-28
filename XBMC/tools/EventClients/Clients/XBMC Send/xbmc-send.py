#!/usr/bin/python

import sys
import getopt
from socket import *
try:
    from xbmc.xbmcclient import *
except:
    sys.path.append('../../lib/python')
    from xbmcclient import *

def usage():
    print "xbmc-send [OPTION] --action=ACTION"
    print 'Example'
    print '\txbmc-send --ip=192.168.0.1 --p 9777 --action="XBMC.Quit"'
    print "Options"
    print "\t--help, -h.\t\t\tWill bring up this message"
    print "\t-i IP, --ip=IP.\t\t\tChoose what IP to connect to (default=localhost)"
    print "\t-p PORT, --port=PORT.\t\tChoose what PORT to connect to (default=9777)"
    pass

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hipa:v", ["help", "ip=", "port=", "action="])
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
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-i", "--ip"):
            ip = a
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
