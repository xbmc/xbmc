#!/usr/bin/python

import time
import sys
import struct
import math
from bluetooth import set_l2cap_mtu

xval = 0
yval = 0
num_samples = 16
sumx = [0] * num_samples
sumy = [0] * num_samples


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
    
    return data


def read_input(isock):
    return isock.recv(48)


def process_input(data, xbmc=None):
    if len(data) >= 48:
        v1 = struct.unpack("h", data[42:44])
        v2 = struct.unpack("h", data[44:46])
        v3 = struct.unpack("h", data[46:48])
    else:
        v1 = [0,0]
        v2 = [0,0]
        v3 = [0,0]

    ax = float(v1[0])
    ay = float(v2[0])
    az = float(v3[0])
    at = math.sqrt(ax*ax + ay*ay + az*az)


    bflags = struct.unpack("H", data[3:5])[0]
    psflags = struct.unpack("B", data[5:6])[0]
    preasure = struct.unpack("BBBBBBBBBBBB", data[15:27])

    roll  = -math.atan2(ax, math.sqrt(ay*ay + az*az))
    pitch = math.atan2(ay, math.sqrt(ax*ax + az*az))

    xpos = normalize_angle(roll, math.radians(60))
    ypos = normalize_angle(pitch, math.radians(45))

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

    # send the mouse position to xbmc
    if xbmc:
        xbmc.send_mouse_position(xval/num_samples, yval/num_samples)    

    return (bflags, psflags, preasure)

