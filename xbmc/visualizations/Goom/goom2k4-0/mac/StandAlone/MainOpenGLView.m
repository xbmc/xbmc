#define IMAGE_SIZE (1<<(int)(log(([self bounds].size.width<[self bounds].size.height)?[self bounds].size.width:[self bounds].size.height)/log(2.0f)))

#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#include "src/goom.h"

#include <QuickTime/ImageCompression.h> // for image loading and decompression
#include <QuickTime/QuickTimeComponents.h> // for file type support

#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLContext.h>
#include <OpenGL/gl.h>

#import "MainOpenGLView.h"

@implementation MainOpenGLView

- (void) dealloc
{
    [super dealloc];
}

- (id)initWithFrame:(NSRect)frameRect
{
    // Init pixel format attribs
    NSOpenGLPixelFormatAttribute attrs[] =
    {
        // Attributes Common to FullScreen and non-FullScreen
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFADepthSize, 16,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFANoRecovery,
        0
    };
    
    // Get pixel format from OpenGL
    NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!pixFmt)
    {
        NSLog(@"No pixel format -- exiting");
        exit(1);
    }
    
    self = [super initWithFrame:frameRect pixelFormat:pixFmt];
    return self;
}
/*
- (void)displayMPixels
{
    static long loop_count = 0;
    struct timeval t2;
    unsigned long t;
    
    loop_count++;
    
    gettimeofday(&t2, NULL);
    t = 1000000 * (t2.tv_sec - cycle_time.tv_sec) + (t2.tv_usec - cycle_time.tv_usec);
    
    // Display the average data rate
    if(t > 1000000 * STAT_UPDATE)
    {
        gettimeofday(&t2, NULL);
        t = 1000000 * (t2.tv_sec - cycle_time.tv_sec) + (t2.tv_usec - cycle_time.tv_usec);
        gettimeofday(&cycle_time, NULL);
        avg_fps = (1000000.0f * (float) loop_count) / (float) t;
        
        loop_count = 0;
        
        gettimeofday(&cycle_time, NULL);
    }
}
*/

- (BOOL)canBeHQ
{
    [[self openGLContext] makeCurrentContext];
    return (strstr(glGetString(GL_EXTENSIONS),"GL_EXT_texture_rectangle") != NULL);
}

 - (void) drawRect:(NSRect)rect
 {
    // Delegate to our scene object for rendering.
    
    [[self openGLContext] makeCurrentContext];
    
    glViewport(0, 0, (GLsizei) rect.size.width, (GLsizei) rect.size.height);

    //GLfloat clear_color[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    //glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    //glClear(GL_COLOR_BUFFER_BIT+GL_DEPTH_BUFFER_BIT+GL_STENCIL_BUFFER_BIT);
    
    [myGoom render];
    
    [[self openGLContext] flushBuffer];
 }


- (void)update  // moved or resized
{
    NSRect rect;
    
    [myGoom setSize:[self bounds].size];

    [super update];
    
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    
    rect = [self bounds];
    
    glViewport(0, 0, (int) rect.size.width, (int) rect.size.height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 

    
    [self setNeedsDisplay:true];
}

- (void)reshape	// scrolled, moved or resized
{
    NSRect rect;
    
    [myGoom setSize:[self bounds].size];

    [super reshape];
    
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    
    //[myGoom setSize:[self bounds].size];

 
    rect = [self bounds];
    
    glViewport(0, 0, (int) rect.size.width, (int) rect.size.height);
    
    //[self loadTextures:GL_FALSE];
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    [self setNeedsDisplay:true];
}

- (IBAction) clientStorage: (id) sender
{
    [myGoom clientStorage:([sender state]==NSOnState)?GL_TRUE:GL_FALSE];

    //[self loadTextures:GL_FALSE];
}

- (IBAction) rectTextures: (id) sender
{
    [myGoom setHighQuality:(rect_texture==NSOnState)?YES:NO];
    
    //[self loadTextures:GL_FALSE];
}

- (IBAction) textureHint: (id) sender
{
    int tag = [[sender selectedItem] tag];
    GLenum    texture_hint = GL_STORAGE_CACHED_APPLE;
    if(tag == 1) texture_hint = GL_STORAGE_PRIVATE_APPLE;
    if(tag == 2) texture_hint = GL_STORAGE_SHARED_APPLE;
    
    [myGoom setTextureHint:texture_hint];
    //[self loadTextures:YES];
}
/*
- (void)loadTextures: (GLboolean)first
{
    NSLog(@"LoadsTExtures");
    PluginInfo * goomInfos = [myGoom infos];

    NSRect rect = [self bounds];
    
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    glEnable(GL_LIGHTING);
                if(rect_texture)
                {
                    if(!first)
                    {
                        GLint dt = 1;
                        glDeleteTextures(1, &dt);
                    }
                    
                    goom_set_resolution (goomInfos, rect.size.width, rect.size.height);
                    
                    glDisable(GL_TEXTURE_2D);
                    glEnable(GL_TEXTURE_RECTANGLE_EXT);
                    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, 1);
                    
                    glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
                    
                    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE , texture_hint);
                    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, client_storage);
                    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                    
                    glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, [self bounds].size.width,
                                 [self bounds].size.height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, [myGoom getGoomDataWithFPS:avg_fps]);
                }
                else
                {
                    glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
                    glTextureRangeAPPLE(GL_TEXTURE_2D, 0, NULL);
                    
                    if(!first)
                    {
                        GLint dt = 1;
                        glDeleteTextures(1, &dt);
                    }
                    
                    goom_set_resolution (goomInfos,IMAGE_SIZE, IMAGE_SIZE);
                    
                    glDisable(GL_TEXTURE_RECTANGLE_EXT);
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, 1);
                    
                    glTextureRangeAPPLE(GL_TEXTURE_2D, 0, NULL);
                    
                    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE , texture_hint);
                    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, client_storage);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                    
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IMAGE_SIZE,
                                 IMAGE_SIZE, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, [myGoom getGoomDataWithFPS:avg_fps]);
                }
}
*/
@end
