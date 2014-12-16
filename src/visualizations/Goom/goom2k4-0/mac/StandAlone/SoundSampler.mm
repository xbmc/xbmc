//
//  SoundSampler.mm
//  iGoom
//
//  Created by Guillaume Borios on Thu May 27 2004.
//  Copyright (c) 2004 iOS. All rights reserved.
//

#import "SoundSampler.h"

#import "CoreAudioHardware.h"
#import "CoreAudioDevice.h"

#include <sys/types.h>
#include <unistd.h>

#define kAudioDeviceNone        0
#define kAudioDeviceUndefined   0xFFFF


OSStatus deviceChanged(AudioDeviceID inDevice, UInt32 /*inChannel*/, Boolean inIsInput, AudioDevicePropertyID inPropertyID, void * deviceController)
{
    if (inIsInput)
    {
        NS_DURING
        
	switch(inPropertyID)
	{
            case kAudioDevicePropertyDeviceIsAlive:
            case kAudioDevicePropertyHogMode:
            case kAudioDevicePropertyDeviceHasChanged:
            {
                [(SoundSampler*)deviceController UpdateDeviceList];
                CoreAudioDevice theDevice(inDevice);
                [(SoundSampler*)deviceController refreshAudioVolumeInterface:theDevice.GetVolumeControlScalarValue(1,kAudioDeviceSectionInput)];
            }
                break;
                
            default:
                break;
	};
	
        NS_HANDLER
        NS_ENDHANDLER
    }
	return 0;
}

OSStatus devicesChanged(AudioHardwarePropertyID property, void * deviceController)
{
    NS_DURING
	
	switch(property)
	{
            case kAudioHardwarePropertyDevices:
                [(SoundSampler*)deviceController UpdateDeviceList];
                break;
                
            default:
                break;
	};
	
    NS_HANDLER
        NS_ENDHANDLER
        
        return 0;
}

static SoundSampler * sharedInstance = nil;

@implementation SoundSampler

-(SoundSampler*)init
{
    self = [super init];
    if (self)
    {
        if (sharedInstance==nil) sharedInstance = self;
        oldDevice = curDevice = kAudioDeviceUndefined;
        BufferIndexReady = 2;
        BufferIndexRead = 0;
        BufferIndexWrite = 1;
        BufferLock = [[NSLock alloc] init];
    }
    return self;
}

+(SoundSampler*)sharedSampler
{
    if (sharedInstance==nil)
    {
        NSLog(@"Error : Sound Sampler invoked to early");
        exit(0);
    }
    return sharedInstance;
}

-(void)awakeFromNib
{
    [ODeviceList setAutoenablesItems:NO];
    [self UpdateDeviceList];
    CoreAudioHardware::AddPropertyListener(kAudioHardwarePropertyDevices, (AudioHardwarePropertyListenerProc)devicesChanged, self);
}

-(void) dealloc
{
    CoreAudioHardware::RemovePropertyListener(kAudioHardwarePropertyDevices, (AudioHardwarePropertyListenerProc)devicesChanged);
    [super dealloc];
}


OSStatus myDeviceProc(AudioDeviceID inDevice, const AudioTimeStamp * inNow,
                      const AudioBufferList * inInputData,
                      const AudioTimeStamp * inInputTime,
                      AudioBufferList * outOutputData, 
                      const AudioTimeStamp * inOutputTime, void * inClientData)
{
    [(SoundSampler*)inClientData updateBuffer:inInputData withDevice:inDevice];
}

#define maxValue 32567.0f

-(void)swapBuffersForRead:(BOOL)read
{
    int tmp;
    
    [BufferLock lock];
    if (read)
    {
        tmp = BufferIndexRead;
        BufferIndexRead = BufferIndexReady;
        BufferIndexReady = tmp;
    }
    else
    {
        tmp = BufferIndexWrite;
        BufferIndexWrite = BufferIndexReady;
        BufferIndexReady = tmp;
    }
    [BufferLock unlock];
}

-(void)updateBuffer:(const AudioBufferList *)inInputData withDevice:(AudioDeviceID)inDevice
{
    // WARNING !!!  This function assumes the input format is (interleaved) Float32 !!!
    int curBuffer;
    int curPosition = 512-1;
    int i;
    for (curBuffer = inInputData->mNumberBuffers-1; (curBuffer >= 0) && (curPosition >= 0); --curBuffer)
    {
        UInt32 channels = inInputData->mBuffers[curBuffer].mNumberChannels;
        UInt32 size = inInputData->mBuffers[curBuffer].mDataByteSize / (sizeof(Float32)*channels);
        if ( (channels > 0) && (size > 0) )
        {
            if (channels == 1)
            {
                // We will duplicate the first channel
                for (i=size-1; (i >=0 ) && (curPosition >= 0); --i)
                {
                    data[BufferIndexWrite][0][curPosition]=(short)(maxValue * ((Float32*)inInputData->mBuffers[curBuffer].mData)[i]);
                    data[BufferIndexWrite][1][curPosition]=data[BufferIndexWrite][0][curPosition];
                    curPosition--;
                }
            }
            else
            {
                // Uses only the 2 first channels
                for (i=size-1; (i >=0 ) && (curPosition >= 1); --i)
                {
                    data[BufferIndexWrite][0][curPosition]=(short)(maxValue * ((Float32*)inInputData->mBuffers[curBuffer].mData)[i]);
                    i--;
                    data[BufferIndexWrite][1][curPosition]=(short)(maxValue * ((Float32*)inInputData->mBuffers[curBuffer].mData)[i]);
                    curPosition--;
                }
            }
        }
    }
    [self swapBuffersForRead:NO];
}

-(void*)getData
{    
    if (oldDevice != curDevice )
    {
        // The device changed
        
        // Stop the old one
        if ( (oldDevice != kAudioDeviceUndefined) && (oldDevice != kAudioDeviceNone) )
        {
            AudioDeviceStop(oldDevice, myDeviceProc);
            AudioDeviceRemoveIOProc(oldDevice, myDeviceProc);
            bzero((void*)data,3*2*512*sizeof(short));
        }
        oldDevice = curDevice;
        
        //Start the new one
        if ( (curDevice != kAudioDeviceUndefined) && (curDevice != kAudioDeviceNone) )
        {
            AudioDeviceAddIOProc(curDevice, myDeviceProc, (void*)self);
            AudioDeviceStart(curDevice, myDeviceProc);
        }
    }

    [self swapBuffersForRead:YES];

    return (void*)(&(data[BufferIndexRead][0][0]));
}




-(IBAction)_changeAudioDevice:(AudioDeviceID)device
{
    if (oldDevice==device) return;
    
    //NSLog(@"Changing audio device from %d to %d",oldDevice,device);
    
    if ( (oldDevice != kAudioDeviceUndefined) && (oldDevice != kAudioDeviceNone) )
    {
        CoreAudioDevice old(oldDevice);
        old.RemovePropertyListener(kAudioPropertyWildcardChannel, kAudioPropertyWildcardSection, kAudioPropertyWildcardPropertyID, (AudioDevicePropertyListenerProc)deviceChanged);
    }
    
    curDevice = device;
    
    if (device == kAudioDeviceNone)
    {
        [OSoundVolume setEnabled:NO];
        [OSoundVolume setFloatValue:0.0f];
    }
    else
    {
        CoreAudioDevice theDevice(device);
        theDevice.AddPropertyListener(kAudioPropertyWildcardChannel, kAudioPropertyWildcardSection, kAudioPropertyWildcardPropertyID, (AudioDevicePropertyListenerProc)deviceChanged, self);
        if (theDevice.HasVolumeControl(1,kAudioDeviceSectionInput))
        {
            [OSoundVolume setEnabled:theDevice.VolumeControlIsSettable(1,kAudioDeviceSectionInput)];
            [OSoundVolume setFloatValue:theDevice.GetVolumeControlScalarValue(1,kAudioDeviceSectionInput)];
        }
        else
        {
            [OSoundVolume setEnabled:NO];
            [OSoundVolume setFloatValue:0.0f];
        }
    }
}

-(IBAction)changeAudioDevice:(id)sender
{
    //NSLog(@"Will change to %@",[[ODeviceList selectedItem]title]);
    [self _changeAudioDevice:[[ODeviceList selectedItem]tag]];
}


-(void)	AddDeviceToMenu:(CoreAudioDevice *)dev
{
    if (dev == nil)
    {
        [ODeviceList addItemWithTitle:[[NSBundle bundleForClass:[self class]] localizedStringForKey:@"None" value:nil table:nil]];
        [[ODeviceList lastItem] setTag:kAudioDeviceNone];
    }
    else
    {
        [ODeviceList addItemWithTitle:@""];
        NSMenuItem* zeItem = [ODeviceList lastItem];
    
        NSString* name = (NSString*)dev->name();
        [zeItem setTitle: [[NSBundle bundleForClass:[self class]] localizedStringForKey:name value:nil table:nil]];
        [name release];
        [zeItem setTag: dev->getDeviceID()];
    }
}


-(void)	UpdateDeviceList
{
    int i,c;
    

    // Cleanup
    [ODeviceList removeAllItems];
    [self AddDeviceToMenu:nil];

    
    // List devices
    int n = CoreAudioHardware::numberOfDevices();
    //NSLog(@"Current device %d",curDevice);
    for(i = 0; i < n; i++)
    {
        CoreAudioDevice theDevice(CoreAudioHardware::deviceAtIndex(i));

        // select audio devices with input streams only        
        if (theDevice.numberOfChannels(kAudioDeviceSectionInput) > 0)
        {
            [self AddDeviceToMenu:&theDevice];
        }
        //NSLog(@"Tag %d : %d",i,[ODeviceList lastItem]);
    }

    // Set up the new selection
    pid_t theHogModeOwner;
    
    
    // Choose the old device, if not hogged...
    c = [ODeviceList indexOfItemWithTag: curDevice];
    if (c != -1)
    {
        CoreAudioDevice dev(CoreAudioHardware::deviceAtIndex(i-1));
        theHogModeOwner = dev.hogModeOwner();
        if ((theHogModeOwner != -1) && (theHogModeOwner != getpid()))
        {
            c = -1;
        }
    }
    
    // Disables all hogged audio inputs, and choose one if necessary and possible
    int m = 1;
    for (i = 0; i < n; i++)
    {
        CoreAudioDevice dev(CoreAudioHardware::deviceAtIndex(i));
        if (dev.numberOfChannels(kAudioDeviceSectionInput) > 0)
        {
            theHogModeOwner = dev.hogModeOwner();
            if ((theHogModeOwner != -1) && (theHogModeOwner != getpid()))
            {
                NS_DURING
                    [[ODeviceList itemAtIndex:m] setEnabled:NO];
                NS_HANDLER
                    //NSLog(@"Exception 1 a pété ! c = %d, i = %d, n = %d, max = %d",c,i,n,[ODeviceList numberOfItems]);
                NS_ENDHANDLER
            }
            else if (c == -1)
            {
                c = m;
                NS_DURING
                    [self _changeAudioDevice:[[ODeviceList itemAtIndex:c] tag]];
                NS_HANDLER
                    //NSLog(@"Exception 2 a pété ! c = %d, i = %d, n = %d, max = %d",c,i,n,[ODeviceList numberOfItems]);
                NS_ENDHANDLER
            }
            m++;
        }
    }
    
    // If nothing could be selected, choose "None"
    if (c == -1)
    {
        c = 0;
        [self _changeAudioDevice:kAudioDeviceNone];
    }
    
    [ODeviceList selectItemAtIndex:c];
}


-(void)refreshAudioVolumeInterface:(float)value
{
    [OSoundVolume setFloatValue:value];
}

-(IBAction)changeAudioVolume:(id)sender
{
    CoreAudioDevice theDevice(curDevice);
    if (theDevice.VolumeControlIsSettable(1,kAudioDeviceSectionInput))
        theDevice.SetVolumeControlScalarValue(1,kAudioDeviceSectionInput,[sender floatValue]);
    if (theDevice.VolumeControlIsSettable(2,kAudioDeviceSectionInput))
        theDevice.SetVolumeControlScalarValue(2,kAudioDeviceSectionInput,[sender floatValue]);
}



@end
