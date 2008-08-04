#import <Cocoa/Cocoa.h>

#import "Goom.h"

#import "MainOpenGLView.h"

@interface AppController : NSResponder
{
    // Views
    IBOutlet NSWindow * PrefWin;
    IBOutlet NSTabView * TabView;

    IBOutlet MainOpenGLView * GLView;
    
    IBOutlet NSButton * HQButton;
    IBOutlet NSTextField * QEWarning;
    IBOutlet NSTextField * FPSCounter;

    // Model
    BOOL isAnimating;
    NSTimer *animationTimer;
    CFAbsoluteTime timeBefore;
    
    BOOL stayInFullScreenMode;
    NSOpenGLContext *fullScreenContext;

    IBOutlet Goom * myGoom;
    float FrameRate;
    float lastFPS;
    AbsoluteTime backUpTime;
}

- (IBAction) goFullScreen:(id)sender;
- (IBAction) setHighQuality:(id)sender;
- (IBAction) setFrameRate:(id)sender;
- (BOOL) isInFullScreenMode;

@end
