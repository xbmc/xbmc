/*
 *      Copyright (C) 2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <AudioToolbox/AudioToolbox.h>

#import "XBMC_events.h"
#include "XBMC_keysym.h"

@interface XBMCController : UIViewController
{
  int m_screensaverTimeout;
	
  /* Touch handling */
  CGPoint firstTouch;
  CGPoint lastTouch;
  CGSize screensize;
	
  UIInterfaceOrientation orientation;

  XBMC_Event lastEvent;
}
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property CGPoint firstTouch;
@property CGPoint lastTouch;
@property CGSize screensize;
@property XBMC_Event lastEvent;

// message from which our instance is obtained
- (void)pauseAnimation;
- (void)resumeAnimation;
- (void)startAnimation;
- (void)stopAnimation;
- (void) sendKey: (XBMCKey) key;
- (void) observeDefaultCenterStuff: (NSNotification *) notification;
- (void) initDisplayLink;
- (void) deinitDisplayLink;
- (double) getDisplayLinkFPS;
- (void) setFramebuffer;
- (bool) presentFramebuffer;
- (CGSize) getScreenSize;
- (UIInterfaceOrientation) getOrientation;
- (void)createGestureRecognizers;
- (void) disableSystemSleep;
- (void) enableSystemSleep;
- (void) disableScreenSaver;
- (void) enableScreenSaver;
@end

extern XBMCController *g_xbmcController;
