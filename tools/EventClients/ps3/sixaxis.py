#!/usr/bin/python

import time
import sys
import struct
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
    v1 = struct.unpack("H", data[42:44])
    v2 = struct.unpack("H", data[44:46])

    bflags = struct.unpack("H", data[3:5])[0]
    psflags = struct.unpack("B", data[5:6])[0]

    xpos = normalize(-v1[0])
    ypos = normalize(v2[0])

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

    return (bflags, psflags)

