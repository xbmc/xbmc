//
//  SoundSampler.h
//  iGoom
//
//  Created by Guillaume Borios on Thu May 27 2004.
//  Copyright (c) 2004 iOS. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <CoreAudio/CoreAudio.h>


@interface SoundSampler : NSObject {
    
    @private
    
    IBOutlet NSPopUpButton * ODeviceList;
    IBOutlet NSSlider      * OSoundVolume;

    AudioDeviceID oldDevice, curDevice;
    
    signed short data[3][2][512];
    int BufferIndexReady, BufferIndexRead, BufferIndexWrite;
    NSLock * BufferLock;
}

+(SoundSampler*)sharedSampler;
-(void*)getData;
-(void)	UpdateDeviceList;
-(void)updateBuffer:(const AudioBufferList *)inInputData withDevice:(AudioDeviceID)inDevice;

-(IBAction)changeAudioDevice:(id)sender;
-(IBAction)changeAudioVolume:(id)sender;
-(void)refreshAudioVolumeInterface:(float)value;

@end
