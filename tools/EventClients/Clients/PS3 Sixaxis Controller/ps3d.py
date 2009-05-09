#!/usr/bin/python

import sys
import traceback

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

def printerr():
	trace = ""
	exception = ""
	exc_list = traceback.format_exception_only (sys.exc_type, sys.exc_value)
	for entry in exc_list:
		exception += entry
	tb_list = traceback.format_tb(sys.exc_info()[2])
	for entry in tb_list:
		trace += entry	
	print("%s\n%s" % (exception, trace), "Script Error")



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

# to make sure all combination keys are checked first
# we sort the keymap's button codes in reverse order
# this guranties that any bit combined button code
# will be processed first
keymap_sixaxis_keys = keymap_sixaxis.keys()
keymap_sixaxis_keys.sort()
keymap_sixaxis_keys.reverse()

def getkeys(bflags):
    keys = [];
    for k in keymap_sixaxis_keys:
        if (k & bflags) == k:
            keys.append(k)
            bflags = bflags & ~k
    return keys;

class PS3SixaxisThread ( StoppableThread ):
    def __init__(self, csock, isock, ipaddr="127.0.0.1"):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient(name="PS3 Sixaxis", icon_file=ICON_PATH + "/bluetooth.png", ip=ipaddr)
        self.set_timeout(600)

    def run(self):
        sixaxis.initialize(self.csock, self.isock)
        self.xbmc.connect()
        bflags = 0
        released = set()
        pressed  = set()
        pending  = set()
        held     = set()
        psflags = 0
        psdown = 0
        toggle_mouse = 0
        self.reset_timeout()
        try:
            while not self.stop():
                if self.timed_out():

                    for key in (held | pressed):
                        (mapname, action, amount, axis) = keymap_sixaxis[key]
                        self.xbmc.send_button_state(map=mapname, button=action, amount=0, down=0, axis=axis)

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

                (bflags, psflags, pressure) = sixaxis.process_input(data, self.xbmc, toggle_mouse)

                if psflags:
                    self.reset_timeout()
                    if psdown:
                        if (time.time() - psdown) > 5:

                            for key in (held | pressed):
                                (mapname, action, amount, axis) = keymap_sixaxis[key]
                                self.xbmc.send_button_state(map=mapname, button=action, amount=0, down=0, axis=axis)
  
                            raise Exception("PS3 Sixaxis powering off, user request")
                    else:
                        psdown = time.time()
                else:
                    if psdown:
                        toggle_mouse = 1 - toggle_mouse
                    psdown = 0

                keys = set(getkeys(bflags))
                released = (pressed | held) - keys
                held     = (pressed | held) - released
                pressed  = (keys - held) & pending
                pending  = (keys - held)

                for key in released:
                    (mapname, action, amount, axis) = keymap_sixaxis[key]
                    self.xbmc.send_button_state(map=mapname, button=action, amount=0, down=0, axis=axis)

                for key in held:
                    (mapname, action, amount, axis) = keymap_sixaxis[key]
                    if amount > 0:
                        amount = pressure[amount-1] * 256
                        self.xbmc.send_button_state(map=mapname, button=action, amount=amount, down=1, axis=axis)

                for key in pressed:
                    (mapname, action, amount, axis) = keymap_sixaxis[key]
                    if amount > 0:
                        amount = pressure[amount-1] * 256
                    self.xbmc.send_button_state(map=mapname, button=action, amount=amount, down=1, axis=axis)

                if keys:
                    self.reset_timeout()


        except Exception, e:
            printerr()
        self.close_sockets()


class PS3RemoteThread ( StoppableThread ):
    def __init__(self, csock, isock, ipaddr="127.0.0.1"):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient(name="PS3 Blu-Ray Remote", icon_file=ICON_PATH + "/bluetooth.png", ip=ipaddr)
        self.set_timeout(300)

    def run(self):
        sixaxis.initialize(self.csock, self.isock)
        self.xbmc.connect()
        try:
            while not self.stop():
                status = process_remote(self.isock, self.xbmc)

                if status == 2:   # 2 = socket read timeout
                    if self.timed_out():
                        raise Exception("PS3 Blu-Ray Remote powering off, "\
                                            "timed out")

                elif not status:  # 0 = keys are normally processed
                    self.reset_timeout()

        # process_remote() will raise an exception on read errors
        except Exception, e:
            print str(e)
        self.close_sockets()


def usage():
    print """
PS3 Sixaxis / Blu-Ray Remote HID Server v0.1

Usage: ps3.py [bdaddress] [XBMC host]

  bdaddress  => address of local bluetooth device to use (default: auto)
                (e.g. aa:bb:cc:dd:ee:ff)
  ip address => IP address or hostname of the XBMC instance (default: localhost)
                (e.g. 192.168.1.110)
"""

def start_hidd(bdaddr=None, ipaddr="127.0.0.1"):
    devices = [ 'PLAYSTATION(R)3 Controller',
                'BD Remote Control' ]
    hid = HID(bdaddr)
    while True:
        if hid.listen():
            (csock, addr) = hid.get_control_socket()
            device_name = bt_lookup_name(addr[0])
            if device_name == devices[0]:
                # handle PS3 controller
                handle_ps3_controller(hid, ipaddr)
            elif device_name == devices[1]:
                # handle the PS3 remote
                handle_ps3_remote(hid, ipaddr)
            else:
                print "Unknown Device: %s" % (device_name)

def handle_ps3_controller(hid, ipaddr):
    print "Received connection from a Sixaxis PS3 Controller"
    csock = hid.get_control_socket()[0]
    isock = hid.get_interrupt_socket()[0]
    sixaxis = PS3SixaxisThread(csock, isock, ipaddr)
    add_thread(sixaxis)
    sixaxis.start()
    return

def handle_ps3_remote(hid, ipaddr):
    print "Received connection from a PS3 Blu-Ray Remote"
    csock = hid.get_control_socket()[0]
    isock = hid.get_interrupt_socket()[0]
    isock.settimeout(1)
    remote = PS3RemoteThread(csock, isock, ipaddr)
    add_thread(remote)
    remote.start()
    return

def add_thread(thread):
    global event_threads
    event_threads.append(thread)

def main():
    if len(sys.argv)>3:
        return usage()
    bdaddr = ""
    ipaddr = "127.0.0.1"
    try:
        for addr in sys.argv[1:]:
            try:
                # ensure that the addr is of the format 'aa:bb:cc:dd:ee:ff'
                if "".join([ str(len(a)) for a in addr.split(":") ]) != "222222":
                    raise Exception("Invalid format")
                bdaddr = addr
                print "Connecting to Bluetooth device: %s" % bdaddr
            except Exception, e:
                try:
                    ipaddr = addr
                    print "Connecting to : %s" % ipaddr
                except:
                    print str(e)
                    return usage()
    except:
        pass

    print "Starting HID daemon"
    start_hidd(bdaddr, ipaddr)

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
