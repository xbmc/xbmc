//
//  HotKeyController.h
//
//  Modified by Gaurav Khanna on 8/17/10.
//  SOURCE: http://github.com/sweetfm/SweetFM/blob/master/Source/HMediaKeys.h
//
//
//  Permission is hereby granted, free of charge, to any person 
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify, 
//  merge, publish, distribute, sublicense, and/or sell copies of 
//  the Software, and to permit persons to whom the Software is 
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be 
//  included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
//  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
//  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

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
