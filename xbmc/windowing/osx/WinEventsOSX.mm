/*
*      Copyright (C) 2011 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "system.h"

#include "utils/log.h"
#include "input/KeymapLoader.h"
#include "windowing/WinEventsSDL.h"
#include "windowing/osx/WinEventsOSX.h"

#import <CoreFoundation/CFNumber.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOMessage.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/usb/IOUSBLib.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <libkern/OSTypes.h>
#import <Carbon/Carbon.h>

// place holder for future native osx event handler

typedef struct USBDevicePrivateData {
  UInt16                vendorId; 
  UInt16                productId;
  CStdString            deviceName;
  CStdString            keymapDeviceId;
  io_object_t           notification;
} USBDevicePrivateData;

static IONotificationPortRef g_notify_port = 0;
static io_iterator_t         g_attach_iterator = 0;

CWinEventsOSX::CWinEventsOSX()
{
  Initialize();
}

CWinEventsOSX::~CWinEventsOSX()
{
  if (g_notify_port)
  {
    // remove the sleep notification port from the application runloop
    CFRunLoopRemoveSource( CFRunLoopGetCurrent(),
      IONotificationPortGetRunLoopSource(g_notify_port), kCFRunLoopDefaultMode );
    IONotificationPortDestroy(g_notify_port);
    g_notify_port = 0;
  }
  if (g_attach_iterator)
  {
    IOObjectRelease(g_attach_iterator);
    g_attach_iterator = 0;
  }
}

bool CWinEventsOSX::MessagePump()
{
  return CWinEventsSDL::MessagePump();
}

static void DeviceDetachCallback(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{ 
  if (messageType == kIOMessageServiceIsTerminated)
  {
    IOReturn result;

    USBDevicePrivateData *privateDataRef = (USBDevicePrivateData*)refCon;
    CKeymapLoader().DeviceRemoved(privateDataRef->keymapDeviceId);
    CLog::Log(LOGDEBUG, "HID Device Detach:%s, %s\n",
      privateDataRef->deviceName.c_str(), privateDataRef->keymapDeviceId.c_str());
    result = IOObjectRelease(privateDataRef->notification);
    delete privateDataRef;
    //release the service
    result = IOObjectRelease(service);
  }
}

static void DeviceAttachCallback(void* refCon, io_iterator_t iterator)
{
  io_service_t usbDevice;
  while ((usbDevice = IOIteratorNext(iterator)))
  {
    IOReturn  result;

    io_name_t deviceName;
    result = IORegistryEntryGetName(usbDevice, deviceName);
		if (result != KERN_SUCCESS)
      deviceName[0] = '\0';

    SInt32 deviceScore;
    IOCFPlugInInterface **devicePlugin;
    result = IOCreatePlugInInterfaceForService(usbDevice,
      kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &devicePlugin, &deviceScore);
    if (result != kIOReturnSuccess)
    {
      IOObjectRelease(usbDevice);
      continue;
    }

    IOUSBDeviceInterface	**deviceInterface;
    // Use the plugin interface to retrieve the device interface.
    result = (*devicePlugin)->QueryInterface(devicePlugin,
      CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID*)&deviceInterface);
    if (result != kIOReturnSuccess)
    {
      IODestroyPlugInInterface(devicePlugin);
      IOObjectRelease(usbDevice);
      continue;
    }

    // get vendor/product ids
    UInt16  vendorId;
    UInt16  productId;
    result = (*deviceInterface)->GetDeviceVendor( deviceInterface, &vendorId);
    result = (*deviceInterface)->GetDeviceProduct(deviceInterface, &productId);

    // we only care about usb devices with an HID interface class
    io_service_t usbInterface;
    io_iterator_t interface_iterator;
    IOUSBFindInterfaceRequest	request;
    request.bInterfaceClass    = kUSBHIDInterfaceClass;
    request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;
    result = (*deviceInterface)->CreateInterfaceIterator(deviceInterface, &request, &interface_iterator);
    while ((usbInterface = IOIteratorNext(interface_iterator)))
    {
      SInt32 interfaceScore;
      IOCFPlugInInterface **interfacePlugin;
      //create intermediate plugin for interface access
      result = IOCreatePlugInInterfaceForService(usbInterface, 
        kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &interfacePlugin, &interfaceScore);
      if (result != kIOReturnSuccess)
      {
        IOObjectRelease(usbInterface);
        continue;
      }
      IOUSBInterfaceInterface** interfaceInterface;
      result = (*interfacePlugin)->QueryInterface(interfacePlugin,
        CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (void**)&interfaceInterface);
      if (result != kIOReturnSuccess)
      {
        IODestroyPlugInInterface(interfacePlugin);
        IOObjectRelease(usbInterface);
        continue;
      }

      // finally we can get to bInterfaceClass/bInterfaceProtocol
      // we should also check for kHIDKeyboardInterfaceProtocol but
      // some IR remotes that emulate an HID keyboard do not report this.
      UInt8 bInterfaceClass;
      result = (*interfaceInterface)->GetInterfaceClass(interfaceInterface, &bInterfaceClass);
      if (bInterfaceClass == kUSBHIDInterfaceClass)
      {
        USBDevicePrivateData *privateDataRef;
        privateDataRef = new USBDevicePrivateData;
        // save some device info to our private data.        
        privateDataRef->vendorId   = vendorId;
        privateDataRef->productId  = productId;
        privateDataRef->deviceName = deviceName;
        privateDataRef->keymapDeviceId.Format("HID#VID_%04X&PID_%04X", vendorId, productId);
        // register this usb device for an interest notification callback. 
        result = IOServiceAddInterestNotification(g_notify_port,
          usbDevice,                      // service
          kIOGeneralInterest,             // interestType
          DeviceDetachCallback,           // callback
          privateDataRef,                 // refCon
          &privateDataRef->notification   // notification
        );
        if (result == kIOReturnSuccess)
        {
          CKeymapLoader().DeviceAdded(privateDataRef->keymapDeviceId);
          CLog::Log(LOGDEBUG, "HID Device Attach:%s, %s\n",
            deviceName, privateDataRef->keymapDeviceId.c_str());
        }

        // done with this device, only need one notification per device.
        IODestroyPlugInInterface(interfacePlugin);
        IOObjectRelease(usbInterface);
        break;
      }
      IODestroyPlugInInterface(interfacePlugin);
      IOObjectRelease(usbInterface);
    }
    IODestroyPlugInInterface(devicePlugin);
    IOObjectRelease(usbDevice);
  }
}

void CWinEventsOSX::Initialize(void)
{
  IOReturn result;

  //set up the matching criteria for the devices we're interested in
  //interested in instances of class IOUSBDevice and its subclasses
  // match any usb device by not creating matching values in the dict
  CFMutableDictionaryRef matching_dict = IOServiceMatching(kIOUSBDeviceClassName);

  g_notify_port = IONotificationPortCreate(kIOMasterPortDefault);
  CFRunLoopSourceRef run_loop_source = IONotificationPortGetRunLoopSource(g_notify_port);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);

  //add a notification callback for attach event
  result = IOServiceAddMatchingNotification(g_notify_port,
    kIOFirstMatchNotification, matching_dict, DeviceAttachCallback, NULL, &g_attach_iterator);
  if (result == kIOReturnSuccess)
  {
    //call the callback to 'arm' the notification
    DeviceAttachCallback(NULL, g_attach_iterator);
  }
}


