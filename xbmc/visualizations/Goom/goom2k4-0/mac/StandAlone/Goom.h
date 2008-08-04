//
//  Goom.h
//  iGoom
//
//  Created by Guillaume Borios on Sat Dec 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/glu.h>
#include <OpenGL/glext.h>
#include "src/goom.h"

@interface Goom : NSObject {

    PluginInfo * goomInfos;
    
    gint16 sndData[2][512];
    
    GLuint textureName;
    AbsoluteTime	backUpTime;
    
    GLenum    texture_hint;
    GLboolean client_storage;

    NSSize curSize;
    BOOL HQ;
    BOOL dirty;
}

- (void)setSize:(NSSize)_size;

- (PluginInfo*)infos;

-(void)render;

-(void)setHighQuality:(BOOL)quality;
- (void)prepareTextures;

- (guint32 *)getGoomDataWithFPS:(float)fps;

-(void)setTextureHint:(GLenum)hint;
-(void)clientStorage:(GLboolean)client;

@end
