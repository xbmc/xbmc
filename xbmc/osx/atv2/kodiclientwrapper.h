/*
 *  xbmcclient.cpp
 *  xbmclauncher
 *
 *      Created by Stephan Diederich on 17.09.08.
 *      Copyright 2008 Stephan Diederich. All rights reserved.
 *      Copyright (C) 2008-2013 Team XBMC
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
typedef enum{
  ATV_BUTTON_DONT_USE_THIS = 0, //don't use zero, as those enums get converted to strings later
	ATV_BUTTON_PLAY=1,
	ATV_BUTTON_PLAY_H, //present on ATV>=2.2
	ATV_BUTTON_RIGHT,
	ATV_BUTTON_RIGHT_RELEASE,
	ATV_BUTTON_RIGHT_H, //present on ATV<=2.1
	ATV_BUTTON_LEFT,
	ATV_BUTTON_LEFT_RELEASE,
	ATV_BUTTON_LEFT_H, //present on ATV<=2.1
	ATV_BUTTON_UP,
	ATV_BUTTON_UP_RELEASE,
	ATV_BUTTON_DOWN,
	ATV_BUTTON_DOWN_RELEASE,
	ATV_BUTTON_MENU,
	ATV_BUTTON_MENU_H,
	ATV_LEARNED_PLAY,
	ATV_LEARNED_PAUSE,
	ATV_LEARNED_STOP,
	ATV_LEARNED_PREVIOUS,
	ATV_LEARNED_NEXT,
	ATV_LEARNED_REWIND, //>=ATV 2.3
	ATV_LEARNED_REWIND_RELEASE, //>=ATV 2.3
	ATV_LEARNED_FORWARD, //>=ATV 2.3
	ATV_LEARNED_FORWARD_RELEASE, //>=ATV 2.3
	ATV_LEARNED_RETURN,
	ATV_LEARNED_ENTER,
	ATV_GESTURE_SWIPE_LEFT,
	ATV_GESTURE_SWIPE_RIGHT,
	ATV_GESTURE_SWIPE_UP,
	ATV_GESTURE_SWIPE_DOWN,
	ATV_ALUMINIUM_PLAY,
	ATV_ALUMINIUM_PLAY_H,
	ATV_INVALID_BUTTON
} eATVClientEvent;

@interface KodiClientWrapper : NSObject{
	struct KodiClientWrapperImpl* mp_impl;
}
- (id) initWithUniversalMode:(bool) f_yes_no serverAddress:(NSString*) fp_server;
- (void) setUniversalModeTimeout:(double) f_timeout;

-(void) handleEvent:(eATVClientEvent) f_event;

@end