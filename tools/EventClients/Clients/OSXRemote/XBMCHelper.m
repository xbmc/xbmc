//
//  XBMCHelper.m
//  xbmchelper
//
//  Created by Stephan Diederich on 11/12/08.
//  Copyright 2008 University Heidelberg. All rights reserved.
//

#import "XBMCHelper.h"
#import "XBMCDebugHelpers.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
@interface XBMCHelper (private)

- (NSString *)buttonNameForButtonCode:(HIDRemoteButtonCode)buttonCode;
- (void) checkAndLaunchApp;

@end

@implementation XBMCHelper
- (id) init{
  if( (self = [super init]) ){
    if ((remote = [HIDRemote sharedHIDRemote]))
    {
      [remote setDelegate:self];
      [remote setSimulateHoldEvents:NO];
      //for now, we're using lending of exlusive lock
      //kHIDRemoteModeExclusiveAuto isn't working, as we're a background daemon
      //one possibility would be to know when XBMC is running. Once we know that,
      //we could aquire exclusive lock when it's running, and release _exclusive_
      //access once done
      [remote setExclusiveLockLendingEnabled:YES];

      if ([HIDRemote isCandelairInstallationRequiredForRemoteMode:kHIDRemoteModeExclusive])
      {
        //setup failed. user needs to install CandelaIR driver
        NSLog(@"Error! Candelair driver installation necessary. XBMCHelper won't function properly!");
        NSLog(@"Due to an issue in the OS version you are running, an additional driver needs to be installed before XBMC(Helper) can reliably access the remote.");
        NSLog(@"See http://www.candelair.com/download/ for details");
        [super dealloc];
        return nil;
      }
      else
      {
        if ([remote startRemoteControl:kHIDRemoteModeExclusive])
        {
          DLOG(@"Driver has started successfully.");
          if ([remote activeRemoteControlCount])
            DLOG(@"Driver has found %d remotes.", [remote activeRemoteControlCount]);
          else
            ELOG(@"Driver has not found any remotes it could use. Will use remotes as they become available.");
        }
        else
        {
          ELOG(@"Failed to start remote control.");
          //setup failed, cleanup
          [remote setDelegate:nil];
          [super dealloc];
          return nil;
        }
      }
    }
  }
  return self;
}

//----------------------------------------------------------------------------
- (void) dealloc{
  [remote stopRemoteControl];
  if( [remote delegate] == self)
    [remote setDelegate:nil];
  [mp_wrapper release];
  [mp_app_path release];
  [mp_home_path release];

  [super dealloc];
}

//----------------------------------------------------------------------------
- (void) connectToServer:(NSString*) fp_server onPort:(int) f_port withMode:(eRemoteMode) f_mode withTimeout:(double) f_timeout{
  if(mp_wrapper)
    [self disconnect];
  mp_wrapper = [[XBMCClientWrapper alloc] initWithMode:f_mode serverAddress:fp_server port:f_port verbose:m_verbose];
  [mp_wrapper setUniversalModeTimeout:f_timeout];
}

//----------------------------------------------------------------------------
- (void) disconnect{
  [mp_wrapper release];
  mp_wrapper = nil;
}

//----------------------------------------------------------------------------
- (void) enableVerboseMode:(bool) f_really{
  m_verbose = f_really;
  [mp_wrapper enableVerboseMode:f_really];
}

//----------------------------------------------------------------------------
- (void) setApplicationPath:(NSString*) fp_app_path{
  if (mp_app_path != fp_app_path) {
    [mp_app_path release];
    mp_app_path = [[fp_app_path stringByStandardizingPath] retain];
  }
}

//----------------------------------------------------------------------------
- (void) setApplicationHome:(NSString*) fp_home_path{
  if (mp_home_path != fp_home_path) {
    [mp_home_path release];
    mp_home_path = [[fp_home_path stringByStandardizingPath] retain];
  }
}

#pragma mark -
#pragma mark HIDRemote delegate methods

// Notification of button events
- (void)hidRemote:(HIDRemote *)hidRemote eventWithButton:(HIDRemoteButtonCode)buttonCode
        isPressed:(BOOL)isPressed fromHardwareWithAttributes:(NSMutableDictionary *)attributes
{
  if(m_verbose){
    NSLog(@"Received button '%@' %@ event", [self buttonNameForButtonCode:buttonCode], (isPressed)?@"press":@"release");
  }
  switch(buttonCode)
  {
    case kHIDRemoteButtonCodeUp:
      if(isPressed)
        [mp_wrapper handleEvent:ATV_BUTTON_UP];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_UP_RELEASE];
      break;
    case kHIDRemoteButtonCodeDown:
      if(isPressed)
        [mp_wrapper handleEvent:ATV_BUTTON_DOWN];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_DOWN_RELEASE];
      break;
    case kHIDRemoteButtonCodeLeft:
      if(isPressed)
        [mp_wrapper handleEvent:ATV_BUTTON_LEFT];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_LEFT_RELEASE];
      break;
    case kHIDRemoteButtonCodeRight:
      if(isPressed)
        [mp_wrapper handleEvent:ATV_BUTTON_RIGHT];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_RIGHT_RELEASE];
      break;      
    case kHIDRemoteButtonCodeCenter:
      if(isPressed) [mp_wrapper handleEvent:ATV_BUTTON_CENTER];
      break;
    case kHIDRemoteButtonCodeMenu:
      if(isPressed){
        [self checkAndLaunchApp]; //launch mp_app_path if it's not running
        [mp_wrapper handleEvent:ATV_BUTTON_MENU];
      }
      break;
    case kHIDRemoteButtonCodePlay: //aluminium remote
      if(isPressed) {
        [mp_wrapper handleEvent:ATV_BUTTON_PLAY];
      }
      break;
//    case kHIDRemoteButtonCodeUpHold:
//      //TODO
//      break;
//    case kHIDRemoteButtonCodeDownHold:
//      //TODO
      break;
    case kHIDRemoteButtonCodeLeftHold:
      if(isPressed)
        [mp_wrapper handleEvent:ATV_BUTTON_LEFT_H];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_LEFT_H_RELEASE];
      break;
    case kHIDRemoteButtonCodeRightHold:
      if(isPressed)
        [mp_wrapper handleEvent:ATV_BUTTON_RIGHT_H];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_RIGHT_H_RELEASE];
      break;
    case kHIDRemoteButtonCodeCenterHold:
      if(isPressed) [mp_wrapper handleEvent:ATV_BUTTON_CENTER_H];
      break;      
    case kHIDRemoteButtonCodeMenuHold:
      if(isPressed) {
        [self checkAndLaunchApp]; //launch mp_app_path if it's not running
        [mp_wrapper handleEvent:ATV_BUTTON_MENU_H];
      }
      break;
    case kHIDRemoteButtonCodePlayHold: //aluminium remote
      if(isPressed) {
        [mp_wrapper handleEvent:ATV_BUTTON_PLAY_H];
      }
      break;
    default:
      NSLog(@"Oha, remote button not recognized %i pressed/released %i", buttonCode, isPressed);
  }
}


// Notification of ID changes
- (void)hidRemote:(HIDRemote *)hidRemote remoteIDChangedOldID:(SInt32)old 
            newID:(SInt32)newID forHardwareWithAttributes:(NSMutableDictionary *)attributes
{
  if(m_verbose)
    NSLog(@"Change of remote ID from %d to %d", old, newID);
  [mp_wrapper switchRemote: newID];
  
}

#pragma mark -
#pragma mark Helper methods

- (NSString *)buttonNameForButtonCode:(HIDRemoteButtonCode)buttonCode
{
	switch (buttonCode)
	{
		case kHIDRemoteButtonCodePlus:
			return (@"Plus");
      break;
		case kHIDRemoteButtonCodeMinus:
			return (@"Minus");
      break;
		case kHIDRemoteButtonCodeLeft:
			return (@"Left");
      break;
		case kHIDRemoteButtonCodeRight:
			return (@"Right");
      break;
		case kHIDRemoteButtonCodePlayPause:
			return (@"Play/Pause");
      break;
		case kHIDRemoteButtonCodeMenu:
			return (@"Menu");
      break;
		case kHIDRemoteButtonCodePlusHold:
			return (@"Plus (hold)");
      break;
		case kHIDRemoteButtonCodeMinusHold:
			return (@"Minus (hold)");
      break;
		case kHIDRemoteButtonCodeLeftHold:
			return (@"Left (hold)");
      break;
		case kHIDRemoteButtonCodeRightHold:
			return (@"Right (hold)");
      break;
		case kHIDRemoteButtonCodePlayPauseHold:
			return (@"Play/Pause (hold)");
      break;
		case kHIDRemoteButtonCodeMenuHold:
			return (@"Menu (hold)");
      break;
	}
	return ([NSString stringWithFormat:@"Button %x", (int)buttonCode]);
}

//----------------------------------------------------------------------------
- (void) checkAndLaunchApp
{
  if(!mp_app_path || ![mp_app_path length]){
    ELOG(@"No executable set. Nothing to launch");
    return;
  }
  NSFileManager *fileManager = [NSFileManager defaultManager];
  if([fileManager fileExistsAtPath:mp_app_path]){
    if(mp_home_path && [mp_home_path length])
      setenv("KODI_HOME", [mp_home_path UTF8String], 1);
    //launch or activate xbmc
    if(![[NSWorkspace sharedWorkspace] launchApplication:mp_app_path])
      ELOG(@"Error launching %@", mp_app_path);
  } else
    ELOG(@"Path does not exist: %@. Cannot launch executable", mp_app_path);
}


#pragma mark -
#pragma mark Other (unused) HIDRemoteDelegate methods
//- (BOOL)hidRemote:(HIDRemote *)aHidRemote
//lendExclusiveLockToApplicationWithInfo:(NSDictionary *)applicationInfo
//{
//	NSLog(@"Lending exclusive lock to %@ (pid %@)", [applicationInfo objectForKey:(id)kCFBundleIdentifierKey], [applicationInfo objectForKey:kHIDRemoteDNStatusPIDKey]);
//	return (YES);
//}
//
//- (void)hidRemote:(HIDRemote *)aHidRemote
//exclusiveLockReleasedByApplicationWithInfo:(NSDictionary *)applicationInfo
//{
//  NSLog(@"Exclusive lock released by %@ (pid %@)", [applicationInfo objectForKey:(id)kCFBundleIdentifierKey], [applicationInfo objectForKey:kHIDRemoteDNStatusPIDKey]);
//	[aHidRemote startRemoteControl:kHIDRemoteModeExclusive];
//}
//
//- (BOOL)hidRemote:(HIDRemote *)aHidRemote
//shouldRetryExclusiveLockWithInfo:(NSDictionary *)applicationInfo
//{
//  NSLog(@"%@ (pid %@) says I should retry to acquire exclusive locks", [applicationInfo objectForKey:(id)kCFBundleIdentifierKey], [applicationInfo objectForKey:kHIDRemoteDNStatusPIDKey]);
//	return (YES);
//}
//
//
//// Notification about hardware additions/removals
//- (void)hidRemote:(HIDRemote *)aHidRemote foundNewHardwareWithAttributes:(NSMutableDictionary *)attributes
//{
//	NSLog(@"Found hardware: %@ by %@ (Transport: %@)", [attributes objectForKey:kHIDRemoteProduct], [attributes objectForKey:kHIDRemoteManufacturer], [attributes objectForKey:kHIDRemoteTransport]);
//}
//
//- (void)hidRemote:(HIDRemote *)aHidRemote failedNewHardwareWithError:(NSError *)error
//{
//	NSLog(@"Initialization of hardware failed with error %@ (%@)", [error localizedDescription], [[error userInfo] objectForKey:@"InternalErrorCode"]);
//}
//
//- (void)hidRemote:(HIDRemote *)aHidRemote releasedHardwareWithAttributes:(NSMutableDictionary *)attributes
//{
//	NSLog(@"Released hardware: %@ by %@ (Transport: %@)", [attributes objectForKey:kHIDRemoteProduct], [attributes objectForKey:kHIDRemoteManufacturer], [attributes objectForKey:kHIDRemoteTransport]);
//}

@end
