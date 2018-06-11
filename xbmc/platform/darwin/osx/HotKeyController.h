/*
 *  HotKeyController.h
 *
 *  Modified by Gaurav Khanna on 8/17/10.
 *  SOURCE: http://github.com/sweetfm/SweetFM/blob/master/Source/HMediaKeys.h
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

#import <Cocoa/Cocoa.h>

extern NSString* const MediaKeyPower;
extern NSString* const MediaKeySoundMute;
extern NSString* const MediaKeySoundUp;
extern NSString* const MediaKeySoundDown;
extern NSString* const MediaKeyPlayPauseNotification;
extern NSString* const MediaKeyFastNotification;
extern NSString* const MediaKeyRewindNotification;
extern NSString* const MediaKeyNextNotification;
extern NSString* const MediaKeyPreviousNotification;

@interface HotKeyController : NSObject
{
  CFMachPortRef m_eventPort;
  CFRunLoopSourceRef m_runLoopSource;
  BOOL          m_active;
  BOOL          m_controlSysVolume;
  BOOL          m_controlSysPower;
}

+ (HotKeyController*)sharedController;

- (void)enableTap;
- (void)disableTap;

- (CFMachPortRef)eventPort;

- (void)sysPower:  (BOOL)enable;
- (BOOL)controlPower;

- (void)sysVolume: (BOOL)enable;
- (BOOL)controlVolume;

- (void)setActive: (BOOL)active;
- (BOOL)getActive;

@end
