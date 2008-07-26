#!/usr/bin/python

import sys
sys.path.append("../PS3 BD Remote")
try:
    from xbmc.bt.hid import HID
    from xbmc.bt.bt import bt_lookup_name
    from xbmc.xbmcclient import XBMCClient
    from xbmc.ps3 import sixaxis
    from xbmc.ps3.keymaps import keymap_sixaxis
    from xbmc.ps3_remote import process_keys as process_remote
    from xbmc.xbmc.defs import *
except:
    sys.path.append("../../lib/python")
    from bt.hid import HID
    from bt.bt import bt_lookup_name
    from xbmcclient import XBMCClient
    from ps3 import sixaxis
    from ps3.keymaps import keymap_sixaxis
    from ps3_remote import process_keys as process_remote
    ICON_PATH = "../../icons/"

import time
import struct
import threading

event_threads = []

class StoppableThread ( threading.Thread ):
    def __init__(self):
        threading.Thread.__init__(self)
        self._stop = False
        self.set_timeout(0)

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
        self.isock = None
        if self.csock:
            try:
                self.csock.close()
            except:
                pass
        self.csock = None
        self.last_action = 0

    def set_timeout(self, seconds):
        self.timeout = seconds

    def reset_timeout(self):
        self.last_action = time.time()

    def idle_time(self):
        return time.time() - self.last_action

    def timed_out(self):
        if (time.time() - self.last_action) > self.timeout:
            return True
        else:
            return False


class PS3SixaxisThread ( StoppableThread ):
    def __init__(self, csock, isock, ip=""):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient("PS3 Sixaxis", ICON_PATH + "/bluetooth.png")
        self.set_timeout(600)

    def run(self):
        sixaxis.initialize(self.csock, self.isock)
        self.xbmc.connect()
        bflags = 0
        last_bflags = 0
        psflags = 0
        psdown = 0
        toggle_mouse = 0
        self.reset_timeout()
        try:
            while not self.stop():
                if self.timed_out():
                    raise Exception("PS3 Sixaxis powering off, timed out")
                if self.idle_time() > 50:
                    self.xbmc.connect()
                try:
                    data = sixaxis.read_input(self.isock)
                except Exception, e:
                    print str(e)
                    break
                if not data:
                    continue
                if psflags:
                    self.reset_timeout()
                    if psdown:
                        if (time.time() - psdown) > 5:
                            raise Exception("PS3 Sixaxis powering off, user request")
                    else:
                        psdown = time.time()
                else:
                    if psdown:
                        toggle_mouse = 1 - toggle_mouse
                    psdown = 0
                (bflags, psflags) = sixaxis.process_input(data, toggle_mouse
                                                          and self.xbmc
                                                          or None)
                if bflags != last_bflags:
                    if bflags:
                        try:
                            self.xbmc.send_keyboard_button(keymap_sixaxis[bflags])
                            self.reset_timeout()
                        except:
                            pass
                    else:
                        self.xbmc.release_button()
                    last_bflags = bflags
        except Exception, e:
            print str(e)
        self.close_sockets()


class PS3RemoteThread ( StoppableThread ):
    def __init__(self, csock, isock, ip=""):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient("PS3 Blu-Ray Remote", ICON_PATH + "/bluetooth.png")
        self.set_timeout(300)

    def run(self):
        sixaxis.initialize(self.csock, self.isock)
        self.xbmc.connect()
        last_connect = time.time()
        bflags = 0
        psflags = 0
        try:
            while not self.stop():
                if process_remote(self.isock, self.xbmc)=="2":
                    if self.timed_out():
                        raise Exception("PS3 Blu-Ray Remote powering off, "\
                                            "timed out")
                elif process_remote(self.isock, self.xbmc):
                    break
                else:
                    self.reset_timeout()
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
