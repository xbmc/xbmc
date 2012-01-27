#!/usr/bin/python
# -*- coding: utf-8 -*-

import pyudev
import sixpair
import threading

vendor  = 0x054c
product = 0x0268


def main(mac):
    context = pyudev.Context()
    monitor = pyudev.Monitor.from_netlink(context)
    monitor.filter_by(subsystem="usb")
    for action, device in monitor:
        if 'ID_VENDOR_ID' in device and 'ID_MODEL_ID' in device:
            if device['ID_VENDOR_ID'] == '054c' and device['ID_MODEL_ID'] == '0268':
                if action == 'add':
                    print "Detected sixaxis connected by usb"
                    try:
                        sixpair.set_pair_filename(device.attributes['busnum'], device.attributes['devnum'], mac)
                    except Exception, e:
                        print "Failed to check pairing of sixaxis: " + str(e)
                        pass



if __name__=="__main__":
    main((0,0,0,0,0,0))

