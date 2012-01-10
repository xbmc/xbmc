#!/usr/bin/python
# -*- coding: utf-8 -*-

#   Copyright (C) 2008-2009 Team XBMC
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import sys
import traceback
import time
import struct
import threading
import os

if os.path.exists("../../lib/python"):
    sys.path.append("../PS3 BD Remote")
    sys.path.append("../../lib/python")
    from bt.hid import HID
    from bt.bt import bt_lookup_name
    from xbmcclient import XBMCClient
    from ps3 import sixaxis
    from ps3_remote import process_keys as process_remote
    try:
        from ps3 import sixwatch
    except Exception, e:
        print "Failed to import sixwatch now disabled: " + str(e)
        sixwatch = None

    try:
        import zeroconf
    except:
        zeroconf = None
    ICON_PATH = "../../icons/"
else:
    # fallback to system wide modules
    from xbmc.bt.hid import HID
    from xbmc.bt.bt import bt_lookup_name
    from xbmc.xbmcclient import XBMCClient
    from xbmc.ps3 import sixaxis
    from xbmc.ps3_remote import process_keys as process_remote
    from xbmc.defs import *
    try:
        from xbmc.ps3 import sixwatch
    except Exception, e:
        print "Failed to import sixwatch now disabled: " + str(e)
        sixwatch = None
    try:
        import xbmc.zeroconf as zeroconf
    except:
        zeroconf = None


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


class PS3SixaxisThread ( StoppableThread ):
    def __init__(self, csock, isock, ipaddr="127.0.0.1"):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient(name="PS3 Sixaxis", icon_file=ICON_PATH + "/bluetooth.png", ip=ipaddr)
        self.set_timeout(600)

    def run(self):
        six = sixaxis.sixaxis(self.xbmc, self.csock, self.isock)
        self.xbmc.connect()
        self.reset_timeout()
        try:
            while not self.stop():

                if self.timed_out():
                    raise Exception("PS3 Sixaxis powering off, timed out")
                if self.idle_time() > 50:
                    self.xbmc.connect()
                try:
                    if six.process_socket(self.isock):
                        self.reset_timeout()
                except Exception, e:
                    print e
                    break

        except Exception, e:
            printerr()
        six.close()
        self.close_sockets()


class PS3RemoteThread ( StoppableThread ):
    def __init__(self, csock, isock, ipaddr="127.0.0.1"):
        StoppableThread.__init__(self)
        self.csock = csock
        self.isock = isock
        self.xbmc = XBMCClient(name="PS3 Blu-Ray Remote", icon_file=ICON_PATH + "/bluetooth.png", ip=ipaddr)
        self.set_timeout(600)
        self.services = []
        self.current_xbmc = 0

    def run(self):
        self.xbmc.connect()
        try:
            # start the zeroconf thread if possible
            try:
                self.zeroconf_thread = ZeroconfThread()
                self.zeroconf_thread.add_service('_xbmc-events._udp',
                                             self.zeroconf_service_handler)
                self.zeroconf_thread.start()
            except Exception, e:
                print str(e)

            # main thread loop
            while not self.stop():
                status = process_remote(self.isock, self.xbmc)

                if status == 2:   # 2 = socket read timeout
                    if self.timed_out():
                        raise Exception("PS3 Blu-Ray Remote powering off, "\
                                            "timed out")
                elif status == 3: # 3 = ps and skip +
                    self.next_xbmc()

                elif status == 4: # 4 = ps and skip -
                    self.previous_xbmc()

                elif not status:  # 0 = keys are normally processed
                    self.reset_timeout()

        # process_remote() will raise an exception on read errors
        except Exception, e:
            print str(e)

        self.zeroconf_thread.stop()
        self.close_sockets()

    def next_xbmc(self):
        """
        Connect to the next XBMC instance
        """
        self.current_xbmc = (self.current_xbmc + 1) % len( self.services )
        self.reconnect()
        return

    def previous_xbmc(self):
        """
        Connect to the previous XBMC instance
        """
        self.current_xbmc -= 1
        if self.current_xbmc < 0 :
            self.current_xbmc = len( self.services ) - 1
        self.reconnect()
        return

    def reconnect(self):
        """
        Reconnect to an XBMC instance based on self.current_xbmc
        """
        try:
            service = self.services[ self.current_xbmc ]
            print "Connecting to %s" % service['name']
            self.xbmc.connect( service['address'], service['port'] )
            self.xbmc.send_notification("PS3 Blu-Ray Remote", "New Connection", None)
        except Exception, e:
            print str(e)

    def zeroconf_service_handler(self, event, service):
        """
        Zeroconf event handler
        """
        if event == zeroconf.SERVICE_FOUND:  # new xbmc service detected
            self.services.append( service )

        elif event == zeroconf.SERVICE_LOST: # xbmc service lost
            try:
                # search for the service by name, since IP+port isn't available
                for s in self.services:
                    # nuke it, if found
                    if service['name'] == s['name']:
                        self.services.remove(s)
                        break
            except:
                pass
        return

class SixWatch(threading.Thread):
    def __init__(self, mac):
        threading.Thread.__init__(self)
        self.mac = mac
        self.daemon = True
        self.start()
    def run(self):
      while True:
        try:
            sixwatch.main(self.mac)
        except Exception, e:
            print "Exception caught in sixwatch, restarting: " + str(e)

class ZeroconfThread ( threading.Thread ):
    """
    
    """
    def __init__(self):
        threading.Thread.__init__(self)
        self._zbrowser = None
        self._services = []

    def run(self):
        if zeroconf:
            # create zeroconf service browser
            self._zbrowser = zeroconf.Browser()

            # add the requested services
            for service in self._services:
                self._zbrowser.add_service( service[0], service[1] )

            # run the event loop
            self._zbrowser.run()

        return


    def stop(self):
        """
        Stop the zeroconf browser
        """
        try:
            self._zbrowser.stop()
        except:
            pass
        return

    def add_service(self, type, handler):
        """
        Add a new service to search for.
        NOTE: Services must be added before thread starts.
        """
        self._services.append( [ type, handler ] )


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
    watch = None
    if sixwatch:
        try:
            print "Starting USB sixwatch"
            watch = SixWatch(hid.get_local_address())
        except Exception, e:
            print "Failed to initialize sixwatch" + str(e)
            pass

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
    except Exception, e:
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

