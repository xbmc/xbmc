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

SX_SELECT   = 1 << 0
SX_L3       = 1 << 1
SX_R3       = 1 << 2
SX_START    = 1 << 3
SX_DUP      = 1 << 4
SX_DRIGHT   = 1 << 5
SX_DDOWN    = 1 << 6
SX_DLEFT    = 1 << 7
SX_L2       = 1 << 8
SX_R2       = 1 << 9
SX_L1       = 1 << 10
SX_R1       = 1 << 11
SX_TRIANGLE = 1 << 12
SX_CIRCLE   = 1 << 13
SX_X        = 1 << 14
SX_SQUARE   = 1 << 15
SX_POWER    = 1 << 16

SX_LSTICK_X  = 0
SX_LSTICK_Y  = 1
SX_RSTICK_X  = 2
SX_RSTICK_Y  = 3

# (map, key, amount index, axis)
keymap_sixaxis = {
    SX_X        : ('XG', 'A', 0, 0),
    SX_CIRCLE   : ('XG', 'B', 0, 0),
    SX_SQUARE   : ('XG', 'X', 0, 0),
    SX_TRIANGLE : ('XG', 'Y', 0, 0),

    SX_DUP      : ('XG', 'dpadup', 0, 0),
    SX_DDOWN    : ('XG', 'dpaddown', 0, 0),
    SX_DLEFT    : ('XG', 'dpadleft', 0, 0),
    SX_DRIGHT   : ('XG', 'dpadright', 0, 0),

    SX_START    : ('XG', 'start', 0, 0),
    SX_SELECT   : ('XG', 'back', 0, 0),

    SX_R1       : ('XG', 'white', 0, 0),
    SX_R2       : ('XG', 'rightanalogtrigger', 6, 1),
    SX_L2       : ('XG', 'leftanalogtrigger', 5, 1),
    SX_L1       : ('XG', 'black', 0, 0),

    SX_L3       : ('XG', 'leftthumbbutton', 0, 0),
    SX_R3       : ('XG', 'rightthumbbutton', 0, 0),
}

# (data index, left map, left action, right map, right action)
axismap_sixaxis = {
    SX_LSTICK_X : ('XG', 'leftthumbstickleft' , 'leftthumbstickright'),
    SX_LSTICK_Y : ('XG', 'leftthumbstickup'   , 'leftthumbstickdown'),
    SX_RSTICK_X : ('XG', 'rightthumbstickleft', 'rightthumbstickright'),
    SX_RSTICK_Y : ('XG', 'rightthumbstickup'  , 'rightthumbstickdown'),
}

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

def average(array):
    val = 0
    for i in array:
      val += i
    return val / len(array)
    
def smooth(arr, val):
    cnt = len(arr)
    arr.insert(0, val)
    arr.pop(cnt)
    return average(arr)    

def set_l2cap_mtu2(sock, mtu):
  SOL_L2CAP = 6
  L2CAP_OPTIONS = 1
  
  s = sock.getsockopt (SOL_L2CAP, L2CAP_OPTIONS, 12)
  o = list( struct.unpack ("HHHBBBH", s) )
  o[0] = o[1] = mtu
  s = struct.pack ("HHHBBBH", *o)
  try:
    sock.setsockopt (SOL_L2CAP, L2CAP_OPTIONS, s)
  except:
    print "Warning: Unable to set mtu"

class sixaxis():
    
  def __init__(self, xbmc, control_sock, interrupt_sock):

    self.xbmc = xbmc
    self.num_samples = 16
    self.sumx = [0] * self.num_samples
    self.sumy = [0] * self.num_samples
    self.sumr = [0] * self.num_samples
    self.axis_amount = [0, 0, 0, 0]
    
    self.released = set()
    self.pressed  = set()
    self.pending  = set()
    self.held     = set()
    self.psflags  = 0
    self.psdown   = 0
    self.mouse_enabled = 0
    
    set_l2cap_mtu2(control_sock, 64)
    set_l2cap_mtu2(interrupt_sock, 64)
    time.sleep(0.25) # If we ask to quickly here, it sometimes doesn't start

    # sixaxis needs this to enable it
    # 0x53 => HIDP_TRANS_SET_REPORT | HIDP_DATA_RTYPE_FEATURE
    control_sock.send("\x53\xf4\x42\x03\x00\x00")
    data = control_sock.recv(1)
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
    data = control_sock.recv(1)

  def __del__(self):
    self.close()

  def close(self):

    for key in (self.held | self.pressed):
        (mapname, action, amount, axis) = keymap_sixaxis[key]
        self.xbmc.send_button_state(map=mapname, button=action, amount=0, down=0, axis=axis)
    self.held    = set()
    self.pressed = set()


  def process_socket(self, isock):
    data = isock.recv(50)
    if data == None:
      return False
    return self.process_data(data)


  def process_data(self, data):
    if len(data) < 3:
        return False

    # make sure this is the correct report
    if struct.unpack("BBB", data[0:3]) != (0xa1, 0x01, 0x00):
        return False

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

    bflags = struct.unpack("<I", data[3:7])[0]
    if len(data) > 27:
        pressure = struct.unpack("BBBBBBBBBBBB", data[15:27])
    else:
        pressure = [0,0,0,0,0,0,0,0,0,0,0,0,0]

    roll  = -math.atan2(ax, math.sqrt(ay*ay + az*az))
    pitch = math.atan2(ay, math.sqrt(ax*ax + az*az))

    pitch -= math.radians(20);

    xpos = normalize_angle(roll, math.radians(30))
    ypos = normalize_angle(pitch, math.radians(30))
    

    axis = struct.unpack("BBBB", data[7:11])
    return self.process_input(bflags, pressure, axis, xpos, ypos)

  def process_input(self, bflags, pressure, axis, xpos, ypos):

    xval = smooth(self.sumx, xpos)
    yval = smooth(self.sumy, ypos)

    analog = False
    for i in range(4):
        config = axismap_sixaxis[i]
        self.axis_amount[i] = self.send_singleaxis(axis[i], self.axis_amount[i], config[0], config[1], config[2])
        if self.axis_amount[i] != 0:
            analog = True

    # send the mouse position to xbmc
    if self.mouse_enabled == 1:
        self.xbmc.send_mouse_position(xval, yval)

    if (bflags & SX_POWER) == SX_POWER:
        if self.psdown:
            if (time.time() - self.psdown) > 5:

                for key in (self.held | self.pressed):
                    (mapname, action, amount, axis) = keymap_sixaxis[key]
                    self.xbmc.send_button_state(map=mapname, button=action, amount=0, down=0, axis=axis)

                raise Exception("PS3 Sixaxis powering off, user request")
        else:
            self.psdown = time.time()
    else:
        if self.psdown:
            self.mouse_enabled = 1 - self.mouse_enabled
        self.psdown = 0

    keys     = set(getkeys(bflags))
    self.released = (self.pressed | self.held) - keys
    self.held     = (self.pressed | self.held) - self.released
    self.pressed  = (keys - self.held) & self.pending
    self.pending  = (keys - self.held)

    for key in self.released:
        (mapname, action, amount, axis) = keymap_sixaxis[key]
        self.xbmc.send_button_state(map=mapname, button=action, amount=0, down=0, axis=axis)

    for key in self.held:
        (mapname, action, amount, axis) = keymap_sixaxis[key]
        if amount > 0:
            amount = pressure[amount-1] * 256
            self.xbmc.send_button_state(map=mapname, button=action, amount=amount, down=1, axis=axis)

    for key in self.pressed:
        (mapname, action, amount, axis) = keymap_sixaxis[key]
        if amount > 0:
            amount = pressure[amount-1] * 256
        self.xbmc.send_button_state(map=mapname, button=action, amount=amount, down=1, axis=axis)

    if analog or keys or self.mouse_enabled:
      return True
    else:
      return False


  def send_singleaxis(self, axis, last_amount, mapname, action_min, action_pos):
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
        self.xbmc.send_button_state(map=mapname, button=last_action, amount=0, axis=1)

    if new_action and amount != last_amount:
        self.xbmc.send_button_state(map=mapname, button=new_action, amount=abs(amount), axis=1)

    return amount
