/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <memory>
#include <string>

#import <Foundation/Foundation.h>
#import <OpenGLES/EAGL.h>
#import <UIKit/UIKit.h>

@class AVDisplayManager;
@class DarwinEmbedNowPlayingInfoManager;
@class TVOSEAGLView;
@class TVOSLibInputHandler;
@class TVOSDisplayManager;

class CFileItem;

@interface XBMCController : UIViewController
{
  BOOL m_isPlayingBeforeInactive;
  UIBackgroundTaskIdentifier m_bgTask;
  BOOL m_bgTaskActive;
  bool m_nativeKeyboardActive;
  BOOL m_pause;
  BOOL m_animating;
  NSConditionLock* m_animationThreadLock;
  NSThread* m_animationThread;
  std::unique_ptr<CFileItem> m_playingFileItemBeforeBackground;
  std::string m_lastUsedPlayer;
}

@property(nonatomic) BOOL appAlive;
@property(nonatomic, strong) DarwinEmbedNowPlayingInfoManager* MPNPInfoManager;
@property(nonatomic, strong) TVOSDisplayManager* displayManager;
@property(nonatomic, strong) TVOSEAGLView* glView;
@property(nonatomic, strong) TVOSLibInputHandler* inputHandler;

- (void)pauseAnimation;
- (void)resumeAnimation;
- (void)startAnimation;
- (void)stopAnimation;

- (void)enterBackground;
- (void)enterForeground;
- (void)setFramebuffer;
- (bool)presentFramebuffer;
- (void)activateKeyboard:(UIView*)view;
- (void)deactivateKeyboard:(UIView*)view;
- (void)nativeKeyboardActive:(bool)active;

- (UIBackgroundTaskIdentifier)enableBackGroundTask;
- (void)disableBackGroundTask:(UIBackgroundTaskIdentifier)bgTaskID;

- (void)disableScreenSaver;
- (void)enableScreenSaver;
- (bool)resetSystemIdleTimer;

- (CGRect)fullscreenSubviewFrame;

- (AVDisplayManager*)avDisplayManager __attribute__((availability(tvos, introduced = 11.2)));

- (EAGLContext*)getEAGLContextObj;

@end

extern XBMCController* g_xbmcController;
