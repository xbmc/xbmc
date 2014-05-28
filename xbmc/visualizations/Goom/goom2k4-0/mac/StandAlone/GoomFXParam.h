//
//  GoomFXParam.h
//  iGoom copie
//
//  Created by Guillaume Borios on Sun Jul 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#ifndef STANDALONE_SRC_GOOM_H_INCLUDED
#define STANDALONE_SRC_GOOM_H_INCLUDED
#include "src/goom.h"
#endif


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
