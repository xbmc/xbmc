/*
 *  CoreAudioDevice.cpp
 *  iGoom
 *
 *  Created by Guillaume Borios on 14/01/05.
 *  Copyright 2005 iOS Software. All rights reserved.
 *
 */

#include "CoreAudioDevice.h"


CoreAudioDevice::CoreAudioDevice(AudioDeviceID devId):deviceID(devId) {}

CoreAudioDevice::~CoreAudioDevice() {}


CFStringRef CoreAudioDevice::name() const
{
    CFStringRef nom;
    try
    {
        UInt32 size = sizeof(CFStringRef);
        propertyData(0, kAudioDeviceSectionGlobal, kAudioDevicePropertyDeviceNameCFString, size, &nom);
    }
    catch(...)
    {
        nom = CFSTR("");
    }
    return nom;
}

UInt32 CoreAudioDevice::propertyDataSize(UInt32 channel, CoreAudioDeviceSection section, AudioHardwarePropertyID property) const
{
    UInt32 size = 0;
    if (AudioDeviceGetPropertyInfo(deviceID, channel, section, property, &size, NULL) != 0)
    {
        fprintf(stderr,"Error while fetching audio device property size. Exiting.");
        exit(0);
    }
    return size;
}

void CoreAudioDevice::propertyData(UInt32 channel, CoreAudioDeviceSection section, AudioHardwarePropertyID property, UInt32 &size, void* data) const
{
    AudioDeviceGetProperty(deviceID, channel, section, property, &size, data) != 0;
}

void CoreAudioDevice::setPropertyData(UInt32 channel, CoreAudioDeviceSection section, AudioHardwarePropertyID property, UInt32 inDataSize, const void* data)
{
    OSStatus theError = AudioDeviceSetProperty(deviceID, NULL, channel, section, property, inDataSize, data);
    //if (theError) fprintf(stderr,"Error");
}

UInt32 CoreAudioDevice::numberOfChannels(CoreAudioDeviceSection section) const
{
    UInt32 n = 0;
    UInt32 size = propertyDataSize(0, section, kAudioDevicePropertyStreamConfiguration);
    AudioBufferList* bufList=(AudioBufferList*)malloc(size);
    
    propertyData(0, section, kAudioDevicePropertyStreamConfiguration, size, bufList);
    for(UInt32 i = 0; i < bufList->mNumberBuffers; ++i)
    {
        n += bufList->mBuffers[i].mNumberChannels;
    }
    free(bufList);
    return n;
}

pid_t	CoreAudioDevice::hogModeOwner() const
{
    pid_t retour = 0;
    UInt32 size = sizeof(pid_t);
    propertyData(0, kAudioDeviceSectionInput, kAudioDevicePropertyHogMode, size, &retour);
    return retour;
}


void	CoreAudioDevice::AddPropertyListener(UInt32 inChannel, CoreAudioDeviceSection inSection, AudioHardwarePropertyID inPropertyID, AudioDevicePropertyListenerProc inListenerProc, void* inClientData)
{
    if (AudioDeviceAddPropertyListener(deviceID, inChannel, inSection, inPropertyID, inListenerProc, inClientData) != 0)
    {
        fprintf(stderr,"Error while Installing device notifications listener. Exiting.");
        exit(0);
    }
}

void	CoreAudioDevice::RemovePropertyListener(UInt32 inChannel, CoreAudioDeviceSection inSection, AudioHardwarePropertyID inPropertyID, AudioDevicePropertyListenerProc inListenerProc)
{
    if (AudioDeviceRemovePropertyListener(deviceID, inChannel, inSection, inPropertyID, inListenerProc) !=0)
    {
        //fprintf(stderr,"Error while Removing device notifications listener. Exiting.");
        //exit(0);
    }
}

// *************************************** VOLUME CONTROL ***************************************

bool CoreAudioDevice::HasVolumeControl(UInt32 channel, CoreAudioDeviceSection section) const
{
    OSStatus theError = AudioDeviceGetPropertyInfo(deviceID, channel, section, kAudioDevicePropertyVolumeScalar, NULL, NULL);
    return (theError == 0);
}

bool CoreAudioDevice::VolumeControlIsSettable(UInt32 channel, CoreAudioDeviceSection section) const
{
        Boolean isWritable = false;
        OSStatus theError = AudioDeviceGetPropertyInfo(deviceID, channel, section, kAudioDevicePropertyVolumeScalar, NULL, &isWritable);
        return (isWritable != 0);
}

Float32 CoreAudioDevice::GetVolumeControlScalarValue(UInt32 channel, CoreAudioDeviceSection section) const
{
    Float32 value = 0.0;
    UInt32 size = sizeof(Float32);
    propertyData(channel, section, kAudioDevicePropertyVolumeScalar, size, &value);
    return value;
}

void CoreAudioDevice::SetVolumeControlScalarValue(UInt32 channel, CoreAudioDeviceSection section, Float32 value)
{
    UInt32 size = sizeof(Float32);
    setPropertyData(channel, section, kAudioDevicePropertyVolumeScalar, size, &value);
}


