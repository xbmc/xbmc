/*
 *  CoreAudioDevice.h
 *  iGoom
 *
 *  Created by Guillaume Borios on 14/01/05.
 *  Copyright 2005 iOS Software. All rights reserved.
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <CoreAudio/CoreAudio.h>

#ifndef COREAUDIODEVICE
#define COREAUDIODEVICE

typedef	UInt8	CoreAudioDeviceSection;
#define	kAudioDeviceSectionInput	((CoreAudioDeviceSection)0x01)
#define	kAudioDeviceSectionOutput	((CoreAudioDeviceSection)0x00)
#define	kAudioDeviceSectionGlobal	((CoreAudioDeviceSection)0x00)
#define	kAudioDeviceSectionWildcard	((CoreAudioDeviceSection)0xFF)

class CoreAudioDevice
{

public:
    CoreAudioDevice(AudioDeviceID devId);
    ~CoreAudioDevice();
    
    AudioDeviceID   getDeviceID() const { return deviceID; }
    CFStringRef     name() const;
    
    UInt32 propertyDataSize(UInt32 channel, CoreAudioDeviceSection section, AudioHardwarePropertyID property) const;
    void propertyData(UInt32 channel, CoreAudioDeviceSection section, AudioHardwarePropertyID property, UInt32 &size, void* data) const;
    void setPropertyData(UInt32 channel, CoreAudioDeviceSection section, AudioHardwarePropertyID property, UInt32 inDataSize, const void* data);

    UInt32 numberOfChannels(CoreAudioDeviceSection section) const;

    pid_t hogModeOwner() const;
    
    void AddPropertyListener(UInt32 inChannel, CoreAudioDeviceSection inSection, AudioHardwarePropertyID inPropertyID, AudioDevicePropertyListenerProc inListenerProc, void* inClientData);
    void RemovePropertyListener(UInt32 inChannel, CoreAudioDeviceSection inSection, AudioHardwarePropertyID inPropertyID, AudioDevicePropertyListenerProc inListenerProc);

    bool CoreAudioDevice::HasVolumeControl(UInt32 channel, CoreAudioDeviceSection section) const;
    bool CoreAudioDevice::VolumeControlIsSettable(UInt32 channel, CoreAudioDeviceSection section) const;
    Float32 CoreAudioDevice::GetVolumeControlScalarValue(UInt32 channel, CoreAudioDeviceSection section) const;
    void CoreAudioDevice::SetVolumeControlScalarValue(UInt32 channel, CoreAudioDeviceSection section, Float32 value);
    
private:
	AudioDeviceID	deviceID;
};


#endif /* COREAUDIODEVICE */