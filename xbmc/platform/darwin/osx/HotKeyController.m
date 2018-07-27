/*
 *  HotKeyController.m
 *
 *  Modified by Gaurav Khanna on 8/17/10.
 *  SOURCE: http://github.com/sweetfm/SweetFM/blob/master/Source/HMediaKeys.m
 *  SOURCE: http://stackoverflow.com/questions/2969110/cgeventtapcreate-breaks-down-mysteriously-with-key-down-events
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

#import "HotKeyController.h"
#import <IOKit/hidsystem/ev_keymap.h>
#import <sys/sysctl.h>

NSString* const MediaKeyPower                 = @"MediaKeyPower";
NSString* const MediaKeySoundMute             = @"MediaKeySoundMute";
NSString* const MediaKeySoundUp               = @"MediaKeySoundUp";
NSString* const MediaKeySoundDown             = @"MediaKeySoundDown";
NSString* const MediaKeyPlayPauseNotification = @"MediaKeyPlayPauseNotification";
NSString* const MediaKeyFastNotification      = @"MediaKeyFastNotification";
NSString* const MediaKeyRewindNotification    = @"MediaKeyRewindNotification";
NSString* const MediaKeyNextNotification      = @"MediaKeyNextNotification";
NSString* const MediaKeyPreviousNotification  = @"MediaKeyPreviousNotification";

#ifndef kCGEventTapOptionDefault
#define kCGEventTapOptionDefault 0
#endif

#define NX_KEYSTATE_DOWN    0x0A
#define NX_KEYSTATE_UP      0x0B

@implementation HotKeyController

+ (HotKeyController*)sharedController
{
  static HotKeyController *sharedHotKeyController = nil;
  if (sharedHotKeyController == nil)
    sharedHotKeyController = [[super allocWithZone:NULL] init];

  return sharedHotKeyController;
}

+ (id)allocWithZone:(NSZone *)zone
{
  return [[self sharedController] retain];
}

- (id)copyWithZone:(NSZone *)zone
{
  return self;
}

- (id)retain
{
  return self;
}

- (NSUInteger)retainCount
{
  //denotes an object that cannot be released
  return NSUIntegerMax;
}

- (oneway void)release
{
  //do nothing
}

- (id)autorelease
{
  return self;
}

- (CFMachPortRef)eventPort
{
  return m_eventPort;
}

- (void)sysPower: (BOOL)enable
{
  m_controlSysPower = enable;
}
- (BOOL)controlPower
{
  return m_controlSysPower;
}

- (void)sysVolume: (BOOL)enable
{
  m_controlSysVolume = enable;
}
- (BOOL)controlVolume
{
  return m_controlSysVolume;
}

- (void)setActive: (BOOL)active
{
  m_active = active;
}

- (BOOL)getActive
{
  return m_active;
}

- (BOOL)getDebuggerActive
{
  // Technical Q&A QA1361
  // returns true if the current process is being debugged (either
  // running under the debugger or has a debugger attached post facto).
  int                 junk;
  int                 mib[4];
  struct kinfo_proc   info;
  size_t              size;

  // initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.
  info.kp_proc.p_flag = 0;

  // initialize mib, which tells sysctl the info we want, in this case
  // we are looking for information about a specific process ID.
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();

  // Call sysctl.
  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  assert(junk == 0);

  // we are being debugged if the P_TRACED flag is set.
  return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}

// WARNING: do not debugger breakpoint in this routine.
// It's a system level call back that taps ALL Events
// and you WILL lose all key control :)
static CGEventRef tapEventCallback2(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  HotKeyController *hot_key_controller = (HotKeyController*)refcon;

  if (type == kCGEventTapDisabledByTimeout)
  {
    if ([hot_key_controller getActive])
      CGEventTapEnable([hot_key_controller eventPort], TRUE);
    return NULL;
  }

  if ((type != NX_SYSDEFINED) || (![hot_key_controller getActive]))
    return event;

  NSEvent *nsEvent = [NSEvent eventWithCGEvent:event];
  if (!nsEvent || [nsEvent subtype] != 8)
    return event;

  int data = [nsEvent data1];
  int keyCode  = (data & 0xFFFF0000) >> 16;
  int keyFlags = (data & 0xFFFF);
  int keyState = (keyFlags & 0xFF00) >> 8;
  BOOL keyIsRepeat = (keyFlags & 0x1) > 0;

  // allow repeated keypresses for volume buttons
  // all other repeated keypresses are handled by the os (is this really good?)
  if (keyIsRepeat && keyCode != NX_KEYTYPE_SOUND_UP && keyCode != NX_KEYTYPE_SOUND_DOWN)
    return event;

  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  switch (keyCode)
  {
    case NX_POWER_KEY:
      if ([hot_key_controller controlPower])
      {
        if (keyState == NX_KEYSTATE_DOWN)
          [center postNotificationName:MediaKeyPower object:(HotKeyController *)refcon];
        if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
          return NULL;
      }
    break;
    case NX_KEYTYPE_MUTE:
      if ([hot_key_controller controlVolume])
      {
        if (keyState == NX_KEYSTATE_DOWN)
          [center postNotificationName:MediaKeySoundMute object:(HotKeyController *)refcon];
        if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
          return NULL;
      }
    break;
    case NX_KEYTYPE_SOUND_UP:
      if ([hot_key_controller controlVolume])
      {
        if (keyState == NX_KEYSTATE_DOWN)
          [center postNotificationName:MediaKeySoundUp object:(HotKeyController *)refcon];
        if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
          return NULL;
      }
    break;
    case NX_KEYTYPE_SOUND_DOWN:
      if ([hot_key_controller controlVolume])
      {
        if (keyState == NX_KEYSTATE_DOWN)
          [center postNotificationName:MediaKeySoundDown object:(HotKeyController *)refcon];
        if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
          return NULL;
      }
    break;
    case NX_KEYTYPE_PLAY:
      if (keyState == NX_KEYSTATE_DOWN)
        [center postNotificationName:MediaKeyPlayPauseNotification object:(HotKeyController *)refcon];
      if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
        return NULL;
    break;
    case NX_KEYTYPE_FAST:
      if (keyState == NX_KEYSTATE_DOWN)
        [center postNotificationName:MediaKeyFastNotification object:(HotKeyController *)refcon];
      if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
        return NULL;
    break;
    case NX_KEYTYPE_REWIND:
      if (keyState == NX_KEYSTATE_DOWN)
        [center postNotificationName:MediaKeyRewindNotification object:(HotKeyController *)refcon];
      if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
        return NULL;
    break;
    case NX_KEYTYPE_NEXT:
      if (keyState == NX_KEYSTATE_DOWN)
        [center postNotificationName:MediaKeyNextNotification object:(HotKeyController *)refcon];
      if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
        return NULL;
    break;
    case NX_KEYTYPE_PREVIOUS:
      if (keyState == NX_KEYSTATE_DOWN)
        [center postNotificationName:MediaKeyPreviousNotification object:(HotKeyController *)refcon];
      if (keyState == NX_KEYSTATE_UP || keyState == NX_KEYSTATE_DOWN)
        return NULL;
    break;
  }
  return event;
}

static CGEventRef tapEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  NSAutoreleasePool *pool = [NSAutoreleasePool new];
  CGEventRef ret = tapEventCallback2(proxy, type, event, refcon);
  [pool drain];
  return ret;
}


-(void)eventTapThread
{
  m_runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorSystemDefault, m_eventPort, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), m_runLoopSource, kCFRunLoopCommonModes);
  // Enable the event tap.
  CGEventTapEnable(m_eventPort, TRUE);

  CFRunLoopRun();

  [self setActive:NO];
  // Disable the event tap.
  if (m_eventPort)
    CGEventTapEnable(m_eventPort, FALSE);

  if (m_runLoopSource)
    CFRelease(m_runLoopSource);
  m_runLoopSource = NULL;
  if (m_eventPort)
    CFRelease(m_eventPort);
  m_eventPort = NULL;
}

- (id)init
{
  if (self = [super init])
  {
    m_active = NO;
    m_eventPort = NULL;
    m_runLoopSource = NULL;
    // power button controls xbmc sleep button (this will also trigger the osx shutdown menu - we can't prevent this as it seems)
    m_controlSysPower = YES;
    m_controlSysVolume = NO; // volume keys control sys volume
  }
  return self;
}

- (void)enableTap
{
  if (![self getDebuggerActive] && ![self getActive])
  {
    m_eventPort = CGEventTapCreate(kCGSessionEventTap,
      kCGHeadInsertEventTap, kCGEventTapOptionDefault,
      CGEventMaskBit(NX_SYSDEFINED), tapEventCallback, self);
    if (m_eventPort != NULL)
    {
      // Run this in a separate thread so that a slow app
      // doesn't lag the event tap
      [NSThread detachNewThreadSelector:@selector(eventTapThread) toTarget:self withObject:nil];
      [self setActive:YES];
    }
  }
}

- (void)disableTap
{
  if ([self getActive])
  {
    [self setActive:NO];

    // Disable the event tap.
    if (m_eventPort)
      CGEventTapEnable(m_eventPort, FALSE);

    if (m_runLoopSource)
      CFRelease(m_runLoopSource);
    m_runLoopSource = NULL;
    if (m_eventPort)
      CFRelease(m_eventPort);
    m_eventPort = NULL;
  }
}
@end
