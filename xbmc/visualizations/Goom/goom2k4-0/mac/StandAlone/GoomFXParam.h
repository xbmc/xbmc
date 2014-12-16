//
//  GoomFXParam.h
//  iGoom copie
//
//  Created by Guillaume Borios on Sun Jul 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "src/goom.h"

@interface GoomFXParam : NSObject {
    PluginParam * parametres;
    NSProgressIndicator * progress;
    NSSlider * slider;
    NSButton * button;

    NSTextField * value;
}

- (GoomFXParam*)initWithParam:(PluginParam*)p;
- (NSView*)makeViewAtHeight:(float)h;
- (void)setValue:(id)sender;

@end
