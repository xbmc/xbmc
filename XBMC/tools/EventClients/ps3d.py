#!/usr/bin/python

import time
import sys
import struct
import threading

from bt.hid import HID
from bt.bt import bt_lookup_name
from xbmcclient import XBMCClient
from ps3 import sixaxis
from ps3_remote import process_keys as process_remote

event_threads = []

class StoppableThread ( threading.Thread ):
    def __init__(self):
        threading.Thread.__init__(self)
        self._stop = False

    def stop_thread(self):
        self._stop = True

    def stop(self):
        return self._stop

    def close_sockets(self):
        if self.isock:
            try:
                self.isock.close()
            except:
                pass
        if self.csock:
            try:
                self.csock.close()
            except:
                pass

    
class PS3SixaxisThread ( StoppableThread ):
    def __init__(self, csock, isock, ip=""):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient("PS3 Sixaxis", "icons/bluetooth.png")

    def run(self):
        sixaxis.initialize(self.csock, self.isock)
        self.xbmc.connect()
        last_connect = time.time()
        bflags = 0
        psflags = 0
        try:
            while not self.stop():
                if (time.time() - last_connect) > 50:
                    self.xbmc.connect()
                try:
                    data = sixaxis.read_input(self.isock)
                except Exception, e:
                    print str(e)
                    break
                if not data:
                    continue
                if psflags:
                    (bflags, psflags) = sixaxis.process_input(data, self.xbmc)
                else:
                    (bflags, psflags) = sixaxis.process_input(data, None)

        except Exception, e:
            print str(e)
        self.close_sockets()


class PS3RemoteThread ( StoppableThread ):
    def __init__(self, csock, isock, ip=""):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient("PS3 BluRay Remote", "icons/bluetooth.png")

    def run(self):
        sixaxis.initialize(self.csock, self.isock)
        self.xbmc.connect()
        last_connect = time.time()
        bflags = 0
        psflags = 0
        try:
            while not self.stop():
                if process_remote(self.isock, self.xbmc):
                    break
        except Exception, e:
            print str(e)
        self.close_sockets()


def usage():
    print """
PS3 Sixaxis / Blu-Ray Remote HID Server v0.1

Usage: ps3.py [bdaddress]

  bdaddress => address of local bluetooth device to use (default: auto)
               (e.g. aa:bb:cc:dd:ee:ff)
"""

def start_hidd(bdaddr=None):
    devices = [ 'PLAYSTATION(R)3 Controller',
                'BD Remote Control' ]
    hid = HID(bdaddr)
    while True:
        if hid.listen():
            (csock, addr) = hid.get_control_socket()
            device_name = bt_lookup_name(addr[0])
            if device_name == devices[0]:
                # handle PS3 controller
                handle_ps3_controller(hid)
            elif device_name == devices[1]:
                # handle the PS3 remote
                handle_ps3_remote(hid)
            else:
                print "Unknown Device: %s" % (device_name)

def handle_ps3_controller(hid):
    print "Received connection from a Sixaxis PS3 Controller"
    csock = hid.get_control_socket()[0]
    isock = hid.get_interrupt_socket()[0]
    sixaxis = PS3SixaxisThread(csock, isock)
    add_thread(sixaxis)
    sixaxis.start()
    return

def handle_ps3_remote(hid):
    print "Received connection from a PS3 Blu-Ray Remote"
    csock = hid.get_control_socket()[0]
    isock = hid.get_interrupt_socket()[0]
    isock.settimeout(1)
    remote = PS3RemoteThread(csock, isock)
    add_thread(remote)
    remote.start()
    return

def add_thread(thread):
    global event_threads
    event_threads.append(thread)

def main():
    if len(sys.argv)>2:
        return usage()
    bdaddr = ""
    try:
        bdaddr = sys.argv[1]
        try:
            # ensure that the addr is of the format 'aa:bb:cc:dd:ee:ff'
            if "".join([ str(len(a)) for a in bdaddr.split(":") ]) != "222222":
                raise Exception("Invalid format")

        except Exception, e:
            print str(e)
            return usage()

    except:
        pass

    print "Starting HID daemon"
    start_hidd(bdaddr)

if __name__=="__main__":        
    try:
        main()
    finally:
        for t in event_threads:
            try:
                print "Waiting for thread "+str(t)+" to terminate"
                t.stop_thread()
                if t.isAlive():
                    t.join()
                print "Thread "+str(t)+" terminated"
                
            except Exception, e:
                print str(e)        
        pass
