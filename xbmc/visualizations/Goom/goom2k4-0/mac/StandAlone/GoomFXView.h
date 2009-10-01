//
//  GoomFXView.h
//  iGoom copie
//
//  Created by Guillaume Borios on Sun Jul 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import <AppKit/AppKit.h>

#include "src/goom.h"

@interface GoomFXView : NSTabView {

    NSMutableArray * params;
    @public
    float height;
}

- (id)initWithFrame:(NSRect)frame andFX:(PluginParameters)FX ;

@end
