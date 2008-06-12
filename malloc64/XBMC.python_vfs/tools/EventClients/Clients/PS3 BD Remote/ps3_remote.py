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

import sys
try:
    from xbmc.xbmcclient import *
    from xbmc.ps3.keymaps import keymap_remote as g_keymap # look here to change the keymapping
    from xbmc.bt.bt import *
    from xbmc.defs import *
except:
    sys.path.append("../../lib/python")
    from xbmcclient import *
    from ps3.keymaps import keymap_remote as g_keymap # look here to change the keymapping
    from bt.bt import *
    ICON_PATH = "../../icons/"

import os
import time

xbmc = None
bticon = ICON_PATH + "/bluetooth.png"

def get_remote_address(remote, target_name = "BD Remote Control"):
    global xbmc
    target_connected = False
    target_address = None
    while target_connected is False:
        xbmc.send_notification("Action Required!",
                               "Hold Start+Enter on your remote.",
                               bticon)
        print "Searching for %s" % target_name
        print "(Hold Start + Enter on remote to make it discoverable)"
        time.sleep(2)

        if not target_address:
            try:
                nearby_devices = bt_discover_devices()
            except Exception, e:
                print "Error performing bluetooth discovery"
                print str(e)
                xbmc.send_notification("Error", "Unable to find devices.", bticon)
                time.sleep(5)
                continue

            for bdaddr in nearby_devices:
                bname = bt_lookup_name( bdaddr )
                addr = bt_lookup_addr ( bdaddr )
                print "%s (%s) in range" % (bname,addr)
                if target_name == bname:
                    target_address = addr
                    break

        if target_address is not None:
            print "Found %s with address %s" % (target_name, target_address)
            xbmc.send_notification("Found Device",
                                   "Pairing %s, please wait." % target_name,
                                   bticon)
            print "Attempting to pair with remote"

            try:
                remote.connect((target_address,19))
                target_connected = True
                print "Remote Paired.\a"
                xbmc.send_notification("Pairing Successfull",
                                       "Your remote was successfully "\
                                           "paired and is ready to be used.",
                                       bticon)
            except:
                del remote
                remote = bt_create_socket()
                target_address = None
                xbmc.send_notification("Pairing Failed",
                                       "An error occurred while attempting to "\
                                           "pair.", bticon)
                print "ERROR - Could Not Connect. Trying again..."
                time.sleep(2)
        else:
            xbmc.send_notification("Error", "No remotes were found.", bticon)
            print "Could not find BD Remote Control. Trying again..."
            time.sleep(2)
    return (remote,target_address)


def usage():
    print """
PS3 Blu-Ray Remote Control Client for XBMC v0.1

Usage: ps3_remote.py <address> [port]

  address => address of system that XBMC is running on
             ("localhost" if it is this machine)

     port => port to send packets to
             (default 9777)
"""

def process_keys(remote, xbmc):
    done = False

    xbmc.connect()
    datalen = 0
    try:
        data = remote.recv(1024)
        datalen = len(data)
    except Exception, e:
        if str(e)=="timed out":
            return "2"
        time.sleep(2)
        done = True

    if datalen == 13:
        keycode = data.encode("hex")[10:12]
        if keycode == "ff":
            xbmc.release_button()
            return done
        try:
            if g_keymap[keycode]:
                xbmc.send_remote_button(g_keymap[keycode])
        except Exception, e:
            print "Unknown data: %s" % str(e)
    return done

def main():
    global xbmc, bticon

    host = "127.0.0.1"
    port = 9777

    if len(sys.argv)>1:
        try:
            host = sys.argv[1]
            port = sys.argv[2]
        except:
            pass
    else:
        return usage()

    loop_forever = True
    xbmc = XBMCClient("PS3 Bluetooth Remote",
                      icon_file=bticon)

    while loop_forever is True:
        target_connected = False
        remote = bt_create_socket()
        xbmc.connect(host, port)
        (remote,target_address) = get_remote_address(remote)
        while True:
            if process_keys(remote, xbmc):
                break
        print "Disconnected."
        try:
            remote.close()
        except:
            print "Cannot close."

if __name__=="__main__":
    main()
