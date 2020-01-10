/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "TVOSDisplayManager.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#import "windowing/tvos/WinSystemTVOS.h"

#import "platform/darwin/tvos/TVOSEAGLView.h"
#import "platform/darwin/tvos/XBMCController.h"

#import <AVFoundation/AVDisplayCriteria.h>
#import <AVKit/AVDisplayManager.h>
#import <QuartzCore/CADisplayLink.h>

#define DISPLAY_MODE_SWITCH_IN_PROGRESS NSStringFromSelector(@selector(displayModeSwitchInProgress))

@interface AVDisplayCriteria ()
@property(readonly) int videoDynamicRange;
@property(readonly, nonatomic) float refreshRate;
- (id)initWithRefreshRate:(float)arg1 videoDynamicRange:(int)arg2;
@end

@implementation TVOSDisplayManager

@synthesize screenScale;

#pragma mark - display switching routines
- (float)getDisplayRate
{
  if (m_displayRate > 0.0f)
    return m_displayRate;

  return 60.0f;
}

- (void)displayLinkTick:(CADisplayLink*)sender
{
  auto duration = m_displayLink.duration;
  if (duration > 0.0)
  {
    // we want fps, not duration in seconds.
    m_displayRate = 1.0 / duration;
  }
}

- (void)displayRateSwitch:(float)refreshRate withDynamicRange:(int)dynamicRange
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) == ADJUST_REFRESHRATE_OFF)
    return;
  if (@available(tvOS 11.2, *))
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      auto avDisplayManager = [g_xbmcController avDisplayManager];
      if (refreshRate > 0.0f)
      {
        // videoDynamicRange values are based on watching
        // console log when forcing different values.
        // search for "Native Mode Requested" and pray :)
        // searches for "FBSDisplayConfiguration" and "currentMode" will show the actual
        // for example, currentMode = <FBSDisplayMode: 0x1c4298100; 1920x1080@2x (3840x2160/2) 24Hz p3 HDR10>
        // SDR == 0, 1
        // HDR == 2, 3
        // DoblyVision == 4
        auto displayCriteria = [[AVDisplayCriteria alloc] initWithRefreshRate:refreshRate
                                                            videoDynamicRange:dynamicRange];
        // setting preferredDisplayCriteria will trigger a display rate switch
        [self setDisplayCriteria:avDisplayManager displayCriteria:displayCriteria];
      }
      else
      {
        // switch back to tvOS defined user settings if we get
        // zero or less than value for refreshRate. Should never happen :)
        [self setDisplayCriteria:avDisplayManager displayCriteria:nil];
      }
    });
    CLog::Log(LOGDEBUG, "displayRateSwitch request: refreshRate = {}, dynamicRange = {}",
              refreshRate, [self stringFromDynamicRange:dynamicRange]);
  }
}

- (void)displayRateReset
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) == ADJUST_REFRESHRATE_OFF)
    return;
  if (@available(tvOS 11.2, *))
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      // setting preferredDisplayCriteria to nil will
      // switch back to tvOS defined user settings
      auto avDisplayManager = [g_xbmcController avDisplayManager];
      [self setDisplayCriteria:avDisplayManager displayCriteria:nil];
    });
  }
}

- (void)setDisplayCriteria:(AVDisplayManager*)avDisplayManager
           displayCriteria:(AVDisplayCriteria*)dispCriteria
    __attribute__((availability(tvos, introduced = 11.2)))
{
  if (@available(tvOS 11.3, *))
  {
    if (avDisplayManager.displayCriteriaMatchingEnabled)
      avDisplayManager.preferredDisplayCriteria = dispCriteria;
  }
  else
  {
    avDisplayManager.preferredDisplayCriteria = dispCriteria;
  }
}

- (void)removeModeSwitchObserver
{
  if (@available(tvOS 11.2, *))
  {
    auto avDisplayManager = [g_xbmcController avDisplayManager];
    [avDisplayManager removeObserver:self forKeyPath:DISPLAY_MODE_SWITCH_IN_PROGRESS];
  }
}

- (void)addModeSwitchObserver
{
  if (@available(tvOS 11.2, *))
  {
    auto avDisplayManager = [g_xbmcController avDisplayManager];
    [avDisplayManager addObserver:self
                       forKeyPath:DISPLAY_MODE_SWITCH_IN_PROGRESS
                          options:NSKeyValueObservingOptionNew
                          context:nullptr];
  }
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context
{
  if (![keyPath isEqualToString:DISPLAY_MODE_SWITCH_IN_PROGRESS])
    return;

  // tracking displayModeSwitchInProgress via NSKeyValueObservingOptionNew,
  // any changes in displayModeSwitchInProgress will fire this callback.
  if (@available(tvOS 11.2, *))
  {
    std::string switchState = "NO";
    int dynamicRange = 0;
    float refreshRate;
    auto avDisplayManager = [g_xbmcController avDisplayManager];
    auto displayCriteria = avDisplayManager.preferredDisplayCriteria;
    // preferredDisplayCriteria can be nil, this is NOT an error
    // and just indicates tvOS defined user settings which we cannot see.
    if (displayCriteria != nil)
    {
      refreshRate = displayCriteria.refreshRate;
      dynamicRange = displayCriteria.videoDynamicRange;
    }
    if (avDisplayManager.displayModeSwitchInProgress)
    {
      switchState = "YES";
      m_winSystem->AnnounceOnLostDevice();
      m_winSystem->StartLostDeviceTimer();
    }
    else
    {
      switchState = "DONE";
      m_winSystem->StartLostDeviceTimer();
      m_winSystem->AnnounceOnLostDevice();
      // displayLinkTick is tracking actual refresh duration.
      // when isDisplayModeSwitchInProgress == NO, we have switched
      // and stablized. We might have switched to some other
      // rate than what we requested. setting preferredDisplayCriteria is
      // only a request. For example, 30Hz might only be avaliable in HDR
      // and asking for 30Hz/SDR might result in 60Hz/SDR and
      // g_graphicsContext.SetFPS needs the actual refresh rate.
      refreshRate = [self getDisplayRate];
    }
    //! @todo
    //g_graphicsContext.SetFPS(refreshRate);
    CLog::Log(LOGDEBUG, "displayModeSwitchInProgress = {}, refreshRate = {}, dynamicRange = {}",
              switchState, refreshRate, [self stringFromDynamicRange:dynamicRange]);
  }
}

- (const char*)stringFromDynamicRange:(int)dynamicRange
{
  switch (dynamicRange)
  {
    case 0 ... 1:
      return "SDR";
    case 2 ... 3:
      return "HDR10";
    case 4:
      return "DolbyVision";
    default:
      return "Unknown";
  }
}

- (CGSize)getScreenSize
{
  dispatch_sync(dispatch_get_main_queue(), ^{
    m_screensize.width = g_xbmcController.glView.bounds.size.width * self.screenScale;
    m_screensize.height = g_xbmcController.glView.bounds.size.height * self.screenScale;
  });
  return m_screensize;
}

#pragma mark - init
- (instancetype)init
{
  self = [super init];
  if (!self)
    return nil;

  m_winSystem = dynamic_cast<CWinSystemTVOS*>(CServiceBroker::GetWinSystem());

  m_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkTick:)];
  // we want the native cadence of the display hardware.
  m_displayLink.preferredFramesPerSecond = 0;
  [m_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];

  return self;
}

@end
