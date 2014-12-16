#import "AppController.h"
#import "GoomFXView.h"
#include "src/goom.h"

#import <OpenGL/OpenGL.h>

@interface AppController (AnimationMethods)
- (BOOL) isAnimating;
- (void) startAnimation;
- (void) stopAnimation;
- (void) toggleAnimation;

- (void) startAnimationTimer;
- (void) stopAnimationTimer;
- (void) animationTimerFired:(NSTimer *)timer;
@end

static float HowLong(AbsoluteTime * backUpTime)
{
    AbsoluteTime absTime;
    Nanoseconds nanosec;
    
    absTime = SubAbsoluteFromAbsolute(UpTime(), *backUpTime);
    nanosec = AbsoluteToNanoseconds(absTime);
    //fprintf(stderr,"Time :  %f\n", (float) UnsignedWideToUInt64( nanosec ) / 1000000000.0);
    //fprintf(stderr,"FPS :  %f\n", (float) 1000000000.0f/UnsignedWideToUInt64( nanosec ));
    *backUpTime = UpTime();
    return (float) (1000000000.0f/UnsignedWideToUInt64( nanosec ));
}

static void logGLError(int line)
{
    GLenum err = glGetError();
    char * code;
    
    if (err == GL_NO_ERROR) return;
    
    switch (err)
    {
        case GL_INVALID_ENUM:
            code = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            code = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            code = "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            code = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            code = "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            code = "GL_OUT_OF_MEMORY";
            break;
        default:
            code = "Unknown Error";
            break;
    }
    fprintf(stderr,"iGoom OpenGL error : %s", code);
    
}

@implementation AppController

-(void) awakeFromNib
{
    PluginInfo * goomInfos;
    int i;
    
    goomInfos = [myGoom infos];
    
    for (i=0; i < goomInfos->nbParams; i++)
    {
        NSTabViewItem * item = [[[NSTabViewItem alloc] initWithIdentifier:nil] autorelease];
        [item setLabel:[NSString stringWithCString:goomInfos->params[i].name]];
        [item setView:[[[GoomFXView alloc] initWithFrame:[TabView contentRect] andFX:goomInfos->params[i]] autorelease]];
        [TabView addTabViewItem:item];
        
        // Create and load textures for the first time
        //[GLView loadTextures:GL_TRUE];
    }

    //[self goFullScreen:self];
    isAnimating = NO;
    lastFPS = 0.0f;
    backUpTime=UpTime();
    FrameRate = 0.028f; // ~35 FPS
    
    if ([GLView canBeHQ])
    {
        [HQButton setEnabled:YES];
        [QEWarning removeFromSuperview];
    }
    
    [self startAnimation];
}


// Action method wired up to fire when the user clicks the "Go FullScreen" button.  We remain in this method until the user exits FullScreen mode.
- (IBAction) goFullScreen:(id)sender
{
    CFAbsoluteTime timeNow;
    CGLContextObj cglContext;
    CGDisplayErr err;
    long oldSwapInterval;
    long newSwapInterval;
    BOOL plugrunning = YES;
    long rendererID;

    // Pixel Format Attributes for the FullScreen NSOpenGLContext
    NSOpenGLPixelFormatAttribute attrs[] = {

        // Specify that we want a full-screen OpenGL context.
        NSOpenGLPFAFullScreen,

        // We may be on a multi-display system (and each screen may be driven by a different renderer),
        // so we need to specify which screen we want to take over.
        // For this demo, we'll specify the main screen.
        NSOpenGLPFAScreenMask, CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay),

        // Attributes Common to FullScreen and non-FullScreen
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFADepthSize, 16,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        0
    };

    // Create the FullScreen NSOpenGLContext with the attributes listed above.
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];    
    if (pixelFormat == nil) {
        NSLog(@"Failed to create 1st pixelFormat, trying another format...");
        NSOpenGLPixelFormatAttribute attrs2[] = {
            NSOpenGLPFAFullScreen,
            NSOpenGLPFAScreenMask, CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay),
            0
        };
        
        // Create the FullScreen NSOpenGLContext with the attributes listed above.
        NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs2];    
        if (pixelFormat == nil) {
            NSLog(@"Failed to create 2nd pixelFormat, canceling full screen mode.");
            return;
        }
    }
    
    // Just as a diagnostic, report the renderer ID that this pixel format binds to.
    // CGLRenderers.h contains a list of known renderers and their corresponding RendererID codes.
    [pixelFormat getValues:&rendererID forAttribute:NSOpenGLPFARendererID forVirtualScreen:0];
    
    // Create an NSOpenGLContext with the FullScreen pixel format.
    // By specifying the non-FullScreen context as our "shareContext",
    // we automatically inherit all of the textures, display lists, and other OpenGL objects it has defined.
    fullScreenContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:[GLView openGLContext]];
    [pixelFormat release];
    pixelFormat = nil;
    
    if (fullScreenContext == nil) {
        NSLog(@"Failed to create fullScreenContext");
        return;
    }
    
    // Pause animation in the OpenGL view.
    // While we're in full-screen mode, we'll drive the animation actively instead of using a timer callback.
    if ([self isAnimating]) {
        [self stopAnimationTimer];
    }

    // Take control of the display where we're about to go FullScreen.
    err = CGCaptureAllDisplays();
    if (err != CGDisplayNoErr) {
        [fullScreenContext release];
        fullScreenContext = nil;
        return;
    }

    // Enter FullScreen mode and make our FullScreen context the active context for OpenGL commands.
    [fullScreenContext setFullScreen];
    [fullScreenContext makeCurrentContext];

    // Save the current swap interval so we can restore it later, and then set the new swap interval to lock us to the display's refresh rate.
    cglContext = CGLGetCurrentContext();
    CGLGetParameter(cglContext, kCGLCPSwapInterval, &oldSwapInterval);
    newSwapInterval = 1;
    CGLSetParameter(cglContext, kCGLCPSwapInterval, &newSwapInterval);

    // Tell the myGoom the dimensions of the area it's going to render to, so it can set up an appropriate viewport and viewing transformation.
    [myGoom setSize:NSMakeSize(CGDisplayPixelsWide(kCGDirectMainDisplay), CGDisplayPixelsHigh(kCGDirectMainDisplay))];

    // Now that we've got the screen, we enter a loop in which we alternately process input events and computer and render the next frame of our animation.  The shift here is from a model in which we passively receive events handed to us by the AppKit to one in which we are actively driving event processing.
    timeBefore = CFAbsoluteTimeGetCurrent();
    stayInFullScreenMode = YES;
    while (stayInFullScreenMode) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

        // Check for and process input events.
        NSEvent *event;
        while (event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES]) {
            switch ([event type]) {
                case NSLeftMouseDown:
                    [self mouseDown:event];
                    break;

                case NSLeftMouseUp:
                    plugrunning = plugrunning?NO:YES;
                    [self mouseUp:event];
                    break;

                case NSRightMouseUp:
                    plugrunning = plugrunning?NO:YES;
                    break;
                    
                case NSLeftMouseDragged:
                    [self mouseDragged:event];
                    break;

                case NSKeyDown:
                    [self keyDown:event];
                    break;

                default:
                    break;
            }
        }

        // Render a frame, and swap the front and back buffers.
        timeNow = CFAbsoluteTimeGetCurrent();
        if ((timeNow-timeBefore) >= FrameRate)
        {
            timeBefore = timeNow;
            if (plugrunning==YES) {
                [myGoom render];
                [fullScreenContext flushBuffer];
            }
        }


        // Clean up any autoreleased objects that were created this time through the loop.
        [pool release];
    }
    
    // Clear the front and back framebuffers before switching out of FullScreen mode.  (This is not strictly necessary, but avoids an untidy flash of garbage.)
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    [fullScreenContext flushBuffer];
    glClear(GL_COLOR_BUFFER_BIT);
    [fullScreenContext flushBuffer];

    // Restore the previously set swap interval.
    CGLSetParameter(cglContext, kCGLCPSwapInterval, &oldSwapInterval);

    // Exit fullscreen mode and release our FullScreen NSOpenGLContext.
    [NSOpenGLContext clearCurrentContext];
    [fullScreenContext clearDrawable];
    [fullScreenContext release];
    fullScreenContext = nil;

    // Release control of the display.
    CGReleaseAllDisplays();

    // Reset the size to the window size
    [myGoom setSize:[GLView frame].size];
    
    // Mark our view as needing drawing.  (The animation has advanced while we were in FullScreen mode, so its current contents are stale.)
    [GLView setNeedsDisplay:YES];

    // Resume animation timer firings.
    if ([self isAnimating]) {
        [self startAnimationTimer];
    }
}

- (IBAction) setHighQuality:(id)sender
{
    [myGoom setHighQuality:([sender state]==NSOnState)];
}

- (IBAction) setFrameRate:(id)sender
{
    FrameRate = 1.0f/[sender floatValue];
    [self stopAnimation];
    [self startAnimation];
}

- (void) keyDown:(NSEvent *)event
{
    unichar c = [[event charactersIgnoringModifiers] characterAtIndex:0];
    switch (c) {

        // [Esc] exits FullScreen mode.
        case 27:
            stayInFullScreenMode = NO;
            break;

        // [space] toggles rotation of the globe.
        case 32:
            [self toggleAnimation];
            break;

        case 'l':
        case 'L':
            [myGoom setHighQuality:NO];
            break;
            
        case 'h':
        case 'H':
            [myGoom setHighQuality:YES];
            break;
            
        default:
            break;
    }
}
/*
- (void)mouseDown:(NSEvent *)theEvent
{
    BOOL wasAnimating = [self isAnimating];
    BOOL dragging = YES;
    NSPoint windowPoint;
    NSPoint lastWindowPoint = [theEvent locationInWindow];
    float dx, dy;

    if (wasAnimating) {
        [self stopAnimation];
    }
    while (dragging) {
        theEvent = [[GLView window] nextEventMatchingMask:NSLeftMouseUpMask | NSLeftMouseDraggedMask];
        windowPoint = [theEvent locationInWindow];
        switch ([theEvent type]) {
            case NSLeftMouseUp:
                dragging = NO;
                break;

            case NSLeftMouseDragged:
                dx = windowPoint.x - lastWindowPoint.x;
                dy = windowPoint.y - lastWindowPoint.y;
                lastWindowPoint = windowPoint;

                // Render a frame.
                if (fullScreenContext) {
                    [myGoom render];
                    [fullScreenContext flushBuffer];
                } else {
                    [GLView display];
                }
                break;

            default:
                break;
        }
    }
    if (wasAnimating) {
        [self startAnimation];
        timeBefore = CFAbsoluteTimeGetCurrent();
    }
}
*/
- (BOOL) isInFullScreenMode
{
    return fullScreenContext != nil;
}

@end

@implementation AppController (AnimationMethods)

- (BOOL) isAnimating
{
    return isAnimating;
}

- (void) startAnimation
{
    if (!isAnimating) {
        isAnimating = YES;
        if (![self isInFullScreenMode])
        {
            [self startAnimationTimer];
        }
    }
}

- (void) stopAnimation
{
    if (isAnimating) {
        if (animationTimer != nil) {
            [self stopAnimationTimer];
        }
        isAnimating = NO;
    }
}

- (void) toggleAnimation
{
    if ([self isAnimating]) [self stopAnimation];
    else [self startAnimation];
}

- (void) startAnimationTimer
{
    if (animationTimer == nil) {
        animationTimer = [[NSTimer scheduledTimerWithTimeInterval:FrameRate target:self selector:@selector(animationTimerFired:) userInfo:nil repeats:YES] retain];
    }
}

- (void) stopAnimationTimer
{
    if (animationTimer != nil) {
        [animationTimer invalidate];
        [animationTimer release];
        animationTimer = nil;
    }
}

- (void) animationTimerFired:(NSTimer *)timer
{
    lastFPS = (HowLong(&backUpTime) + lastFPS) * 0.5f;
    [FPSCounter setStringValue:[NSString stringWithFormat:@"%d/%d FPS",(int)lastFPS,(int)(1.0f/FrameRate)]];
    [GLView setNeedsDisplay:YES];
}

// TAB VIEW DELEGATE
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSRect frame = [PrefWin frame];
    float height;
    if(![[tabViewItem identifier] isEqual:@"maintab"]) height = ((GoomFXView*)[tabViewItem view])->height;
    else height = 356.0f;
    height += 20.0f;
    frame.origin.y -= height-frame.size.height;
    frame.size.height = height;
    [PrefWin setFrame:frame display:YES animate:YES];
}

@end
