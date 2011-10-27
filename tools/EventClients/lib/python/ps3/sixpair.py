#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import usb

vendor  = 0x054c
product = 0x0268
timeout = 5000
passed_value = 0x03f5

def find_sixaxes():
  res = []
  for bus in usb.busses():
    print bus.dirname
    for dev in bus.devices:
      print "-" + dev.filename
      if dev.idVendor == vendor and dev.idProduct == product:
        res.append(dev)
  return res

def find_interface(dev):
  for cfg in dev.configurations:
    for itf in cfg.interfaces:
      for alt in itf:
        if alt.interfaceClass == 3:
          return alt
  raise Exception("Unable to find interface")

def mac_to_string(mac):
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])

def set_pair_filename(dirname, filename, mac):
    for bus in usb.busses():
        if int(bus.dirname) == int(dirname):
            for dev in bus.devices:
                if int(dev.filename) == int(filename):
                    if dev.idVendor == vendor and dev.idProduct == product:
                        update_pair(dev, mac)
                        return
                    else:
                        raise Exception("Device is not a sixaxis")
    raise Exception("Device not found")


def set_pair(dev, mac):
  itf = find_interface(dev)
  handle = dev.open()

  msg = (0x01, 0x00) + mac;

  try:
    handle.detachKernelDriver(itf.interfaceNumber)
  except usb.USBError:
    pass

  handle.claimInterface(itf.interfaceNumber)
  try:
    handle.controlMsg(usb.ENDPOINT_OUT | usb.TYPE_CLASS | usb.RECIP_INTERFACE
                    , usb.REQ_SET_CONFIGURATION, msg, passed_value, itf.interfaceNumber, timeout)
  finally:
    handle.releaseInterface()  


def get_pair(dev):
  itf = find_interface(dev)
  handle = dev.open()

  try:
    handle.detachKernelDriver(itf.interfaceNumber)
  except usb.USBError:
    pass

  handle.claimInterface(itf.interfaceNumber)
  try:
    msg = handle.controlMsg(usb.ENDPOINT_IN | usb.TYPE_CLASS | usb.RECIP_INTERFACE
                          , usb.REQ_CLEAR_FEATURE, 8, passed_value, itf.interfaceNumber, timeout)
  finally:
    handle.releaseInterface()
  return msg[2:8]

def set_pair_all(mac):
    devs = find_sixaxes()
    for dev in devs:
        update_pair(dev, mac)

def update_pair(dev, mac):
    old = get_pair(dev)
    if old != mac:
        print "Reparing sixaxis from:" + mac_to_string(old) + " to:" + mac_to_string(mac)
    set_pair(dev, mac)

if __name__=="__main__":
    devs = find_sixaxes()
    
    for dev in devs:
      msg = get_pair(dev)
      print "Found sixaxis paired to: %02x:%02x:%02x:%02x:%02x:%02x" % (msg[0], msg[1], msg[2], msg[3], msg[4], msg[5]); msg




