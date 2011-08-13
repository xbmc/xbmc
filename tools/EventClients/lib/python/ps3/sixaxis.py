#!/usr/bin/python
# -*- coding: utf-8 -*-

#   Copyright (C) 2008-2009 Team XBMC http://www.xbmc.org
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

import time
import sys
import struct
import math
import binascii
from bluetooth import set_l2cap_mtu
from keymaps import keymap_sixaxis
from keymaps import axismap_sixaxis

xval = 0
yval = 0
num_samples = 16
sumx = [0] * num_samples
sumy = [0] * num_samples
sumr = [0] * num_samples
axis_amount = [0, 0, 0, 0]

def normalize(val):
    upperlimit = 65281
    lowerlimit = 2
    val_range = upperlimit - lowerlimit
    offset = 10000

    val = (val + val_range / 2) % val_range
    upperlimit -= offset
    lowerlimit += offset

    if val < lowerlimit:
        val = lowerlimit
    if val > upperlimit:
        val = upperlimit

    val = ((float(val) - offset) / (float(upperlimit) - 
                                    lowerlimit)) * 65535.0    
    if val <= 0:
        val = 1
    return val

def normalize_axis(val, deadzone):

    val = float(val) - 127.5
    val = val / 127.5

    if abs(val) < deadzone:
      return 0.0

    if val > 0.0:
        val = (val - deadzone) / (1.0 - deadzone)
    else:
        val = (val + deadzone) / (1.0 - deadzone)

    return 65536.0 * val

def normalize_angle(val, valrange):
    valrange *= 2

    val = val / valrange
    if val > 1.0:
        val = 1.0
    if val < -1.0:
        val = -1.0
    return (val + 0.5) * 65535.0

def initialize(control_sock, interrupt_sock):    
    # sixaxis needs this to enable it
    # 0x53 => HIDP_TRANS_SET_REPORT | HIDP_DATA_RTYPE_FEATURE
    control_sock.send("\x53\xf4\x42\x03\x00\x00")
    time.sleep(0.25)
    data = control_sock.recv(1)

    set_l2cap_mtu(control_sock, 64)
    set_l2cap_mtu(interrupt_sock, 64)

    # This command will turn on the gyro and set the leds
    # I wonder if turning on the gyro makes it draw more current??
    # it's probably a flag somewhere in the following command

    # HID Command: HIDP_TRANS_SET_REPORT | HIDP_DATA_RTYPE_OUTPUT
    # HID Report:1
    bytes = [0x52, 0x1] 
    bytes.extend([0x00, 0x00, 0x00])
    bytes.extend([0xFF, 0x72])
    bytes.extend([0x00, 0x00, 0x00, 0x00])
    bytes.extend([0x02]) # 0x02 LED1, 0x04 LED2 ... 0x10 LED4
    # The following sections should set the blink frequncy of
    # the leds on the controller, but i've not figured out how.
    # These values where suggusted in a mailing list, but no explination
    # for how they should be combined to the 5 bytes per led
    #0xFF = 0.5Hz
    #0x80 = 1Hz
    #0x40 = 2Hz
    bytes.extend([0xFF, 0x00, 0x01, 0x00, 0x01]) #LED4 [0xff, 0xff, 0x10, 0x10, 0x10]
    bytes.extend([0xFF, 0x00, 0x01, 0x00, 0x01]) #LED3 [0xff, 0x40, 0x08, 0x10, 0x10]
    bytes.extend([0xFF, 0x00, 0x01, 0x00, 0x01]) #LED2 [0xff, 0x00, 0x10, 0x30, 0x30] 
    bytes.extend([0xFF, 0x00, 0x01, 0x00, 0x01]) #LED1 [0xff, 0x00, 0x10, 0x40, 0x10]
    bytes.extend([0x00, 0x00, 0x00, 0x00, 0x00])
    bytes.extend([0x00, 0x00, 0x00, 0x00, 0x00])

    control_sock.send(struct.pack("42B", *bytes))
    time.sleep(0.25)
    data = control_sock.recv(1)


    return data


def read_input(isock):
    return isock.recv(50)


def process_input(data, xbmc=None, mouse_enabled=0):
    if len(data) < 3:
        return (0, 0, 0, False)

    # make sure this is the correct report
    if struct.unpack("BBB", data[0:3]) != (0xa1, 0x01, 0x00):
        return (0, 0, 0, False)

    if len(data) >= 48:
        v1 = struct.unpack("h", data[42:44])
        v2 = struct.unpack("h", data[44:46])
        v3 = struct.unpack("h", data[46:48])
    else:
        v1 = [0,0]
        v2 = [0,0]
        v3 = [0,0]

    if len(data) >= 50:
        v4 = struct.unpack("h", data[48:50])
    else:
        v4 = [0,0]

    ax = float(v1[0])
    ay = float(v2[0])
    az = float(v3[0])
    rz = float(v4[0])
    at = math.sqrt(ax*ax + ay*ay + az*az)

    bflags = struct.unpack("H", data[3:5])[0]
    psflags = struct.unpack("B", data[5:6])[0]
    if len(data) > 27:
        pressure = struct.unpack("BBBBBBBBBBBB", data[15:27])
    else:
        pressure = [0,0,0,0,0,0,0,0,0,0,0,0,0]

    roll  = -math.atan2(ax, math.sqrt(ay*ay + az*az))
    pitch = math.atan2(ay, math.sqrt(ax*ax + az*az))

    pitch -= math.radians(20);

    xpos = normalize_angle(roll, math.radians(30))
    ypos = normalize_angle(pitch, math.radians(30))

    # update our sliding window array
    sumx.insert(0, xpos)
    sumy.insert(0, ypos)
    sumx.pop(num_samples)
    sumy.pop(num_samples)

    # reset average
    xval = 0
    yval = 0

    # do a sliding window average to remove high frequency
    # noise in accelerometer sampling
    for i in range(0, num_samples):
        xval += sumx[i]
        yval += sumy[i]

    analog = False
    axis = struct.unpack("BBBB", data[7:11])
    if xbmc:
      for i in range(4):
        config = axismap_sixaxis[i]
        axis_amount[i] = send_singleaxis(xbmc, axis[i], axis_amount[i], config[0], config[1], config[2])
        if axis_amount[i] != 0:
          analog = True

      # send the mouse position to xbmc
      if mouse_enabled == 1:
          xbmc.send_mouse_position(xval/num_samples, yval/num_samples)

    return (bflags, psflags, pressure, analog)

def send_singleaxis(xbmc, axis, last_amount, mapname, action_min, action_pos):
    amount = normalize_axis(axis, 0.30)
    if last_amount < 0:
        last_action = action_min
    elif last_amount > 0:
        last_action = action_pos
    else:
        last_action = None

    if amount < 0:
        new_action = action_min
    elif amount > 0:
        new_action = action_pos
    else:
        new_action = None

    if last_action and new_action != last_action:
        xbmc.send_button_state(map=mapname, button=last_action, amount=0, axis=1)

    if new_action and amount != last_amount:
        xbmc.send_button_state(map=mapname, button=new_action, amount=abs(amount), axis=1)

    return amount
