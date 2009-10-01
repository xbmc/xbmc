//
//  GoomFXView.m
//  iGoom copie
//
//  Created by Guillaume Borios on Sun Jul 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import "GoomFXView.h"
#import "GoomFXParam.h"


@implementation GoomFXView

- (id)initWithFrame:(NSRect)frame andFX:(PluginParameters)FX {
    self = [super initWithFrame:frame];
    if (self) {
        int i;
        height = 15.0f;
        params = [[NSMutableArray alloc] init];
        for (i=0; i < FX.nbParams; i++)
        {
            if (FX.params[i] != NULL)
            {
                GoomFXParam * FXP = [[[GoomFXParam alloc]initWithParam:FX.params[i]]autorelease];
                [params addObject:FXP];
                [self addSubview:[FXP makeViewAtHeight:height]];
                height += 29.0f;
            }
            else
            {
                height += 12.0f;
            }
        }
    }
    
    height += 73.0f;
    
    return self;
}

- (void)resizeWindow
{
    NSWindow * parent = [self window];
    NSRect frame = [parent frame];
    if (frame.size.height != height)
    {
        frame.origin.y -= height-frame.size.height;
        frame.size.height = height;
        [parent setFrame:frame display:NO animate:NO];
    }
}

- (void)drawRect:(NSRect)rect {
    // Drawing code here.
}

-(void)dealloc
{
    [params release];

    [super dealloc];
}

@end
