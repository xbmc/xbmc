//
//  Goom.m
//  iGoom
//
//  Created by Guillaume Borios on Sat Dec 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import "Goom.h"
#import <OpenGL/glu.h>
#include <OpenGL/glext.h>
#import "SoundSampler.h"

@implementation Goom

unsigned int IMAGE_SIZE(NSSize curSize)
{
    int retour =   (1<<(int)(log((curSize.width>curSize.height)?curSize.width:curSize.height)/log(2.0f)));
    if (retour > 512) retour = 512;
    return retour;
}

-(Goom*)init
{
    self = [super init];
    
    if (self != nil)
    {
        curSize = NSMakeSize(16,16);
        HQ = NO;
        dirty = YES;
        goomInfos = goom_init(16,16);
        backUpTime = UpTime();
        
        client_storage = GL_TRUE;
        texture_hint = GL_STORAGE_SHARED_APPLE;
    }
    //NSLog(@"Goom Init");
    return self;
}

-(Goom*)initWithSize:(NSSize)_size
{
    self = [super init];
    
    if (self != nil)
    {
        curSize = _size;
        HQ = NO;
        dirty = YES;
        backUpTime = UpTime();
    }
    //NSLog(@"Goom Init with Size : %f x %f",_size.width,_size.height);
    return self;
}

- (PluginInfo*)infos
{
    return goomInfos;
}

-(void)setSize:(NSSize)_size
{
    if ((curSize.width != _size.width) || (curSize.height != _size.height))
    {
        //NSLog(@"Set size (%f,%f)",_size.width, _size.height);
        curSize = _size;
        dirty = YES;
    }
}

-(void)render
{
    if (dirty==YES) [self prepareTextures]; 

    //glClear(0);
    
    // Bind, update and draw new image
    if(HQ==YES)
    {
        //NSLog(@"Render HQ (%f,%f)",curSize.width, curSize.height);
        
        glEnable(GL_TEXTURE_RECTANGLE_EXT);
        
        glBindTexture(GL_TEXTURE_RECTANGLE_EXT, textureName);
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0, curSize.width, curSize.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, [self getGoomDataWithFPS:0]);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(0.0f, curSize.height);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(curSize.width, curSize.height);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(curSize.width, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glEnd();
        
        glDisable(GL_TEXTURE_RECTANGLE_EXT);

    }
    else
    {
        int loggedSize = IMAGE_SIZE(curSize);
        float ratio = curSize.width/curSize.height;
        
        //NSLog(@"Render LQ (%f,%f / %d)",curSize.width, curSize.height,IMAGE_SIZE(curSize));
 
        glEnable(GL_TEXTURE_2D);
        
	//glColor4f(0.0, 0.5, 0.1, 0.0);
        glBindTexture(GL_TEXTURE_2D, textureName);
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, loggedSize, loggedSize, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, [self getGoomDataWithFPS:0]);

        //glShadeModel(GL_SMOOTH);
        
        glBegin(GL_POLYGON);
        
        if (ratio>1.0f)
        {
            float o=0.5f-0.5f/ratio;
            float y=1.0f/ratio+o;
            //glColor4f(0.0, 0.5, 0.1, 1.0);
            glTexCoord2f(0.0f, o);
            glVertex2f(-1.0f, 1.0f);
            //glColor4f(0.3, 0.5, 0.1, 0.5);
            glTexCoord2f(0.0f, y);
            glVertex2f(-1.0f, -1.0f);
            //glColor4f(0.6, 0.5, 0.1, 0.2);
            glTexCoord2f(1.0f, y);
            glVertex2f(1.0f, -1.0f);
            //glColor4f(0.9, 0.5, 0.1, 0.0);
            glTexCoord2f(1.0f, o);
            glVertex2f(1.0f, 1.0f);
        }
        else
        {
            float o=0.5f-0.5f*ratio;
            float x=ratio+o;
            //glColor4f(0.0, 0.5, 0.1, 1.0);
            glTexCoord2f(o, 0.0f);
            glVertex2f(-1.0f, 1.0f);
            //glColor4f(0.3, 0.5, 0.1, 0.5);
            glTexCoord2f(o, 1.0f);
            glVertex2f(-1.0f, -1.0f);
            //glColor4f(0.6, 0.5, 0.1, 0.2);
            glTexCoord2f(x, 1.0f);
            glVertex2f(1.0f, -1.0f);
            //glColor4f(0.9, 0.5, 0.1, 0.0);
            glTexCoord2f(x, 0.0f);
            glVertex2f(1.0f, 1.0f);
        }
        
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
        //glShadeModel(GL_FLAT);

        /*
        glBegin(GL_QUADS);
        
	glColor4f(0.0, 0.1, 0.0, 1.0);
	glVertex3f(-1.0, -1.0, 0.0);
	glVertex3f( 1.0, -1.0, 0.0);
        
	glColor4f(0.0, 0.5, 0.1, 1.0);
	glVertex3f( 1.0,  1.0, 0.0);
	glVertex3f(-1.0,  1.0, 0.0);
        
	glEnd();*/
        
    }
  
    //glFlush();
}


-(guint32 *)getGoomDataWithFPS:(float)fps
{
    
    if (fps>0) return goom_update(goomInfos, [[SoundSampler sharedSampler] getData], 0, fps, 0, 0);
    else return goom_update(goomInfos, [[SoundSampler sharedSampler] getData], 0, 0, 0, 0);
 //   return goom_update(goomInfos, sndData, 0, fps, 0, 0);
    
}



-(void)setHighQuality:(BOOL)quality
{
    HQ = quality;
    dirty = YES;
}

- (void)prepareTextures
{
    //static BOOL first = YES;
    //[[self openGLContext] update];
    
    // Setup some basic OpenGL stuff
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);    
    
    dirty = NO;
    
    glViewport(0, 0, (int) curSize.width, (int) curSize.height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    //glEnable(GL_LIGHTING);
                                

    if(HQ==YES)
    {
        //NSLog(@"Prepare HQ (%f,%f)",curSize.width, curSize.height);
        
        //glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
        
        glDeleteTextures(1, &textureName);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_RECTANGLE_EXT);
        
        glGenTextures( 1, &textureName );
        glBindTexture(GL_TEXTURE_RECTANGLE_EXT, textureName);
        
        glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
        
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE , client_storage);
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, texture_hint);
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        goom_set_resolution (goomInfos, curSize.width, curSize.height);
        glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, curSize.width, curSize.height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, [self getGoomDataWithFPS:0]);
    }
    else
    {
        //NSLog(@"Prepare LQ (%f,%f)",curSize.width, curSize.height);
        
        //glTextureRangeAPPLE(GL_TEXTURE_2D, 0, NULL);
                
        glDeleteTextures(1, &textureName);
        glDisable(GL_TEXTURE_RECTANGLE_EXT);
        glEnable(GL_TEXTURE_2D);
        
        glGenTextures( 1, &textureName );
        glBindTexture(GL_TEXTURE_2D, textureName);
        
        glTextureRangeAPPLE(GL_TEXTURE_2D, 0, NULL);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_STORAGE_HINT_APPLE , client_storage);
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, texture_hint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        goom_set_resolution (goomInfos, IMAGE_SIZE(curSize), IMAGE_SIZE(curSize));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IMAGE_SIZE(curSize), IMAGE_SIZE(curSize), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, [self getGoomDataWithFPS:0]);
        
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

-(void)setTextureHint:(GLenum)hint
{
    texture_hint = hint;
}

-(void)clientStorage:(GLboolean)client
{
    client_storage = client;
}


- (void)dealloc
{
    goom_close(goomInfos);
    [super dealloc];
}



@end
