/*
 *  CoreAudioHardware.h
 *  iGoom
 *
 *  Created by Guillaume Borios on 14/01/05.
 *  Copyright 2005 iOS Software. All rights reserved.
 *
 */

#include <CoreAudio/CoreAudio.h>


#ifndef COREAUDIOHARDWARE
#define COREAUDIOHARDWARE

class CoreAudioHardware
{
public:
    static UInt32           numberOfDevices();
    static AudioDeviceID    deviceAtIndex(unsigned int i);
public:
    static UInt32           propertyDataSize(AudioHardwarePropertyID property);
    static void             propertyData(AudioHardwarePropertyID property, UInt32 &size, void *data);
    static void             AddPropertyListener(AudioHardwarePropertyID property, AudioHardwarePropertyListenerProc listenerProc, void* inClientData);
    static void             RemovePropertyListener(AudioHardwarePropertyID property, AudioHardwarePropertyListenerProc listenerProc);

};

#endif /*COREAUDIOHARDWARE*/
