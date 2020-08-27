/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBMCApp.h"

#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include <jni.h>
#include <android/configuration.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <androidjni/ActivityManager.h>
#include <androidjni/ApplicationInfo.h>
#include <androidjni/BitmapFactory.h>
#include <androidjni/BroadcastReceiver.h>
#include <androidjni/Build.h>
#include <androidjni/CharSequence.h>
#include <androidjni/ConnectivityManager.h>
#include <androidjni/ContentResolver.h>
#include <androidjni/Context.h>
#include <androidjni/Cursor.h>
#include <androidjni/Display.h>
#include <androidjni/DisplayManager.h>
#include <androidjni/Environment.h>
#include <androidjni/File.h>
#include <androidjni/Intent.h>
#include <androidjni/IntentFilter.h>
#include <androidjni/JNIThreading.h>
#include <androidjni/KeyEvent.h>
#include <androidjni/MediaStore.h>
#include <androidjni/NetworkInfo.h>
#include <androidjni/PackageManager.h>
#include <androidjni/PowerManager.h>
#include <androidjni/StatFs.h>
#include <androidjni/System.h>
#include <androidjni/SystemClock.h>
#include <androidjni/SystemProperties.h>
#include <androidjni/URI.h>
#include <androidjni/View.h>
#include <androidjni/WakeLock.h>
#include <androidjni/Window.h>
#include <androidjni/WindowManager.h>

#include "AndroidKey.h"
#include "settings/AdvancedSettings.h"
#include "interfaces/AnnouncementManager.h"
#include "Application.h"
#include "AppParamParser.h"
#include "messaging/ApplicationMessenger.h"
#include "CompileInfo.h"
#include "settings/DisplaySettings.h"
#include "windowing/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
// Audio Engine includes for Factory and interfaces
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Sinks/AESinkAUDIOTRACK.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/VideoDatabaseFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/Key.h"
#include "input/mouse/MouseStat.h"
#include "platform/xbmc.h"
#include "powermanagement/PowerManager.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/WinEvents.h"
#include "windowing/android/VideoSyncAndroid.h"
#include "windowing/android/WinSystemAndroid.h"

#include "platform/android/activity/IInputDeviceCallbacks.h"
#include "platform/android/activity/IInputDeviceEventHandler.h"
#include "platform/android/network/NetworkAndroid.h"
#include "platform/android/powermanagement/AndroidPowerSyscall.h"

#define GIGABYTES       1073741824

#define ACTION_XBMC_RESUME "android.intent.XBMC_RESUME"

#define PLAYBACK_STATE_STOPPED  0x0000
#define PLAYBACK_STATE_PLAYING  0x0001
#define PLAYBACK_STATE_VIDEO    0x0100
#define PLAYBACK_STATE_AUDIO    0x0200
#define PLAYBACK_STATE_CANNOT_PAUSE 0x0400

using namespace KODI::MESSAGING;
using namespace ANNOUNCEMENT;
using namespace jni;

template<class T, void(T::*fn)()>
void* thread_run(void* obj)
{
  (static_cast<T*>(obj)->*fn)();
  return NULL;
}

CXBMCApp* CXBMCApp::m_xbmcappinstance = NULL;
std::unique_ptr<CJNIXBMCMainView> CXBMCApp::m_mainView;
ANativeActivity *CXBMCApp::m_activity = NULL;
CJNIWakeLock *CXBMCApp::m_wakeLock = NULL;
ANativeWindow* CXBMCApp::m_window = NULL;
int CXBMCApp::m_batteryLevel = 0;
bool CXBMCApp::m_hasFocus = false;
bool CXBMCApp::m_headsetPlugged = false;
bool CXBMCApp::m_hdmiPlugged = true;
bool CXBMCApp::m_hdmiSource = false;
IInputDeviceCallbacks* CXBMCApp::m_inputDeviceCallbacks = nullptr;
IInputDeviceEventHandler* CXBMCApp::m_inputDeviceEventHandler = nullptr;
bool CXBMCApp::m_hasReqVisible = false;
CCriticalSection CXBMCApp::m_applicationsMutex;
CCriticalSection CXBMCApp::m_activityResultMutex;
std::vector<androidPackage> CXBMCApp::m_applications;
CVideoSyncAndroid* CXBMCApp::m_syncImpl = NULL;
CEvent CXBMCApp::m_vsyncEvent;
CEvent CXBMCApp::m_displayChangeEvent;
std::vector<CActivityResultEvent*> CXBMCApp::m_activityResultEvents;

int64_t CXBMCApp::m_frameTimeNanos = 0;
float CXBMCApp::m_refreshRate = 0.0f;

uint32_t CXBMCApp::m_playback_state = PLAYBACK_STATE_STOPPED;

CXBMCApp::CXBMCApp(ANativeActivity* nativeActivity, IInputHandler& inputHandler)
  : CJNIMainActivity(nativeActivity),
    CJNIBroadcastReceiver(CJNIContext::getPackageName() + ".XBMCBroadcastReceiver"),
    m_inputHandler(inputHandler),
    m_videosurfaceInUse(false)
{
  m_xbmcappinstance = this;
  m_activity = nativeActivity;
  if (m_activity == NULL)
  {
    android_printf("CXBMCApp: invalid ANativeActivity instance");
    exit(1);
    return;
  }
  m_mainView.reset(new CJNIXBMCMainView(this));
  m_firstrun = true;
  m_exiting = false;
  m_hdmiSource = CJNISystemProperties::get("ro.hdmi.device_type", "") == "4";
  android_printf("CXBMCApp: Created");
}

CXBMCApp::~CXBMCApp()
{
  m_xbmcappinstance = NULL;
  delete m_wakeLock;
}

void CXBMCApp::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (strcmp(sender, "xbmc") != 0)
    return;

  if (flag & Input)
  {
    if (strcmp(message, "OnInputRequested") == 0)
      CAndroidKey::SetHandleSearchKeys(true);
    else if (strcmp(message, "OnInputFinished") == 0)
      CAndroidKey::SetHandleSearchKeys(false);
  }
  else if (flag & Player)
  {
    if (strcmp(message, "OnPlay") == 0 || strcmp(message, "OnResume") == 0)
      OnPlayBackStarted();
    else if (strcmp(message, "OnPause") == 0)
      OnPlayBackPaused();
    else if (strcmp(message, "OnStop") == 0)
      OnPlayBackStopped();
    else if (strcmp(message, "OnSeek") == 0)
      UpdateSessionState();
    else if (strcmp(message, "OnSpeedChanged") == 0)
      UpdateSessionState();
  }
  else if (flag & Info)
  {
     if (strcmp(message, "OnChanged") == 0)
      UpdateSessionMetadata();
  }
}

void CXBMCApp::onStart()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  if (m_firstrun)
  {
    // Register sink
    AE::CAESinkFactory::ClearSinks();
    CAESinkAUDIOTRACK::Register();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&m_thread, &attr, thread_run<CXBMCApp, &CXBMCApp::run>, this);
    pthread_attr_destroy(&attr);

    // Some intent filters MUST be registered in code rather than through the manifest
    CJNIIntentFilter intentFilter;
    intentFilter.addAction("android.intent.action.BATTERY_CHANGED");
    intentFilter.addAction("android.intent.action.SCREEN_ON");
    intentFilter.addAction("android.intent.action.HEADSET_PLUG");
    // We currently use HDMI_AUDIO_PLUG for mode switch, don't use it on TV's (device_type = "0"
    if (m_hdmiSource)
      intentFilter.addAction("android.media.action.HDMI_AUDIO_PLUG");

    intentFilter.addAction("android.intent.action.SCREEN_OFF");
    intentFilter.addAction("android.net.conn.CONNECTIVITY_CHANGE");
    registerReceiver(*this, intentFilter);
    m_mediaSession.reset(new CJNIXBMCMediaSession());
  }
  if (g_application.IsInitialized())
  {
    IPowerSyscall* syscall = CServiceBroker::GetPowerManager().GetPowerSyscall();
    if (syscall)
      static_cast<CAndroidPowerSyscall*>(syscall)->SetOnResume();
  }
}

void CXBMCApp::onResume()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  if (g_application.IsInitialized() && CServiceBroker::GetWinSystem()->GetOSScreenSaver()->IsInhibited())
    EnableWakeLock(true);

  CJNIAudioManager audioManager(getSystemService("audio"));
  m_headsetPlugged = audioManager.isWiredHeadsetOn() || audioManager.isBluetoothA2dpOn();

  // Clear the applications cache. We could have installed/deinstalled apps
  {
    CSingleLock lock(m_applicationsMutex);
    m_applications.clear();
  }

  if (m_bResumePlayback && g_application.GetAppPlayer().IsPlaying())
  {
    if (g_application.GetAppPlayer().HasVideo())
    {
      if (g_application.GetAppPlayer().IsPaused())
        CApplicationMessenger::GetInstance().SendMsg(
            TMSG_GUI_ACTION, WINDOW_INVALID, -1,
            static_cast<void*>(new CAction(ACTION_PLAYER_PLAY)));
    }
  }

  // Re-request Visible Behind
  if ((m_playback_state & PLAYBACK_STATE_PLAYING) && (m_playback_state & PLAYBACK_STATE_VIDEO))
    RequestVisibleBehind(true);
}

void CXBMCApp::onPause()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_bResumePlayback = false;

  if (g_application.GetAppPlayer().IsPlaying())
  {
    if (g_application.GetAppPlayer().HasVideo())
    {
      if (!g_application.GetAppPlayer().IsPaused() && !m_hasReqVisible)
      {
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
        m_bResumePlayback = true;
      }
    }
  }

  if (m_hasReqVisible)
    g_application.SwitchToFullScreen(true);

  EnableWakeLock(false);
  m_hasReqVisible = false;
}

void CXBMCApp::onStop()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  if ((m_playback_state & PLAYBACK_STATE_PLAYING) && !m_hasReqVisible)
  {
    if (m_playback_state & PLAYBACK_STATE_CANNOT_PAUSE)
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
    else if (m_playback_state & PLAYBACK_STATE_VIDEO)
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
  }

  if (!g_application.IsStopping())
  {
    IPowerSyscall* syscall = CServiceBroker::GetPowerManager().GetPowerSyscall();
    if (syscall)
      static_cast<CAndroidPowerSyscall*>(syscall)->SetOnPause();
  }
}

void CXBMCApp::onDestroy()
{
  android_printf("%s", __PRETTY_FUNCTION__);

  unregisterReceiver(*this);

  m_mediaSession.release();

  // If android is forcing us to stop, ask XBMC to exit then wait until it's
  // been destroyed.
  if (!m_exiting)
  {
    XBMC_Stop();
    pthread_join(m_thread, NULL);
    android_printf(" => XBMC finished");
  }
}

void CXBMCApp::onSaveState(void **data, size_t *size)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // no need to save anything as XBMC is running in its own thread
}

void CXBMCApp::onConfigurationChanged()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // ignore any configuration changes like screen rotation etc
}

void CXBMCApp::onLowMemory()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // can't do much as we don't want to close completely
}

void CXBMCApp::onCreateWindow(ANativeWindow* window)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onResizeWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_window = NULL;
  // no need to do anything because we are fixed in fullscreen landscape mode
}

void CXBMCApp::onDestroyWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onGainFocus()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_hasFocus = true;
  g_application.WakeUpScreenSaverAndDPMS();
}

void CXBMCApp::onLostFocus()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_hasFocus = false;
}

void CXBMCApp::RegisterDisplayListener(CVariant* variant)
{
  CJNIDisplayManager displayManager(getSystemService("display"));
  if (displayManager)
  {
    android_printf("CXBMCApp: installing DisplayManager::DisplayListener");
    displayManager.registerDisplayListener(CXBMCApp::get()->getDisplayListener());
  }
}

void CXBMCApp::Initialize()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(CXBMCApp::get());
  runNativeOnUiThread(RegisterDisplayListener, nullptr);
  m_activityManager.reset(new CJNIActivityManager(getSystemService(CJNIContext::ACTIVITY_SERVICE)));
  m_inputHandler.setDPI(GetDPI());
}

void CXBMCApp::Deinitialize()
{
}

bool CXBMCApp::EnableWakeLock(bool on)
{
  android_printf("%s: %s", __PRETTY_FUNCTION__, on ? "true" : "false");
  if (!m_wakeLock)
  {
    std::string appName = CCompileInfo::GetAppName();
    StringUtils::ToLower(appName);
    std::string className = CCompileInfo::GetPackage();
    // SCREEN_BRIGHT_WAKE_LOCK is marked as deprecated but there is no real alternatives for now
    m_wakeLock = new CJNIWakeLock(CJNIPowerManager(getSystemService("power")).newWakeLock(CJNIPowerManager::SCREEN_BRIGHT_WAKE_LOCK | CJNIPowerManager::ON_AFTER_RELEASE, className.c_str()));
    if (m_wakeLock)
      m_wakeLock->setReferenceCounted(false);
    else
      return false;
  }

  if (on)
  {
    if (!m_wakeLock->isHeld())
      m_wakeLock->acquire();
  }
  else
  {
    if (m_wakeLock->isHeld())
      m_wakeLock->release();
  }

  return true;
}

bool CXBMCApp::AcquireAudioFocus()
{
  if (!m_xbmcappinstance)
    return false;

  CJNIAudioManager audioManager(getSystemService("audio"));

  // Request audio focus for playback
  int result = audioManager.requestAudioFocus(m_audioFocusListener,
                                              // Use the music stream.
                                              CJNIAudioManager::STREAM_MUSIC,
                                              // Request permanent focus.
                                              CJNIAudioManager::AUDIOFOCUS_GAIN);

  if (result != CJNIAudioManager::AUDIOFOCUS_REQUEST_GRANTED)
  {
    CXBMCApp::android_printf("Audio Focus request failed");
    return false;
  }
  return true;
}

bool CXBMCApp::ReleaseAudioFocus()
{
  if (!m_xbmcappinstance)
    return false;

  CJNIAudioManager audioManager(getSystemService("audio"));

  // Release audio focus after playback
  int result = audioManager.abandonAudioFocus(m_audioFocusListener);
  if (result != CJNIAudioManager::AUDIOFOCUS_REQUEST_GRANTED)
  {
    CXBMCApp::android_printf("Audio Focus abandon failed");
    return false;
  }
  return true;
}

void CXBMCApp::RequestVisibleBehind(bool requested)
{
  if (requested == m_hasReqVisible)
    return;

  m_hasReqVisible = requestVisibleBehind(requested);
  CLog::Log(LOGDEBUG, "Visible Behind request: %s", m_hasReqVisible ? "true" : "false");
}

bool CXBMCApp::IsHeadsetPlugged()
{
  return m_headsetPlugged;
}

bool CXBMCApp::IsHDMIPlugged()
{
  return m_hdmiPlugged;
}

void CXBMCApp::run()
{
  int status = 0;

  SetupEnv();

  // Wait for main window
  ANativeWindow* nativeWindow = CXBMCApp::GetNativeWindow(30000);
  if (!nativeWindow)
    return;

  m_firstrun=false;
  android_printf(" => running XBMC_Run...");

  CAppParamParser appParamParser;
  status = XBMC_Run(true, appParamParser);
  android_printf(" => XBMC_Run finished with %d", status);

  // If we are have not been force by Android to exit, notify its finish routine.
  // This will cause android to run through its teardown events, it calls:
  // onPause(), onLostFocus(), onDestroyWindow(), onStop(), onDestroy().
  ANativeActivity_finish(m_activity);
  m_exiting=true;
}

void CXBMCApp::XBMC_Pause(bool pause)
{
  android_printf("XBMC_Pause(%s)", pause ? "true" : "false");
}

void CXBMCApp::XBMC_Stop()
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
}

bool CXBMCApp::XBMC_SetupDisplay()
{
  android_printf("XBMC_SetupDisplay()");
  bool result;
  CApplicationMessenger::GetInstance().SendMsg(TMSG_DISPLAY_SETUP, -1, -1, static_cast<void*>(&result));
  return result;
}

bool CXBMCApp::XBMC_DestroyDisplay()
{
  android_printf("XBMC_DestroyDisplay()");
  bool result;
  CApplicationMessenger::GetInstance().SendMsg(TMSG_DISPLAY_DESTROY, -1, -1, static_cast<void*>(&result));
  return result;
}

int CXBMCApp::SetBuffersGeometry(int width, int height, int format)
{
  return ANativeWindow_setBuffersGeometry(m_window, width, height, format);
}

#include "threads/Event.h"
#include <time.h>

void CXBMCApp::SetRefreshRateCallback(CVariant* rateVariant)
{
  float rate = rateVariant->asFloat();
  delete rateVariant;

  CJNIWindow window = getWindow();
  if (window)
  {
    CJNIWindowManagerLayoutParams params = window.getAttributes();
    if (fabs(params.getpreferredRefreshRate() - rate) > 0.001)
    {
      params.setpreferredRefreshRate(rate);
      if (params.getpreferredRefreshRate() > 0.0)
      {
        window.setAttributes(params);
        return;
      }
    }
  }
  m_displayChangeEvent.Set();
}

void CXBMCApp::SetDisplayModeCallback(CVariant* variant)
{
  int mode = (*variant)["mode"].asInteger();
  float rate = (*variant)["rate"].asFloat();
  delete variant;

  CJNIWindow window = getWindow();
  if (window)
  {
    CJNIWindowManagerLayoutParams params = window.getAttributes();
    if (params.getpreferredDisplayModeId() != mode)
    {
      params.setpreferredDisplayModeId(mode);
      params.setpreferredRefreshRate(rate);
      window.setAttributes(params);
      return;
    }
  }
  m_displayChangeEvent.Set();
}

void CXBMCApp::SetRefreshRate(float rate)
{
  if (rate < 1.0)
    return;

  CJNIWindow window = getWindow();
  if (window)
  {
    CJNIWindowManagerLayoutParams params = window.getAttributes();
    if (fabs(params.getpreferredRefreshRate() - rate) <= 0.001)
      return;
  }

  m_refreshRate = rate;

  m_displayChangeEvent.Reset();
  CVariant *variant = new CVariant(rate);
  runNativeOnUiThread(SetRefreshRateCallback, variant);
  if (g_application.IsInitialized())
  {
    m_displayChangeEvent.WaitMSec(5000);
    if (m_hdmiSource && g_application.GetAppPlayer().IsPlaying())
      dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem())->InitiateModeChange();
  }
}

void CXBMCApp::SetDisplayMode(int mode, float rate)
{
  if (mode < 1.0)
    return;

  CJNIWindow window = getWindow();
  if (window)
  {
    CJNIWindowManagerLayoutParams params = window.getAttributes();
    if (params.getpreferredDisplayModeId() == mode)
      return;
  }

  m_displayChangeEvent.Reset();
  std::map<std::string, CVariant> vmap;
  vmap["mode"] = mode;
  vmap["rate"] = rate;
  m_refreshRate = rate;
  CVariant *variant = new CVariant(vmap);
  runNativeOnUiThread(SetDisplayModeCallback, variant);
  if (g_application.IsInitialized())
  {
    m_displayChangeEvent.WaitMSec(5000);
    if (m_hdmiSource && g_application.GetAppPlayer().IsPlaying())
      dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem())->InitiateModeChange();
  }
}

int CXBMCApp::android_printf(const char *format, ...)
{
  // For use before CLog is setup by XBMC_Run()
  va_list args;
  va_start(args, format);
  int result = __android_log_vprint(ANDROID_LOG_VERBOSE, "Kodi", format, args);
  va_end(args);
  return result;
}

int CXBMCApp::GetDPI()
{
  if (m_activity == NULL || m_activity->assetManager == NULL)
    return 0;

  // grab DPI from the current configuration - this is approximate
  // but should be close enough for what we need
  AConfiguration *config = AConfiguration_new();
  AConfiguration_fromAssetManager(config, m_activity->assetManager);
  int dpi = AConfiguration_getDensity(config);
  AConfiguration_delete(config);

  return dpi;
}

CRect CXBMCApp::MapRenderToDroid(const CRect& srcRect)
{
  float scaleX = 1.0;
  float scaleY = 1.0;

  CJNIRect r = m_xbmcappinstance->getDisplayRect();
  if (r.width() && r.height())
  {
    RESOLUTION_INFO renderRes = CDisplaySettings::GetInstance().GetResolutionInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
    scaleX = (double)r.width() / renderRes.iWidth;
    scaleY = (double)r.height() / renderRes.iHeight;
  }

  return CRect(srcRect.x1 * scaleX, srcRect.y1 * scaleY, srcRect.x2 * scaleX, srcRect.y2 * scaleY);
}

void CXBMCApp::UpdateSessionMetadata()
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  CJNIMediaMetadataBuilder builder;
  builder
      .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_TITLE, infoMgr.GetLabel(PLAYER_TITLE))
      .putString(CJNIMediaMetadata::METADATA_KEY_TITLE, infoMgr.GetLabel(PLAYER_TITLE))
      .putLong(CJNIMediaMetadata::METADATA_KEY_DURATION, g_application.GetAppPlayer().GetTotalTime())
//      .putString(CJNIMediaMetadata::METADATA_KEY_ART_URI, thumb)
//      .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_ICON_URI, thumb)
//      .putString(CJNIMediaMetadata::METADATA_KEY_ALBUM_ART_URI, thumb)
      ;

  std::string thumb;
  if (m_playback_state & PLAYBACK_STATE_VIDEO)
  {
    builder
        .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_SUBTITLE, infoMgr.GetLabel(VIDEOPLAYER_TAGLINE))
        .putString(CJNIMediaMetadata::METADATA_KEY_ARTIST, infoMgr.GetLabel(VIDEOPLAYER_DIRECTOR))
        ;
    thumb = infoMgr.GetImage(VIDEOPLAYER_COVER, -1);
  }
  else if (m_playback_state & PLAYBACK_STATE_AUDIO)
  {
    builder
        .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_SUBTITLE, infoMgr.GetLabel(MUSICPLAYER_ARTIST))
        .putString(CJNIMediaMetadata::METADATA_KEY_ARTIST, infoMgr.GetLabel(MUSICPLAYER_ARTIST))
        ;
    thumb = infoMgr.GetImage(MUSICPLAYER_COVER, -1);
  }
  bool needrecaching = false;
  std::string cachefile = CTextureCache::GetInstance().CheckCachedImage(thumb, needrecaching);
  if (!cachefile.empty())
  {
    std::string actualfile = CSpecialProtocol::TranslatePath(cachefile);
    CJNIBitmap bmp = CJNIBitmapFactory::decodeFile(actualfile);
    if (bmp)
      builder.putBitmap(CJNIMediaMetadata::METADATA_KEY_ART, bmp);
  }
  m_mediaSession->updateMetadata(builder.build());
}

void CXBMCApp::UpdateSessionState()
{
  CJNIPlaybackStateBuilder builder;
  int state = CJNIPlaybackState::STATE_NONE;
  int64_t pos = 0;
  float speed = 0.0;
  if (m_playback_state != PLAYBACK_STATE_STOPPED)
  {
    if (g_application.GetAppPlayer().HasVideo())
      m_playback_state |= PLAYBACK_STATE_VIDEO;
    else
      m_playback_state &= ~PLAYBACK_STATE_VIDEO;
    if (g_application.GetAppPlayer().HasAudio())
      m_playback_state |= PLAYBACK_STATE_AUDIO;
    else
      m_playback_state &= ~PLAYBACK_STATE_AUDIO;
    pos = g_application.GetAppPlayer().GetTime();
    speed = g_application.GetAppPlayer().GetPlaySpeed();
    if (m_playback_state & PLAYBACK_STATE_PLAYING)
      state = CJNIPlaybackState::STATE_PLAYING;
    else
      state = CJNIPlaybackState::STATE_PAUSED;
  }
  else
    state = CJNIPlaybackState::STATE_STOPPED;
  builder
      .setState(state, pos, speed, CJNISystemClock::elapsedRealtime())
      .setActions(0xffffffffffffffff)
      ;
  m_mediaSession->updatePlaybackState(builder.build());
}

void CXBMCApp::OnPlayBackStarted()
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_playback_state = PLAYBACK_STATE_PLAYING;
  if (g_application.GetAppPlayer().HasVideo())
    m_playback_state |= PLAYBACK_STATE_VIDEO;
  if (g_application.GetAppPlayer().HasAudio())
    m_playback_state |= PLAYBACK_STATE_AUDIO;
  if (!g_application.GetAppPlayer().CanPause())
    m_playback_state |= PLAYBACK_STATE_CANNOT_PAUSE;

  m_mediaSession->activate(true);
  UpdateSessionState();

  CJNIIntent intent(ACTION_XBMC_RESUME, CJNIURI::EMPTY, *this, get_class(CJNIContext::get_raw()));
  m_mediaSession->updateIntent(intent);

  m_xbmcappinstance->AcquireAudioFocus();
  CAndroidKey::SetHandleMediaKeys(false);

  RequestVisibleBehind(true);
}

void CXBMCApp::OnPlayBackPaused()
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_playback_state &= ~PLAYBACK_STATE_PLAYING;
  UpdateSessionState();

  RequestVisibleBehind(false);
  m_xbmcappinstance->ReleaseAudioFocus();
}

void CXBMCApp::OnPlayBackStopped()
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_playback_state = PLAYBACK_STATE_STOPPED;
  UpdateSessionState();
  m_mediaSession->activate(false);

  RequestVisibleBehind(false);
  CAndroidKey::SetHandleMediaKeys(true);
  m_xbmcappinstance->ReleaseAudioFocus();
}

const CJNIViewInputDevice CXBMCApp::GetInputDevice(int deviceId)
{
  CJNIInputManager inputManager(getSystemService("input"));
  return inputManager.getInputDevice(deviceId);
}

std::vector<int> CXBMCApp::GetInputDeviceIds()
{
  CJNIInputManager inputManager(getSystemService("input"));
  return inputManager.getInputDeviceIds();
}

void CXBMCApp::ProcessSlow()
{
  if ((m_playback_state & PLAYBACK_STATE_PLAYING) && m_mediaSession->isActive())
    UpdateSessionState();
}

std::vector<androidPackage> CXBMCApp::GetApplications()
{
  CSingleLock lock(m_applicationsMutex);
  if (m_applications.empty())
  {
    CJNIList<CJNIApplicationInfo> packageList = GetPackageManager().getInstalledApplications(CJNIPackageManager::GET_ACTIVITIES);
    int numPackages = packageList.size();
    for (int i = 0; i < numPackages; i++)
    {
      CJNIIntent intent = GetPackageManager().getLaunchIntentForPackage(packageList.get(i).packageName);
      if (!intent && CJNIBuild::SDK_INT >= 21)
        intent = GetPackageManager().getLeanbackLaunchIntentForPackage(packageList.get(i).packageName);
      if (!intent)
        continue;

      androidPackage newPackage;
      newPackage.packageName = packageList.get(i).packageName;
      newPackage.packageLabel = GetPackageManager().getApplicationLabel(packageList.get(i)).toString();
      newPackage.icon = packageList.get(i).icon;
      m_applications.push_back(newPackage);
    }
  }

  return m_applications;
}

bool CXBMCApp::HasLaunchIntent(const std::string &package)
{
  return GetPackageManager().getLaunchIntentForPackage(package) != NULL;
}

// Note intent, dataType, dataURI all default to ""
bool CXBMCApp::StartActivity(const std::string &package, const std::string &intent, const std::string &dataType, const std::string &dataURI)
{
  CJNIIntent newIntent = intent.empty() ?
    GetPackageManager().getLaunchIntentForPackage(package) :
    CJNIIntent(intent);

  if (!newIntent && CJNIBuild::SDK_INT >= 21)
    newIntent = GetPackageManager().getLeanbackLaunchIntentForPackage(package);
  if (!newIntent)
    return false;

  if (!dataURI.empty())
  {
    CJNIURI jniURI = CJNIURI::parse(dataURI);

    if (!jniURI)
      return false;

    newIntent.setDataAndType(jniURI, dataType);
  }

  newIntent.setPackage(package);
  startActivity(newIntent);
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CXBMCApp::StartActivity - ExceptionOccurred launching %s", package.c_str());
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  return true;
}

int CXBMCApp::GetBatteryLevel()
{
  return m_batteryLevel;
}

bool CXBMCApp::GetExternalStorage(std::string &path, const std::string &type /* = "" */)
{
  std::string sType;
  std::string mountedState;
  bool mounted = false;

  if(type == "files" || type.empty())
  {
    CJNIFile external = CJNIEnvironment::getExternalStorageDirectory();
    if (external)
      path = external.getAbsolutePath();
  }
  else
  {
    if (type == "music")
      sType = "Music"; // Environment.DIRECTORY_MUSIC
    else if (type == "videos")
      sType = "Movies"; // Environment.DIRECTORY_MOVIES
    else if (type == "pictures")
      sType = "Pictures"; // Environment.DIRECTORY_PICTURES
    else if (type == "photos")
      sType = "DCIM"; // Environment.DIRECTORY_DCIM
    else if (type == "downloads")
      sType = "Download"; // Environment.DIRECTORY_DOWNLOADS
    if (!sType.empty())
    {
      CJNIFile external = CJNIEnvironment::getExternalStoragePublicDirectory(sType);
      if (external)
        path = external.getAbsolutePath();
    }
  }
  mountedState = CJNIEnvironment::getExternalStorageState();
  mounted = (mountedState == "mounted" || mountedState == "mounted_ro");
  return mounted && !path.empty();
}

bool CXBMCApp::GetStorageUsage(const std::string &path, std::string &usage)
{
#define PATH_MAXLEN 50

  if (path.empty())
  {
    std::ostringstream fmt;
    fmt.width(PATH_MAXLEN);  fmt << std::left  << "Filesystem";
    fmt.width(12);  fmt << std::right << "Size";
    fmt.width(12);  fmt << "Used";
    fmt.width(12);  fmt << "Avail";
    fmt.width(12);  fmt << "Use %";

    usage = fmt.str();
    return false;
  }

  CJNIStatFs fileStat(path);
  int blockSize = fileStat.getBlockSize();
  int blockCount = fileStat.getBlockCount();
  int freeBlocks = fileStat.getFreeBlocks();

  if (blockSize <= 0 || blockCount <= 0 || freeBlocks < 0)
    return false;

  float totalSize = (float)blockSize * blockCount / GIGABYTES;
  float freeSize = (float)blockSize * freeBlocks / GIGABYTES;
  float usedSize = totalSize - freeSize;
  float usedPercentage = usedSize / totalSize * 100;

  std::ostringstream fmt;
  fmt << std::fixed;
  fmt.precision(1);
  fmt.width(PATH_MAXLEN);  fmt << std::left  << (path.size() < PATH_MAXLEN-1 ? path : StringUtils::Left(path, PATH_MAXLEN-4) + "...");
  fmt.width(12);  fmt << std::right << totalSize << "G"; // size in GB
  fmt.width(12);  fmt << usedSize << "G"; // used in GB
  fmt.width(12);  fmt << freeSize << "G"; // free
  fmt.precision(0);
  fmt.width(12);  fmt << usedPercentage << "%"; // percentage used

  usage = fmt.str();
  return true;
}

// Used in Application.cpp to figure out volume steps
int CXBMCApp::GetMaxSystemVolume()
{
  JNIEnv* env = xbmc_jnienv();
  static int maxVolume = -1;
  if (maxVolume == -1)
  {
    maxVolume = GetMaxSystemVolume(env);
  }
  //android_printf("CXBMCApp::GetMaxSystemVolume: %i",maxVolume);
  return maxVolume;
}

int CXBMCApp::GetMaxSystemVolume(JNIEnv *env)
{
  CJNIAudioManager audioManager(getSystemService("audio"));
  if (audioManager)
    return audioManager.getStreamMaxVolume();
  android_printf("CXBMCApp::SetSystemVolume: Could not get Audio Manager");
  return 0;
}

float CXBMCApp::GetSystemVolume()
{
  CJNIAudioManager audioManager(getSystemService("audio"));
  if (audioManager)
    return (float)audioManager.getStreamVolume() / GetMaxSystemVolume();
  else
  {
    android_printf("CXBMCApp::GetSystemVolume: Could not get Audio Manager");
    return 0;
  }
}

void CXBMCApp::SetSystemVolume(float percent)
{
  CJNIAudioManager audioManager(getSystemService("audio"));
  int maxVolume = (int)(GetMaxSystemVolume() * percent);
  if (audioManager)
    audioManager.setStreamVolume(maxVolume);
  else
    android_printf("CXBMCApp::SetSystemVolume: Could not get Audio Manager");
}

void CXBMCApp::onReceive(CJNIIntent intent)
{
  if (!g_application.IsInitialized())
    return;

  std::string action = intent.getAction();
  CLog::Log(LOGDEBUG, "CXBMCApp::onReceive - Got intent. Action: %s", action.c_str());
  if (action == "android.intent.action.BATTERY_CHANGED")
    m_batteryLevel = intent.getIntExtra("level",-1);
  else if (action == "android.intent.action.DREAMING_STOPPED" || action == "android.intent.action.SCREEN_ON")
  {
    if (HasFocus())
      g_application.WakeUpScreenSaverAndDPMS();
  }
  else if (action == "android.intent.action.HEADSET_PLUG" ||
    action == "android.bluetooth.a2dp.profile.action.CONNECTION_STATE_CHANGED")
  {
    bool newstate = m_headsetPlugged;
    if (action == "android.intent.action.HEADSET_PLUG")
    {
      newstate = (intent.getIntExtra("state", 0) != 0);

      // If unplugged headset and playing content then pause or stop playback
      if (!newstate && (m_playback_state & PLAYBACK_STATE_PLAYING))
      {
        if (g_application.GetAppPlayer().CanPause())
        {
          CApplicationMessenger::GetInstance().PostMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
        }
        else
        {
          CApplicationMessenger::GetInstance().PostMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
        }
      }
    }
    else if (action == "android.bluetooth.a2dp.profile.action.CONNECTION_STATE_CHANGED")
      newstate = (intent.getIntExtra("android.bluetooth.profile.extra.STATE", 0) == 2 /* STATE_CONNECTED */);

    if (newstate != m_headsetPlugged)
    {
      m_headsetPlugged = newstate;
      IAE *iae = CServiceBroker::GetActiveAE();
      if (iae)
        iae->DeviceChange();
    }
  }
  else if (action == "android.media.action.HDMI_AUDIO_PLUG")
  {
    m_hdmiPlugged = (intent.getIntExtra("android.media.extra.AUDIO_PLUG_STATE", 0) != 0);
    CLog::Log(LOGDEBUG, "-- HDMI state: %s",  m_hdmiPlugged ? "on" : "off");
    if (m_hdmiSource && g_application.IsInitialized())
    {
      CWinSystemBase* winSystem = CServiceBroker::GetWinSystem();
      if (winSystem && dynamic_cast<CWinSystemAndroid*>(winSystem))
        dynamic_cast<CWinSystemAndroid*>(winSystem)->SetHdmiState(m_hdmiPlugged);
    }
  }
  else if (action == "android.intent.action.SCREEN_OFF")
  {
    if (m_playback_state & PLAYBACK_STATE_VIDEO)
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
  }
  else if (action == "android.intent.action.MEDIA_BUTTON")
  {
    if (m_playback_state == PLAYBACK_STATE_STOPPED)
    {
      CLog::Log(LOGINFO, "Ignore MEDIA_BUTTON intent: no media playing");
      return;
    }
    CJNIKeyEvent keyevt = (CJNIKeyEvent)intent.getParcelableExtra(CJNIIntent::EXTRA_KEY_EVENT);

    int keycode = keyevt.getKeyCode();
    bool up = (keyevt.getAction() == CJNIKeyEvent::ACTION_UP);

    CLog::Log(LOGINFO, "Got MEDIA_BUTTON intent: %d, up:%s", keycode, up ? "true" : "false");
    if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_RECORD)
      CAndroidKey::XBMC_Key(keycode, XBMCK_RECORD, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_EJECT)
      CAndroidKey::XBMC_Key(keycode, XBMCK_EJECT, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_FAST_FORWARD)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_FASTFORWARD, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_NEXT)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_NEXT_TRACK, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PAUSE)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PLAY_PAUSE, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PLAY)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PLAY_PAUSE, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PLAY_PAUSE)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PLAY_PAUSE, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PREVIOUS)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PREV_TRACK, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_REWIND)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_REWIND, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_STOP)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_STOP, 0, 0, up);
  }
  else if (action == "android.net.conn.CONNECTIVITY_CHANGE")
  {
    if (g_application.IsInitialized())
    {
      CNetworkBase& net = CServiceBroker::GetNetwork();
      CNetworkAndroid* netdroid = static_cast<CNetworkAndroid*>(&net);
      netdroid->RetrieveInterfaces();
    }
  }
}

void CXBMCApp::onNewIntent(CJNIIntent intent)
{
  if (!intent)
  {
    CLog::Log(LOGINFO, "CXBMCApp::onNewIntent - Got invalid intent.");
    return;
  }

  std::string action = intent.getAction();
  CLog::Log(LOGDEBUG, "CXBMCApp::onNewIntent - Got intent. Action: %s", action.c_str());
  std::string targetFile = GetFilenameFromIntent(intent);
  if (!targetFile.empty() &&  (action == "android.intent.action.VIEW" || action == "android.intent.action.GET_CONTENT"))
  {
    CLog::Log(LOGDEBUG, "-- targetFile: %s", targetFile.c_str());

    CURL targeturl(targetFile);
    std::string value;
    if (action == "android.intent.action.GET_CONTENT" || (targeturl.GetOption("showinfo", value) && value == "true"))
    {
      if (targeturl.IsProtocol("videodb")
          || (targeturl.IsProtocol("special") && targetFile.find("playlists/video") != std::string::npos)
          || (targeturl.IsProtocol("special") && targetFile.find("playlists/mixed") != std::string::npos)
          )
      {
        std::vector<std::string> params;
        params.push_back(targeturl.Get());
        params.emplace_back("return");
        CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTIVATE_WINDOW, WINDOW_VIDEO_NAV, 0, nullptr, "", params);
      }
      else if (targeturl.IsProtocol("musicdb")
               || (targeturl.IsProtocol("special") && targetFile.find("playlists/music") != std::string::npos))
      {
        std::vector<std::string> params;
        params.push_back(targeturl.Get());
        params.emplace_back("return");
        CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTIVATE_WINDOW, WINDOW_MUSIC_NAV, 0, nullptr, "", params);
      }
    }
    else
    {
      CFileItem* item = new CFileItem(targetFile, false);
      if (item->IsVideoDb())
      {
        *(item->GetVideoInfoTag()) = XFILE::CVideoDatabaseFile::GetVideoTag(CURL(item->GetPath()));
        item->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
      }
      CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    }
  }
  else if (action == ACTION_XBMC_RESUME)
  {
    if (m_playback_state != PLAYBACK_STATE_STOPPED)
    {
      if (m_playback_state & PLAYBACK_STATE_VIDEO)
        RequestVisibleBehind(true);
      if (!(m_playback_state & PLAYBACK_STATE_PLAYING))
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
    }
  }
}

void CXBMCApp::onActivityResult(int requestCode, int resultCode, CJNIIntent resultData)
{
  CSingleLock lock(m_activityResultMutex);
  for (auto it = m_activityResultEvents.begin(); it != m_activityResultEvents.end(); ++it)
  {
    if ((*it)->GetRequestCode() == requestCode)
    {
      (*it)->SetResultCode(resultCode);
      (*it)->SetResultData(resultData);
      (*it)->Set();
      m_activityResultEvents.erase(it);
      break;
    }
  }
}

void CXBMCApp::onVisibleBehindCanceled()
{
  CLog::Log(LOGDEBUG, "Visible Behind Cancelled");
  m_hasReqVisible = false;

  // Pressing the pause button calls OnStop() (cf. https://code.google.com/p/android/issues/detail?id=186469)
  if ((m_playback_state & PLAYBACK_STATE_PLAYING))
  {
    if (m_playback_state & PLAYBACK_STATE_CANNOT_PAUSE)
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
    else if (m_playback_state & PLAYBACK_STATE_VIDEO)
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
  }
}

int CXBMCApp::WaitForActivityResult(const CJNIIntent &intent, int requestCode, CJNIIntent &result)
{
  int ret = 0;
  CActivityResultEvent* event = new CActivityResultEvent(requestCode);
  {
    CSingleLock lock(m_activityResultMutex);
    m_activityResultEvents.push_back(event);
  }
  startActivityForResult(intent, requestCode);
  if (event->Wait())
  {
    result = event->GetResultData();
    ret = event->GetResultCode();
  }
  delete event;
  return ret;
}

void CXBMCApp::onVolumeChanged(int volume)
{
  // don't do anything. User wants to use kodi's internal volume freely while
  // using the external volume to change it relatively
  // See: https://forum.kodi.tv/showthread.php?tid=350764
}

void CXBMCApp::onAudioFocusChange(int focusChange)
{
  CXBMCApp::android_printf("Audio Focus changed: %d", focusChange);
  if (focusChange == CJNIAudioManager::AUDIOFOCUS_LOSS)
  {
    if ((m_playback_state & PLAYBACK_STATE_PLAYING))
    {
      if (m_playback_state & PLAYBACK_STATE_CANNOT_PAUSE)
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
      else
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
    }
  }
}

void CXBMCApp::InitFrameCallback(CVideoSyncAndroid* syncImpl)
{
  m_syncImpl = syncImpl;
}

void CXBMCApp::DeinitFrameCallback()
{
  m_syncImpl = NULL;
}

void CXBMCApp::doFrame(int64_t frameTimeNanos)
{
  if (m_syncImpl)
    m_syncImpl->FrameCallback(frameTimeNanos);

  // Calculate the time, when next surface buffer should be rendered
  m_frameTimeNanos = frameTimeNanos;

  m_vsyncEvent.Set();
}

int64_t CXBMCApp::GetNextFrameTime()
{
  if (m_refreshRate > 0.0001f)
    return m_frameTimeNanos + static_cast<int64_t>(1500000000ll / m_refreshRate);
  else
    return m_frameTimeNanos;
}

float CXBMCApp::GetFrameLatencyMs()
{
  return (CurrentHostCounter() - m_frameTimeNanos) * 0.000001;
}

bool CXBMCApp::WaitVSync(unsigned int milliSeconds)
{
  return m_vsyncEvent.WaitMSec(milliSeconds);
}

bool CXBMCApp::GetMemoryInfo(long& availMem, long& totalMem)
{
  if (m_activityManager)
  {
    CJNIActivityManager::MemoryInfo info;
    m_activityManager->getMemoryInfo(info);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      xbmc_jnienv()->ExceptionClear();
      return false;
    }

    availMem = info.availMem();
    totalMem = info.totalMem();

    return true;
  }

  return false;
}

void CXBMCApp::SetupEnv()
{
  setenv("KODI_ANDROID_SYSTEM_LIBS", CJNISystem::getProperty("java.library.path").c_str(), 0);
  setenv("KODI_ANDROID_LIBS", getApplicationInfo().nativeLibraryDir.c_str(), 0);
  setenv("KODI_ANDROID_APK", getPackageResourcePath().c_str(), 0);

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  std::string className = CCompileInfo::GetPackage();

  std::string cacheDir = getCacheDir().getAbsolutePath();
  std::string xbmcHome = CJNISystem::getProperty("xbmc.home", "");
  if (xbmcHome.empty())
  {
    setenv("KODI_BIN_HOME", (cacheDir + "/apk/assets").c_str(), 0);
    setenv("KODI_HOME", (cacheDir + "/apk/assets").c_str(), 0);
  }
  else
  {
    setenv("KODI_BIN_HOME", (xbmcHome + "/assets").c_str(), 0);
    setenv("KODI_HOME", (xbmcHome + "/assets").c_str(), 0);
  }
  setenv("KODI_BINADDON_PATH", (cacheDir + "/lib").c_str(), 0);

  std::string externalDir = CJNISystem::getProperty("xbmc.data", "");
  if (externalDir.empty())
  {
    CJNIFile androidPath = getExternalFilesDir("");
    if (!androidPath)
      androidPath = getDir(className.c_str(), 1);

    if (androidPath)
      externalDir = androidPath.getAbsolutePath();
  }

  if (!externalDir.empty())
    setenv("HOME", externalDir.c_str(), 0);
  else
    setenv("HOME", getenv("KODI_TEMP"), 0);

  std::string pythonPath = cacheDir + "/apk/assets/python3.8";
  setenv("PYTHONHOME", pythonPath.c_str(), 1);
  setenv("PYTHONPATH", "", 1);
  setenv("PYTHONOPTIMIZE","", 1);
  setenv("PYTHONNOUSERSITE", "1", 1);
}

std::string CXBMCApp::GetFilenameFromIntent(const CJNIIntent &intent)
{
    std::string ret;
    if (!intent)
      return ret;
    CJNIURI data = intent.getData();
    if (!data)
      return ret;
    std::string scheme = data.getScheme();
    StringUtils::ToLower(scheme);
    if (scheme == "content")
    {
      std::vector<std::string> filePathColumn;
      filePathColumn.push_back(CJNIMediaStoreMediaColumns::DATA);
      CJNICursor cursor = getContentResolver().query(data, filePathColumn, std::string(), std::vector<std::string>(), std::string());
      if(cursor.moveToFirst())
      {
        int columnIndex = cursor.getColumnIndex(filePathColumn[0]);
        ret = cursor.getString(columnIndex);
      }
      cursor.close();
    }
    else if(scheme == "file")
      ret = data.getPath();
    else
      ret = data.toString();
  return ret;
}

ANativeWindow* CXBMCApp::GetNativeWindow(int timeout)
{
  if (m_window)
    return m_window;

  if (m_mainView)
    m_mainView->waitForSurface(timeout);

  return m_window;
}

void CXBMCApp::RegisterInputDeviceCallbacks(IInputDeviceCallbacks* handler)
{
  if (handler == nullptr)
    return;

  m_inputDeviceCallbacks = handler;
}

void CXBMCApp::UnregisterInputDeviceCallbacks()
{
  m_inputDeviceCallbacks = nullptr;
}

void CXBMCApp::onInputDeviceAdded(int deviceId)
{
  CXBMCApp::android_printf("Input device added: %d", deviceId);

  if (m_inputDeviceCallbacks != nullptr)
    m_inputDeviceCallbacks->OnInputDeviceAdded(deviceId);
}

void CXBMCApp::onInputDeviceChanged(int deviceId)
{
  CXBMCApp::android_printf("Input device changed: %d", deviceId);

  if (m_inputDeviceCallbacks != nullptr)
    m_inputDeviceCallbacks->OnInputDeviceChanged(deviceId);
}

void CXBMCApp::onInputDeviceRemoved(int deviceId)
{
  CXBMCApp::android_printf("Input device removed: %d", deviceId);

  if (m_inputDeviceCallbacks != nullptr)
    m_inputDeviceCallbacks->OnInputDeviceRemoved(deviceId);
}

void CXBMCApp::RegisterInputDeviceEventHandler(IInputDeviceEventHandler* handler)
{
  if (handler == nullptr)
    return;

  m_inputDeviceEventHandler = handler;
}

void CXBMCApp::UnregisterInputDeviceEventHandler()
{
  m_inputDeviceEventHandler = nullptr;
}

bool CXBMCApp::onInputDeviceEvent(const AInputEvent* event)
{
  if (m_inputDeviceEventHandler != nullptr)
    return m_inputDeviceEventHandler->OnInputDeviceEvent(event);

  return false;
}

void CXBMCApp::onDisplayAdded(int displayId)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onDisplayChanged(int displayId)
{
  CLog::Log(LOGDEBUG, "CXBMCApp::%s: id: %d", __FUNCTION__, displayId);

  // Update display modes
  CWinSystemAndroid* winSystemAndroid = dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem());
  if (winSystemAndroid)
    winSystemAndroid->UpdateDisplayModes();

  m_displayChangeEvent.Set();
  m_inputHandler.setDPI(GetDPI());
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onDisplayRemoved(int displayId)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::surfaceCreated(CJNISurfaceHolder holder)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_window = ANativeWindow_fromSurface(xbmc_jnienv(), holder.getSurface().get_raw());
  if (m_window == NULL)
  {
    android_printf(" => invalid ANativeWindow object");
    return;
  }
  if(!m_firstrun)
  {
    XBMC_SetupDisplay();
  }
  g_application.SetRenderGUI(true);
}

void CXBMCApp::surfaceDestroyed(CJNISurfaceHolder holder)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // If we have exited XBMC, it no longer exists.
  g_application.SetRenderGUI(false);
  if (!m_exiting)
  {
    XBMC_DestroyDisplay();
    m_window = NULL;
  }
}
