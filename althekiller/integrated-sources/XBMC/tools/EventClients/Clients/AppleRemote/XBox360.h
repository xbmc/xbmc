#ifndef __XBOX360_H__
#define __XBOX360_H__

#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDUsageTables.h>

#include <ForceFeedback/ForceFeedback.h>

#include <pthread.h>
#include <string>
#include <list>

#include "AppleRemote.h"
#include "../../lib/c++/xbmcclient.h"

#define SAFELY(expr) if ((expr) != kIOReturnSuccess) { printf("ERROR: \"%s\".\n", #expr); return; }

extern AppleRemote g_appleRemote;

class XBox360Controller
{
 public:
  
  static XBox360Controller* XBox360Controller::Create(io_service_t device, int deviceNum, bool deviceWireless)
  {
    XBox360Controller* controller = new XBox360Controller();
    IOReturn ret;
    IOCFPlugInInterface **plugInInterface;
    SInt32 score=0;

    ret = IOCreatePlugInInterfaceForService(device, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
    if (ret == kIOReturnSuccess)
    {
      ret = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), (LPVOID*)&controller->hidDevice);
      (*plugInInterface)->Release(plugInInterface);
      if (ret == kIOReturnSuccess)
      {
        controller->forceFeedback=0;
        FFCreateDevice(device, &controller->forceFeedback);
        controller->index = deviceNum;
        controller->deviceHandle = device;
        controller->wireless = deviceWireless;
        
        char str[128];
        sprintf(str, "%s Controller %d", deviceWireless ? "Wireless" : "Wired", deviceNum);
        controller->name = str;
        
        return controller;
      }
    }
    
    return 0;
  }
  
  ~XBox360Controller()
  {
    if (deviceHandle != 0) IOObjectRelease(deviceHandle);
    if (hidDevice != 0) (*hidDevice)->Release(hidDevice);
    if (forceFeedback != 0) FFReleaseDevice(forceFeedback);
  }
  
  void start()
  {
    int i,j;
    CFRunLoopSourceRef eventSource;

    // Get serial.
    FFEFFESCAPE escape;
    unsigned char c;
    std::string serial;

    serial = getSerialNumber();
    //printf("SERIAL: %s, forceFeedback: 0x%08lx\n", serial.c_str(), forceFeedback);

    CFArrayRef elements;
    SAFELY((*hidDevice)->copyMatchingElements(hidDevice, NULL, &elements));
    
    for(i=0; i<CFArrayGetCount(elements); i++) 
    {
      CFDictionaryRef element = (CFDictionaryRef)CFArrayGetValueAtIndex(elements, i);
      long usage, usagePage, longCookie;
      
      // Get cookie.
      if (getVal(element, CFSTR(kIOHIDElementCookieKey), longCookie) &&
          getVal(element, CFSTR(kIOHIDElementUsageKey), usage) &&
          getVal(element, CFSTR(kIOHIDElementUsagePageKey), usagePage))
      {
        IOHIDElementCookie cookie = (IOHIDElementCookie)longCookie;
        
        // Match up items
        switch (usagePage) 
        {
          case 0x01:  // Generic Desktop
            j=0;
            switch (usage) 
            {
              case 0x35: j++; // Right trigger
              case 0x32: j++; // Left trigger
              case 0x34: j++; // Right stick Y
              case 0x33: j++; // Right stick X
              case 0x31: j++; // Left stick Y
              case 0x30:      // Left stick X
                axis[j] = cookie;
                break;
              default:
                break;
            }
            break;
            
          case 0x09:  // Button
            if (usage >= 1 && usage <= 15)
            {
              // Button 1-11
              buttons[usage-1] = cookie;
            }
            break;
            
          default:
            break;
        }
      }
    }
    
    // Start queue.
    SAFELY((*hidDevice)->open(hidDevice, 0));
    
    hidQueue = (*hidDevice)->allocQueue(hidDevice);
    if (hidQueue == NULL) 
    {
        printf("Unable to allocate queue\n");
        return;
    }
    
    // Create queue, set callback.
    SAFELY((*hidQueue)->create(hidQueue, 0, 32));
    SAFELY((*hidQueue)->createAsyncEventSource(hidQueue, &eventSource));
    SAFELY((*hidQueue)->setEventCallout(hidQueue, CallbackFunction, this, NULL));

    // Add to runloop.
    CFRunLoopAddSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopCommonModes);
    
    // Add the elements.
    for(i=0; i<6; i++)
      (*hidQueue)->addElement(hidQueue, axis[i], 0);
    for(i=0; i<15; i++)
      (*hidQueue)->addElement(hidQueue, buttons[i], 0);
      
    // Start.
    SAFELY((*hidQueue)->start(hidQueue));

#if 0
    // Read existing properties
    {
//        CFDictionaryRef dict=(CFDictionaryRef)IORegistryEntryCreateCFProperty(registryEntry,CFSTR("DeviceData"),NULL,0);
        CFDictionaryRef dict = (CFDictionaryRef)[GetController(GetSerialNumber(registryEntry)) retain];
        if(dict!=0) {
            CFBooleanRef boolValue;
            CFNumberRef intValue;

            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftX"),(void*)&boolValue)) {
                [leftStickInvertX setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertLeftX");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertLeftY"),(void*)&boolValue)) {
                [leftStickInvertY setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertLeftY");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeLeft"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [leftLinked setState:enable?NSOnState:NSOffState];
                [leftStick setLinked:enable];
            } else NSLog(@"No value for RelativeLeft");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneLeft"),(void*)&intValue)) {
                UInt16 i;

                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [leftStickDeadzone setIntValue:i];
                [leftStick setDeadzone:i];
            } else NSLog(@"No value for DeadzoneLeft");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightX"),(void*)&boolValue)) {
                [rightStickInvertX setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertRightX");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("InvertRightY"),(void*)&boolValue)) {
                [rightStickInvertY setState:CFBooleanGetValue(boolValue)?NSOnState:NSOffState];
            } else NSLog(@"No value for InvertRightY");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("RelativeRight"),(void*)&boolValue)) {
                BOOL enable=CFBooleanGetValue(boolValue);
                [rightLinked setState:enable?NSOnState:NSOffState];
                [rightStick setLinked:enable];
            } else NSLog(@"No value for RelativeRight");
            if(CFDictionaryGetValueIfPresent(dict,CFSTR("DeadzoneRight"),(void*)&intValue)) {
                UInt16 i;

                CFNumberGetValue(intValue,kCFNumberShortType,&i);
                [rightStickDeadzone setIntValue:i];
                [rightStick setDeadzone:i];
            } else NSLog(@"No value for DeadzoneRight");
            CFRelease(dict);
        } else NSLog(@"No settings found");
    }
    
    // Set LED and manual motor control
    // [self updateLED:0x0a];
    [self setMotorOverride:TRUE];
    [self testMotorsLarge:0 small:0];
    largeMotor=0;
    smallMotor=0;
    
    // Battery level?
    {
        NSBundle *bundle = [NSBundle bundleForClass:[self class]];
        NSString *path;
        CFTypeRef prop;

        path = nil;
        if (IOObjectConformsTo(registryEntry, "WirelessHIDDevice"))
        {
            prop = IORegistryEntryCreateCFProperty(registryEntry, CFSTR("BatteryLevel"), NULL, 0);
            if (prop != nil)
            {
                unsigned char level;

                if (CFNumberGetValue(prop, kCFNumberCharType, &level))
                    path = [bundle pathForResource:[NSString stringWithFormat:@"batt%i", level / 64] ofType:@"tif"];
                CFRelease(prop);
            }
        }
        if (path == nil)
            path = [bundle pathForResource:@"battNone" ofType:@"tif"];
        [batteryLevel setImage:[[[NSImage alloc] initByReferencingFile:path] autorelease]];
    }
    }
    #endif
    
    c = 0x0a;
    if (serial.length() > 0)
    {
      #if 0
        for (int i = 0; i < 4; i++)
        {
            if ((leds[i] == nil) || ([leds[i] caseInsensitiveCompare:serial] == NSOrderedSame))
            {
                c = 0x06 + i;
                if (leds[i] == nil)
                {
                    //leds[i] = [serial retain];
                    printf("Added controller with LED %i\n", i);
                  }
                break;
            }
        }
        #endif
    }
    c = 0x06;
    escape.dwSize = sizeof(escape);
    escape.dwCommand = 0x02;
    escape.cbInBuffer = sizeof(c);
    escape.lpvInBuffer = &c;
    escape.cbOutBuffer = 0;
    escape.lpvOutBuffer = NULL;
    if (FFDeviceEscape(forceFeedback, &escape) != FF_OK)
      printf("Error: FF\n");
  }
  
  void stop()
  {
    if (hidQueue != 0)
    {
      // Stop the queue.
      (*hidQueue)->stop(hidQueue);
      
      // Remove from the run loop.
      CFRunLoopSourceRef eventSource = (*hidQueue)->getAsyncEventSource(hidQueue);
      if ((eventSource != 0) && CFRunLoopContainsSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopCommonModes))
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), eventSource, kCFRunLoopCommonModes);
      
      // Whack.
      (*hidQueue)->Release(hidQueue);
      hidQueue = 0;
    }
    
    if (hidDevice != 0)
      (*hidDevice)->close(hidDevice);
  }
    
  std::string getName() { return name; }
  
 private:
 
  XBox360Controller()
  {
    for (int i=0; i<15; i++)
      buttons[i] = 0;
      
    for (int i=0; i<6; i++)
      axis[i] = 0;
  }
  
  std::string getSerialNumber()
  {
      CFTypeRef value = IORegistryEntrySearchCFProperty(deviceHandle, kIOServicePlane, CFSTR("SerialNumber"), kCFAllocatorDefault, kIORegistryIterateRecursively);
      std::string ret = std::string(CFStringGetCStringPtr((CFStringRef)value, kCFStringEncodingMacRoman));
      CFRelease(value);
      
      return ret;
  }
  
  void updateLED(int ledIndex)
  {
    FFEFFESCAPE escape;
    unsigned char c;

    if (forceFeedback ==0 )
      return;
    
    c = ledIndex;
    escape.dwSize = sizeof(escape);
    escape.dwCommand = 0x02;
    escape.cbInBuffer = sizeof(c);
    escape.lpvInBuffer = &c;
    escape.cbOutBuffer = 0;
    escape.lpvOutBuffer = NULL;
    
    if (FFDeviceEscape(forceFeedback, &escape) != FF_OK)
      printf("ERROR: FFDeviceEscape.\n");
  }
    
  static void CallbackFunction(void* me, IOReturn result, void* refCon, void* sender)
  {
    ((XBox360Controller* )me)->handleCallback(result, refCon, sender);
  }
  
  void handleCallback(IOReturn result, void* refCon, void* sender)
  {
    AbsoluteTime zeroTime={0,0};
    IOHIDEventStruct event;
    bool found = false;
    int i;
    
    if (sender != hidQueue) 
      return;
      
    while (result == kIOReturnSuccess) 
    {
      result = (*hidQueue)->getNextEvent(hidQueue, &event, zeroTime, 0);
      if (result != kIOReturnSuccess) 
        continue;
        
      // Check axis
      for (i=0, found=FALSE; (i<6) && (!found); i++) 
      {
        if (event.elementCookie == axis[i]) 
        {  
          int amount = 0;
          if (i == 4 || i == 5)
            amount = event.value * (32768/256) + 32768;
          else
            amount = 65536 - (event.value + 32768) - 1;
            
          // Clip.
          if (amount > 65535)
            amount = 65535;
          if (amount < 0)
            amount = 0;
          
          if (g_appleRemote.IsVerbose())
            printf("Axis %d %d.\n", i+1, amount);
          CPacketBUTTON btn(i+1, "JS1:Wireless 360 Controller", BTN_AXIS | BTN_NO_REPEAT | BTN_USE_AMOUNT | BTN_QUEUE, amount);
          g_appleRemote.SendPacket(btn);
          
          found = true;
        }
      }
        
      if (found) continue;
      
      // Check buttons
      for (i=0, found=FALSE; (i<15) && (!found); i++) 
      {
        if (event.elementCookie == buttons[i])
        {
			if (g_appleRemote.IsVerbose())
				printf("Button: %d %d.\n", i+1, event.value);
          
			if (i+1 == 11 && !g_appleRemote.IsProgramRunning("XBMC", 0) && 
				(g_appleRemote.GetServerAddress() == "127.0.0.1" || g_appleRemote.GetServerAddress() == "localhost") && 
				event.value)
			{
				g_appleRemote.LaunchApp();
				return;
			}
          
			int flags = event.value ? BTN_DOWN : BTN_UP;
			CPacketBUTTON btn(i+1, "JS1:Wireless 360 Controller", flags, 0);
			g_appleRemote.SendPacket(btn);
			found = true;
        }
      }
    
      if (found) continue;
      
      // Cookie wasn't for us?
    }
  }
    
  bool getVal(CFDictionaryRef element, const CFStringRef key, long& value)
  {
    CFTypeRef object = CFDictionaryGetValue(element, key);
    
    if ((object == NULL) || (CFGetTypeID(object) != CFNumberGetTypeID())) return false;
    if (!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType, &value)) return false;
    
    return true;
  }

  IOHIDDeviceInterface122 **hidDevice;
  FFDeviceObjectReference forceFeedback;
  io_service_t deviceHandle;
  
  IOHIDQueueInterface **hidQueue;
  IOHIDElementCookie axis[6];
  IOHIDElementCookie buttons[15];
  
  std::string name;
  int index;
  bool wireless;
};


class XBox360
{
 public:
  
  XBox360()
  {
  }
    
  void start()
  {
    pthread_create(&_itsThread, NULL, Run, (void *)this);
  }
  
  void join()
  {
    void* val = 0;
    pthread_join(_itsThread, &val);
  }
    
 protected:
    
  static void* Run(void* param)
  {
    ((XBox360* )param)->run();
  }
  
  void run()
  {
    printf("XBOX360: Registering for notifications.\n");
    io_object_t object;
    
    // Register for wired notifications.
    IONotificationPortRef notificationObject = IONotificationPortCreate(kIOMasterPortDefault);
    IOServiceAddMatchingNotification(notificationObject, kIOFirstMatchNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackHandleDevice, this, &_itsOnIteratorWired);
    callbackHandleDevice(this, _itsOnIteratorWired);
    
    IOServiceAddMatchingNotification(notificationObject, kIOTerminatedNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackHandleDevice, this, &_itsOffIteratorWired);
    while ((object = IOIteratorNext(_itsOffIteratorWired)) != 0)
      IOObjectRelease(object);
                       
    // Wireless notifications.
    IOServiceAddMatchingNotification(notificationObject, kIOFirstMatchNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, this, &_itsOnIteratorWireless);
    callbackHandleDevice(this, _itsOnIteratorWireless);
                                       
    IOServiceAddMatchingNotification(notificationObject, kIOTerminatedNotification, IOServiceMatching("WirelessHIDDevice"), callbackHandleDevice, this, &_itsOffIteratorWireless);
    while ((object = IOIteratorNext(_itsOffIteratorWireless)) != 0)
      IOObjectRelease(object);                                  
      
    // Add notifications to the run loop.                               
    CFRunLoopSourceRef notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationObject);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationRunLoopSource, kCFRunLoopDefaultMode);
    
    // Run.
    CFRunLoopRun();
    printf("XBOX360: Exiting.\n");
  }
  
 private:
 
  // Callback for Xbox360 device notifications.
  static void callbackHandleDevice(void* param, io_iterator_t iterator)
  {
    io_service_t object = 0;
    bool update = false;

    while ((object = IOIteratorNext(iterator))!=0) 
    {
      IOObjectRelease(object);
      update = true;
    }

    if (update)
      ((XBox360* )param)->updateDeviceList();
  }
  
  void updateDeviceList()
  {
    io_iterator_t iterator;
    io_object_t hidDevice;

    // Scrub and whack old items.
    stopDevices();

    for (std::list<XBox360Controller*>::iterator i = controllerList.begin(); i != controllerList.end(); ++i)
      delete *i;
    controllerList.clear();
    
    // Add new items
    CFMutableDictionaryRef hidDictionary = IOServiceMatching(kIOHIDDeviceKey);
    IOReturn ioReturn = IOServiceGetMatchingServices(kIOMasterPortDefault, hidDictionary, &iterator);
    
    if ((ioReturn != kIOReturnSuccess) || (iterator==0))
    {
      printf("No devices.\n");
      return;
    }
    
    for (int count = 1; hidDevice = IOIteratorNext(iterator); ) 
    {
       bool deviceWired = IOObjectConformsTo(hidDevice, "Xbox360ControllerClass");
       bool deviceWireless = IOObjectConformsTo(hidDevice, "WirelessHIDDevice");
       
       if (deviceWired || deviceWireless)
       {
         XBox360Controller* controller = XBox360Controller::Create(hidDevice, count, deviceWireless);
         if (controller)
         {
           // Add to list.
           printf("Adding device: %s.\n", controller->getName().c_str());
           controllerList.push_back(controller);
           count++;
         }
       }
       else
       {
         IOObjectRelease(hidDevice);
       }
    }
    
    IOObjectRelease(iterator);
      
    // Make sure all devices are started.
    startDevices();
  }
  
  void stopDevices()
  {
    for (std::list<XBox360Controller*>::iterator i = controllerList.begin(); i != controllerList.end(); ++i)
      (*i)->stop();
  }
  
  void startDevices()
  {
    for (std::list<XBox360Controller*>::iterator i = controllerList.begin(); i != controllerList.end(); ++i)
      (*i)->start();
  }
  
  std::list<XBox360Controller*> controllerList;
  pthread_t     _itsThread;
  io_iterator_t _itsOnIteratorWired, _itsOffIteratorWired;
  io_iterator_t _itsOnIteratorWireless, _itsOffIteratorWireless;
};

#endif
