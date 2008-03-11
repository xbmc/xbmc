/*
 *  CocoaToCppThunk.cpp
 *  XBMC
 *
 *  Created by Elan Feingold on 2/27/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#include "stdafx.h"
#include "Application.h"
#include "AppleRemoteKeys.h"
#include "CocoaToCppThunk.h"

void Cocoa_OnAppleRemoteKey(void* application, AppleRemoteEventIdentifier event, bool pressedDown, unsigned int count)
{
  CApplication* pApp = (CApplication* )application;
  printf("Remote Key: %d, down=%d, count=%d\n", event, pressedDown, count);
  
  switch (event)
  {
  case kRemoteButtonPlay:
    if (count >= 2)
    {
      CKey key(KEY_BUTTON_WHITE);
      pApp->OnKey(key);
    }
    else
    {
      CKey key(KEY_BUTTON_A);
      pApp->OnKey(key);
    }
    break;

  case kRemoteButtonVolume_Plus:
  {
    CKey key(KEY_BUTTON_DPAD_UP);
    pApp->OnKey(key);
  }
  break;

  case kRemoteButtonVolume_Minus:
  {
    CKey key(KEY_BUTTON_DPAD_DOWN);
    pApp->OnKey(key);
  }
  break;

  case kRemoteButtonRight:
  {
    CKey key(KEY_BUTTON_DPAD_RIGHT);
    pApp->OnKey(key);
  }
  break;

  case kRemoteButtonLeft:
  {
    CKey key(KEY_BUTTON_DPAD_LEFT);
    pApp->OnKey(key);
  }
  break;
  case kRemoteButtonRight_Hold:
  case kRemoteButtonLeft_Hold:
  case kRemoteButtonVolume_Plus_Hold:
  case kRemoteButtonVolume_Minus_Hold:
    /* simulate an event as long as the user holds the button */
    //b_remote_button_hold = pressedDown;
    //if( pressedDown )
    //{
    //    NSNumber* buttonIdentifierNumber = [NSNumber numberWithInt: buttonIdentifier];
    //    [self performSelector:@selector(executeHoldActionForRemoteButton:)
    //               withObject:buttonIdentifierNumber];
    //}
    break;
    
  case kRemoteButtonMenu:
  {
    if (pApp->IsPlayingFullScreenVideo() == false)
    {
      CKey key(KEY_BUTTON_B);
      pApp->OnKey(key);
    }
    else
    {
      CKey key(KEY_BUTTON_START);
      pApp->OnKey(key);
    }
  }
  break;
  
  case kRemoteButtonMenu_Hold:
  {
    if (pApp->IsPlayingFullScreenVideo() == true)
    {
      CKey key(KEY_BUTTON_B);
      pApp->OnKey(key);
    }
  }
  break;
    
  default:
    /* Add here whatever you want other buttons to do */
    break;
  }
}

