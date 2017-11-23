# -*- coding: utf-8 -*-
#   Copyright (C) 2008-2013 Team XBMC
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

from bluetooth import *
import fcntl
import bluetooth._bluetooth as _bt
import array

class HID:
    def __init__(self, bdaddress=None):
        self.cport = 0x11  # HID's control PSM
        self.iport = 0x13  # HID' interrupt PSM
        self.backlog = 1

        self.address = ""
        if bdaddress:
            self.address = bdaddress

        # create the HID control socket
        self.csock = BluetoothSocket( L2CAP )
        self.csock.bind((self.address, self.cport))
        set_l2cap_mtu(self.csock, 64)
        self.csock.settimeout(2)
        self.csock.listen(self.backlog)

        # create the HID interrupt socket
        self.isock = BluetoothSocket( L2CAP )
        self.isock.bind((self.address, self.iport))
        set_l2cap_mtu(self.isock, 64)
        self.isock.settimeout(2)
        self.isock.listen(self.backlog)

        self.connected = False


    def listen(self):
        try:
            (self.client_csock, self.caddress) = self.csock.accept()
            print("Accepted Control connection from %s" % self.caddress[0])
            (self.client_isock, self.iaddress) = self.isock.accept()
            print("Accepted Interrupt connection from %s" % self.iaddress[0])
            self.connected = True
            return True
        except Exception as e:
            self.connected = False
            return False

    def get_local_address(self):
        hci = BluetoothSocket( HCI )
        fd  = hci.fileno()
        buf = array.array('B', [0] * 96)
        fcntl.ioctl(fd, _bt.HCIGETDEVINFO, buf, 1)
        data = struct.unpack_from("H8s6B", buf.tostring())
        return data[2:8][::-1]

    def get_control_socket(self):
        if self.connected:
            return (self.client_csock, self.caddress)
        else:
            return None


    def get_interrupt_socket(self):
        if self.connected:
            return (self.client_isock, self.iaddress)
        else:
            return None
