/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#import <Foundation/Foundation.h>
#import <BackRow/BackRow.h>
#import "IOSEAGLView.h"
#import "IOSSCreenManager.h"
#include "XBMC_keysym.h"

@interface KodiController : BRController
{
  int padding[16];  // credit is due here to SapphireCompatibilityClasses!!
        
  BRController *m_controller;

  NSTimer      *m_keyTimer;
  IOSEAGLView  *m_glView;

  int           m_screensaverTimeout;
  int           m_systemsleepTimeout;

}
// message from which our instance is obtained
//+ (KodiController*) sharedInstance;

- (void) applicationDidExit;
- (void) initDisplayLink;
- (void) deinitDisplayLink;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (CGSize) getScreenSize;
- (void) sendKey: (XBMCKey) key;
- (void) disableSystemSleep;
- (void) enableSystemSleep;
- (void) disableScreenSaver;
- (void) enableScreenSaver;
- (void) pauseAnimation;
- (void) resumeAnimation;
- (void) startAnimation;
- (void) stopAnimation;
- (bool) changeScreen: (unsigned int)screenIdx withMode:(UIScreenMode *)mode;
- (void) activateScreen: (UIScreen *)screen withOrientation:(UIInterfaceOrientation)newOrientation;
- (id) glView;
- (void) setGlView:(id)view;
- (BOOL) ATVClientEventFromBREvent:(id)event Repeatable:(bool *)isRepeatable ButtonState:(bool *)isPressed Result:(int *)xbmc_ir_key;
- (void) setUserEvent:(int) eventId withHoldTime:(unsigned int) holdTime;
- (unsigned int) appleModKeyToXbmcModKey: (unsigned int) appleModifier;
- (void) startKeyPressTimer:(int) keyId;
- (void) stopKeyPressTimer;
- (void) setSystemSleepTimeout:(id) timeout;
- (id) systemSleepTimeout;
- (void) setKeyTimer:(id) timer;
- (id) keyTimer;
- (void) setSystemScreenSaverTimeout:(id) timeout;
- (id) systemScreenSaverTimeout;

@end

extern KodiController *g_xbmcController;
