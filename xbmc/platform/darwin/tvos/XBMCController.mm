/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/tvos/XBMCController.h"

#include "AppParamParser.h"
#include "Application.h"
#include "CompileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "platform/xbmc.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#import "windowing/tvos/WinEventsTVOS.h"
#import "windowing/tvos/WinSystemTVOS.h"

#import "platform/darwin/ios-common/AnnounceReceiver.h"
#import "platform/darwin/ios-common/DarwinEmbedNowPlayingInfoManager.h"
#import "platform/darwin/tvos/TVOSDisplayManager.h"
#import "platform/darwin/tvos/TVOSEAGLView.h"
#import "platform/darwin/tvos/TVOSTopShelf.h"
#import "platform/darwin/tvos/XBMCApplication.h"
#import "platform/darwin/tvos/input/LibInputHandler.h"
#import "platform/darwin/tvos/input/LibInputRemote.h"
#import "platform/darwin/tvos/input/LibInputTouch.h"

#import <AVKit/AVDisplayManager.h>
#import <AVKit/UIWindow.h>

#import "system.h"

using namespace KODI::MESSAGING;

XBMCController* g_xbmcController;

#pragma mark - XBMCController implementation
@implementation XBMCController

@synthesize appAlive = m_appAlive;
@synthesize MPNPInfoManager;
@synthesize displayManager;
@synthesize inputHandler;
@synthesize glView;

#pragma mark - UIView Keyboard

- (void)activateKeyboard:(UIView*)view
{
  [self.view addSubview:view];
  glView.userInteractionEnabled = NO;
}

- (void)deactivateKeyboard:(UIView*)view
{
  [view removeFromSuperview];
  glView.userInteractionEnabled = YES;
  [self becomeFirstResponder];
}

- (void)nativeKeyboardActive:(bool)active;
{
  // Not used on tvOS
}

#pragma mark - View

- (void)viewDidLoad
{
  [super viewDidLoad];

  glView = [[TVOSEAGLView alloc] initWithFrame:self.view.bounds withScreen:[UIScreen mainScreen]];

  // Check if screen is Retina
  displayManager.screenScale = [glView getScreenScale:[UIScreen mainScreen]];

  self.view.backgroundColor = UIColor.blackColor;
  [self.view addSubview:glView];

  [inputHandler.inputTouch createSwipeGestureRecognizers];
  [inputHandler.inputTouch createPanGestureRecognizers];
  [inputHandler.inputTouch createPressGesturecognizers];
  [inputHandler.inputTouch createTapGesturecognizers];

  [displayManager addModeSwitchObserver];
}

- (void)viewWillAppear:(BOOL)animated
{
  [self resumeAnimation];
  [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self becomeFirstResponder];
  [[UIApplication sharedApplication]
      beginReceivingRemoteControlEvents]; // @todo MPRemoteCommandCenter
}

- (void)viewWillDisappear:(BOOL)animated
{
  [self pauseAnimation];
  [super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
  [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
  [self resignFirstResponder];
  [super viewDidUnload];
}

- (UIView*)inputView
{
  // override our input view to an empty view
  // this prevents the on screen keyboard
  // which would be shown whenever this UIResponder
  // becomes the first responder (which is always the case!)
  // caused by implementing the UIKeyInput protocol
  return [[UIView alloc] initWithFrame:CGRectZero];
}

#pragma mark - FirstResponder

- (BOOL)canBecomeFirstResponder
{
  return YES;
}

#pragma mark - FrameBuffer

- (void)setFramebuffer
{
  if (!m_pause)
    [glView setFramebuffer];
}

- (bool)presentFramebuffer
{
  if (!m_pause)
    return [glView presentFramebuffer];
  else
    return FALSE;
}

- (CGRect)fullscreenSubviewFrame
{
    return UIScreen.mainScreen.bounds;
}

- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];
  // Release any cached data, images, etc. that aren't in use.
}

#pragma mark - BackgroundTask

- (UIBackgroundTaskIdentifier)enableBackGroundTask;
{
  CLog::Log(LOGDEBUG, "%s: enableBackgroundTask created", __PRETTY_FUNCTION__);
  // we have to alloc the background task for keep network working after screen lock and dark.
  return [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:nil];
}

- (void)disableBackGroundTask:(UIBackgroundTaskIdentifier)bgTaskID;
{
  if (bgTaskID != UIBackgroundTaskInvalid)
  {
    CLog::Log(LOGDEBUG, "%s: endBackgroundTask closed", __PRETTY_FUNCTION__);
    [[UIApplication sharedApplication] endBackgroundTask:bgTaskID];
    bgTaskID = UIBackgroundTaskInvalid;
  }
}

#pragma mark - AppFocus

- (void)becomeInactive
{
  // if we were interrupted, already paused here
  // else if user background us or lock screen, only pause video here, audio keep playing.
  if (g_application.GetAppPlayer().IsPlayingVideo() && !g_application.GetAppPlayer().IsPaused())
  {
    m_isPlayingBeforeInactive = YES;
    m_lastUsedPlayer = g_application.GetAppPlayer().GetCurrentPlayer();
    m_playingFileItemBeforeBackground =
        std::make_unique<CFileItem>(g_application.CurrentFileItem());
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE_IF_PLAYING);
    g_application.CurrentFileItem().m_lStartOffset = g_application.GetAppPlayer().GetTime() - 2.50;
  }
}

- (void)enterBackground
{
  m_bgTask = [self enableBackGroundTask];
  m_bgTaskActive = YES;

  CLog::Log(LOGNOTICE, "%s: Running sleep jobs", __FUNCTION__);

  CWinSystemTVOS* winSystem = dynamic_cast<CWinSystemTVOS*>(CServiceBroker::GetWinSystem());
  winSystem->OnAppFocusChange(false);

  // Media was paused, Full background shutdown, so stop now.
  // Only do for PVR? leave regular media paused?
  if (g_application.GetAppPlayer().IsPaused())
  {
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW ||
        CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
        CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME ||
        CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

    g_application.StopPlaying();
  }

  CServiceBroker::GetPVRManager().OnSleep();
  CServiceBroker::GetActiveAE()->Suspend();
  CServiceBroker::GetNetwork().GetServices().Stop(true);

  //  if (!m_isPlayingBeforeInactive)
  g_application.CloseNetworkShares();

  m_bgTaskActive = NO;
  [self disableBackGroundTask:m_bgTask];
}

- (void)enterForeground
{
  // stop background task (if running)
  if (m_bgTaskActive)
  {
    CLog::Log(LOGDEBUG, "%s: bgTask already running, closing", __PRETTY_FUNCTION__);
    [self disableBackGroundTask:m_bgTask];
  }

  [NSThread detachNewThreadSelector:@selector(enterForegroundDelayed:)
                           toTarget:self
                         withObject:nil];
}

- (void)enterForegroundDelayed:(id)arg
{

  __block BOOL appstate = YES;
  dispatch_async(dispatch_get_main_queue(), ^{
    if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive)
      appstate = NO;
  });

  if (!appstate)
    return;

  // g_application.IsInitialized is only true if
  // we were running and got moved to background
  while (!g_application.IsInitialized())
    usleep(50 * 1000);

  CServiceBroker::GetNetwork().WaitForNet();
  CServiceBroker::GetNetwork().GetServices().Start();

  if (CServiceBroker::GetActiveAE())
    if (CServiceBroker::GetActiveAE()->IsSuspended())
      CServiceBroker::GetActiveAE()->Resume();

  CServiceBroker::GetPVRManager().OnWake();

  CWinSystemTVOS* winSystem = dynamic_cast<CWinSystemTVOS*>(CServiceBroker::GetWinSystem());
  winSystem->OnAppFocusChange(true);

  // when we come back, restore playing if we were.
  if (m_isPlayingBeforeInactive)
  {
    if (m_playingFileItemBeforeBackground->IsLiveTV())
    {
      CLog::Log(LOGDEBUG, "%s: Live TV was playing before suspend. Restart channel",
                __PRETTY_FUNCTION__);
      // Restart player with lastused FileItem
      g_application.PlayFile(*m_playingFileItemBeforeBackground, m_lastUsedPlayer, true);
    }
    else
    {
      if (g_application.GetAppPlayer().IsPaused() && g_application.GetAppPlayer().HasPlayer())
      {
        CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_UNPAUSE);
      }
      else
      {
        g_application.PlayFile(*m_playingFileItemBeforeBackground, m_lastUsedPlayer, true);
      }
    }
    m_playingFileItemBeforeBackground = std::make_unique<CFileItem>();
    m_lastUsedPlayer = "";
    m_isPlayingBeforeInactive = NO;
  }

  // do not update if we are already updating
  if (!(g_application.IsVideoScanning() || g_application.IsMusicScanning()))
    g_application.UpdateLibraries();

  // this will fire only if we are already alive and have 'menu'ed out and back
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnWake");

  // this handles what to do if we got pushed
  // into foreground by a topshelf item select/play
  CTVOSTopShelf::GetInstance().RunTopShelf();
}

#pragma mark - ScreenSaver Idletimer

- (void)disableScreenSaver
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
  });
}

- (void)enableScreenSaver
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
  });
}

- (bool)resetSystemIdleTimer
{
  // this is silly :)
  // when system screen saver kicks off, we switch to UIApplicationStateInactive, the only way
  // to get out of the screensaver is to call ourself to open an custom URL that is registered
  // in our Info.plist. The openURL method of UIApplication must be supported but we can just
  // reply NO and we get restored to UIApplicationStateActive.
  __block bool inActive = false;
  dispatch_async(dispatch_get_main_queue(), ^{
    inActive = [UIApplication sharedApplication].applicationState == UIApplicationStateInactive;
    if (inActive)
    {
      auto wakeupString =
          [[NSArray arrayWithObjects:[NSString stringWithUTF8String:CCompileInfo::GetAppName()],
                                     @"://wakeup", nil] componentsJoinedByString:@""];
      NSURL* url = [NSURL URLWithString:wakeupString];
      [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
    }
  });
  return inActive;
}

#pragma mark - runtime routines

- (void)pauseAnimation
{
  m_pause = YES;
  g_application.SetRenderGUI(false);
}

- (void)resumeAnimation
{
  m_pause = NO;
  g_application.SetRenderGUI(true);
}

- (void)startAnimation
{
  if (!m_animating && [glView getCurrentEAGLContext])
  {
    // kick off an animation thread
    m_animationThreadLock = [[NSConditionLock alloc] initWithCondition:FALSE];
    m_animationThread = [[NSThread alloc] initWithTarget:self
                                                selector:@selector(runAnimation:)
                                                  object:m_animationThreadLock];
    [m_animationThread start];
    m_animating = YES;
  }
}

- (void)stopAnimation
{
  if (!m_animating && [glView getCurrentEAGLContext])
  {
    m_appAlive = NO;
    m_animating = NO;
    if (!g_application.m_bStop)
    {
      CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
    }

    CAnnounceReceiver::GetInstance()->DeInitialize();

    // wait for animation thread to die
    if (!m_animationThread.finished)
      [m_animationThreadLock lockWhenCondition:TRUE];
  }
}

- (void)runAnimation:(id)arg
{
  @autoreleasepool
  {
    [NSThread currentThread].name = @"XBMC_Run";

    // signal the thread is alive
    NSConditionLock* myLock = arg;
    [myLock lock];

    // Prevent child processes from becoming zombies on exit
    // if not waited upon. See also Util::Command
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDWAIT;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);

    setlocale(LC_NUMERIC, "C");

    int status = 0;
    try
    {
      // set up some Kodi specific relationships
      //    XBMC::Context run_context; //! @todo
      m_appAlive = YES;
      // start up with gui enabled
      status = KODI_Run(true);
      // we exited or died.
      g_application.SetRenderGUI(false);
    }
    catch (...)
    {
      m_appAlive = FALSE;
      CLog::Log(LOGERROR, "%sException caught on main loop status=%d. Exiting", __PRETTY_FUNCTION__,
                status);
    }

    // signal the thread is dead
    [myLock unlockWithCondition:TRUE];

    [self enableScreenSaver];
    [self performSelectorOnMainThread:@selector(CallExit) withObject:nil waitUntilDone:NO];
  }
}

#pragma mark - KODI_Run

int KODI_Run(bool renderGUI)
{
  int status = -1;

  CAppParamParser appParamParser; //! @todo : proper params
  if (!g_application.Create(appParamParser))
  {
    CLog::Log(LOGERROR, "ERROR: Unable to create application. Exiting");
    return status;
  }

  //this can't be set from CAdvancedSettings::Initialize()
  //because it will overwrite the loglevel set with the --debug flag
#ifdef _DEBUG
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel = LOG_LEVEL_DEBUG;
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevelHint = LOG_LEVEL_DEBUG;
#else
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel = LOG_LEVEL_NORMAL;
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevelHint = LOG_LEVEL_NORMAL;
#endif
  CLog::SetLogLevel(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel);

  // not a failure if returns false, just means someone
  // did the init before us.
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->Initialized())
  {
    //CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->Initialize();
    //! @todo
  }

  CAnnounceReceiver::GetInstance()->Initialize();

  if (renderGUI && !g_application.CreateGUI())
  {
    CLog::Log(LOGERROR, "ERROR: Unable to create GUI. Exiting");
    return status;
  }
  if (!g_application.Initialize())
  {
    CLog::Log(LOGERROR, "ERROR: Unable to Initialize. Exiting");
    return status;
  }

  try
  {
    status = g_application.Run(appParamParser);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "ERROR: Exception caught on main loop. Exiting");
    status = -1;
  }

  return status;
}

- (void)CallExit
{
  exit(0);
}

- (AVDisplayManager*)avDisplayManager __attribute__((availability(tvos, introduced = 11.2)))
{
  return self.view.window.avDisplayManager;
}

#pragma mark - EAGLContext

- (EAGLContext*)getEAGLContextObj
{
  return [glView getCurrentEAGLContext];
}

#pragma mark - remoteControlReceivedWithEvent forwarder
//  remoteControlReceived requires subclassing of UIViewController
//  Just implement as a forwarding class to CLibRemote so it doesnt need to subclass
- (void)remoteControlReceivedWithEvent:(UIEvent*)receivedEvent
{
  if (receivedEvent.type == UIEventTypeRemoteControl)
  {
    [inputHandler.inputRemote remoteControlEvent:receivedEvent];
  }
}

#pragma mark - init/deinit

- (void)dealloc
{
  [displayManager removeModeSwitchObserver];
  // stop background task (if running)
  [self disableBackGroundTask:m_bgTask];

  [self stopAnimation];
}

- (instancetype)init
{
  self = [super init];
  if (!self)
    return nil;

  m_pause = NO;
  m_appAlive = NO;
  m_animating = NO;

  m_isPlayingBeforeInactive = NO;
  m_bgTaskActive = NO;
  m_bgTask = UIBackgroundTaskInvalid;

  [self enableScreenSaver];

  g_xbmcController = self;
  MPNPInfoManager = [DarwinEmbedNowPlayingInfoManager new];
  displayManager = [TVOSDisplayManager new];
  inputHandler = [TVOSLibInputHandler new];

  return self;
}

@end
#undef BOOL
