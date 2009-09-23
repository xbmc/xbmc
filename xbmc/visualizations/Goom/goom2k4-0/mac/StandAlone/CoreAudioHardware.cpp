/*
 *  CoreAudioHardware.cpp
 *  iGoom
 *
 *  Created by Guillaume Borios on 14/01/05.
 *  Copyright 2005 iOS Software. All rights reserved.
 *
 */

#include "CoreAudioHardware.h"
#include <stdio.h>
#include <stdlib.h>

UInt32 CoreAudioHardware::numberOfDevices()
{
    return ( propertyDataSize(kAudioHardwarePropertyDevices) / sizeof(AudioDeviceID) );
}

AudioDeviceID CoreAudioHardware::deviceAtIndex(unsigned int i)
{
    AudioDeviceID devID = 0;
    int n = numberOfDevices();
    if((n > 0) && (i < n))
    {
        UInt32 size = n * sizeof(AudioDeviceID);
        AudioDeviceID * list = (AudioDeviceID *)malloc(size);
        propertyData(kAudioHardwarePropertyDevices, size, list);
        devID = list[i];
        free(list);
    }
    return devID;
}

UInt32 CoreAudioHardware::propertyDataSize(AudioHardwarePropertyID property)
{
    UInt32 size = 0;
    if (AudioHardwareGetPropertyInfo(property, &size, NULL) != 0)
    {
        fprintf(stderr,"Error while fetching audio property size. Exiting.");
        exit(0);
    }
    return size;
}

void CoreAudioHardware::propertyData(AudioHardwarePropertyID property, UInt32 &size, void *data)
{
    if (AudioHardwareGetProperty(property, &size, data) != 0)
    {
        fprintf(stderr,"Error while fetching audio property. Exiting.");
        exit(0);
    }
}

void	CoreAudioHardware::AddPropertyListener(AudioHardwarePropertyID property, AudioHardwarePropertyListenerProc proc, void* data)
{
    if (AudioHardwareAddPropertyListener(property, proc, data)!= 0)
    {
        fprintf(stderr,"Error could not add a property listener. Exiting.");
        exit(0);
    }
}

void	CoreAudioHardware::RemovePropertyListener(AudioHardwarePropertyID property, AudioHardwarePropertyListenerProc proc)
{
    if (AudioHardwareRemovePropertyListener(property, proc)!= 0)
    {
        fprintf(stderr,"Error could not remove a property listener. Exiting.");
        exit(0);
    }
}
