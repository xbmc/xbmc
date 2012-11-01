/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
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

@interface XBMCController : BRController
{
  int padding[16];  // credit is due here to SapphireCompatibilityClasses!!
        
  BRController *m_controller;

  NSTimer      *m_keyTimer;
  IOSEAGLView  *m_glView;

  int           m_screensaverTimeout;
  int           m_systemsleepTimeout;

}
// message from which our instance is obtained
//+ (XBMCController*) sharedInstance;

- (void) applicationDidExit;
- (void) initDisplayLink;
- (void) deinitDisplayLink;
- (double) getDisplayLinkFPS;
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
- (void) activateScreen: (UIScreen *)screen;


@end

extern XBMCController *g_xbmcController;
