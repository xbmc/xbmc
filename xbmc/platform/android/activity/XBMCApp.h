/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IActivityHandler.h"
#include "IInputHandler.h"
#include "JNIMainActivity.h"
#include "JNIXBMCAudioManagerOnAudioFocusChangeListener.h"
#include "JNIXBMCDisplayManagerDisplayListener.h"
#include "JNIXBMCMainView.h"
#include "JNIXBMCMediaSession.h"
#include "interfaces/IAnnouncer.h"
#include "platform/xbmc.h"
#include "threads/Event.h"
#include "utils/Geometry.h"

#include <map>
#include <math.h>
#include <memory>
#include <string>
#include <vector>

#include <android/native_activity.h>
#include <androidjni/Activity.h>
#include <androidjni/AudioManager.h>
#include <androidjni/BroadcastReceiver.h>
#include <androidjni/SurfaceHolder.h>
#include <androidjni/View.h>
#include <pthread.h>

// forward declares
class CJNIWakeLock;
class CAESinkAUDIOTRACK;
class CVariant;
class IInputDeviceCallbacks;
class IInputDeviceEventHandler;
class CVideoSyncAndroid;
class CJNIActivityManager;

typedef struct _JNIEnv JNIEnv;

struct androidIcon
{
  unsigned int width;
  unsigned int height;
  void *pixels;
};

struct androidPackage
{
  std::string packageName;
  std::string packageLabel;
  int icon;
};

class CActivityResultEvent : public CEvent
{
public:
  explicit CActivityResultEvent(int requestcode)
    : m_requestcode(requestcode), m_resultcode(0)
  {}
  int GetRequestCode() const { return m_requestcode; }
  int GetResultCode() const { return m_resultcode; }
  void SetResultCode(int resultcode) { m_resultcode = resultcode; }
  CJNIIntent GetResultData() const { return m_resultdata; }
  void SetResultData(const CJNIIntent &resultdata) { m_resultdata = resultdata; }

protected:
  int m_requestcode;
  CJNIIntent m_resultdata;
  int m_resultcode;
};

class CXBMCApp
    : public IActivityHandler
    , public CJNIMainActivity
    , public CJNIBroadcastReceiver
    , public ANNOUNCEMENT::IAnnouncer
    , public CJNISurfaceHolderCallback
{
public:
  explicit CXBMCApp(ANativeActivity* nativeActivity, IInputHandler& inputhandler);
  ~CXBMCApp() override;
  static CXBMCApp* get() { return m_xbmcappinstance; }

  // IAnnouncer IF
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char* sender, const char* message, const CVariant& data) override;

  void onReceive(CJNIIntent intent) override;
  void onNewIntent(CJNIIntent intent) override;
  void onActivityResult(int requestCode, int resultCode, CJNIIntent resultData) override;
  void onVolumeChanged(int volume) override;
  virtual void onAudioFocusChange(int focusChange);
  void doFrame(int64_t frameTimeNanos) override;
  void onVisibleBehindCanceled() override;

  // implementation of CJNIInputManagerInputDeviceListener
  void onInputDeviceAdded(int deviceId) override;
  void onInputDeviceChanged(int deviceId) override;
  void onInputDeviceRemoved(int deviceId) override;

  // implementation of DisplayManager::DisplayListener
  void onDisplayAdded(int displayId) override;
  void onDisplayChanged(int displayId) override;
  void onDisplayRemoved(int displayId) override;
  jni::jhobject getDisplayListener() { return m_displayListener.get_raw(); }

  bool isValid() { return m_activity != NULL; }
  const ANativeActivity *getActivity() const { return m_activity; }

  void onStart() override;
  void onResume() override;
  void onPause() override;
  void onStop() override;
  void onDestroy() override;

  void onSaveState(void **data, size_t *size) override;
  void onConfigurationChanged() override;
  void onLowMemory() override;

  void onCreateWindow(ANativeWindow* window) override;
  void onResizeWindow() override;
  void onDestroyWindow() override;
  void onGainFocus() override;
  void onLostFocus() override;

  void Initialize();
  void Deinitialize();

  static ANativeWindow* GetNativeWindow(int timeout);
  static int SetBuffersGeometry(int width, int height, int format);
  static int android_printf(const char *format, ...);

  static int GetBatteryLevel();
  static bool EnableWakeLock(bool on);
  static bool HasFocus() { return m_hasFocus; }
  static bool IsHeadsetPlugged();
  static bool IsHDMIPlugged();

  static bool StartActivity(const std::string &package, const std::string &intent = std::string(), const std::string &dataType = std::string(), const std::string &dataURI = std::string());
  static std::vector <androidPackage> GetApplications();

  /*!
   * \brief If external storage is available, it returns the path for the external storage (for the specified type)
   * \param path will contain the path of the external storage (for the specified type)
   * \param type optional type. Possible values are "", "files", "music", "videos", "pictures", "photos, "downloads"
   * \return true if external storage is available and a valid path has been stored in the path parameter
   */
  static bool GetExternalStorage(std::string &path, const std::string &type = "");
  static bool GetStorageUsage(const std::string &path, std::string &usage);
  static int GetMaxSystemVolume();
  static float GetSystemVolume();
  static void SetSystemVolume(float percent);

  static void SetRefreshRate(float rate);
  static void SetDisplayMode(int mode, float rate);
  static int GetDPI();

  static CRect MapRenderToDroid(const CRect& srcRect);
  static int WaitForActivityResult(const CJNIIntent &intent, int requestCode, CJNIIntent& result);

  // Playback callbacks
  void OnPlayBackStarted();
  void OnPlayBackPaused();
  void OnPlayBackStopped();

  // Info callback
  void UpdateSessionMetadata();
  void UpdateSessionState();

  // input device methods
  static void RegisterInputDeviceCallbacks(IInputDeviceCallbacks* handler);
  static void UnregisterInputDeviceCallbacks();
  static const CJNIViewInputDevice GetInputDevice(int deviceId);
  static std::vector<int> GetInputDeviceIds();

  static void RegisterInputDeviceEventHandler(IInputDeviceEventHandler* handler);
  static void UnregisterInputDeviceEventHandler();
  static bool onInputDeviceEvent(const AInputEvent* event);

  static void InitFrameCallback(CVideoSyncAndroid *syncImpl);
  static void DeinitFrameCallback();

  // Application slow ping
  void ProcessSlow();

  static bool WaitVSync(unsigned int milliSeconds);
  static int64_t GetNextFrameTime();
  static float GetFrameLatencyMs();

  bool getVideosurfaceInUse();
  void setVideosurfaceInUse(bool videosurfaceInUse);

  bool GetMemoryInfo(long& availMem, long& totalMem);
protected:
  // limit who can access Volume
  friend class CAESinkAUDIOTRACK;

  static int GetMaxSystemVolume(JNIEnv *env);
  bool AcquireAudioFocus();
  bool ReleaseAudioFocus();
  static void RequestVisibleBehind(bool requested);

private:
  static CXBMCApp* m_xbmcappinstance;
  CJNIXBMCAudioManagerOnAudioFocusChangeListener m_audioFocusListener;
  CJNIXBMCDisplayManagerDisplayListener m_displayListener;
  static std::unique_ptr<CJNIXBMCMainView> m_mainView;
  std::unique_ptr<jni::CJNIXBMCMediaSession> m_mediaSession;
  static bool HasLaunchIntent(const std::string &package);
  std::string GetFilenameFromIntent(const CJNIIntent &intent);

  void run();
  void stop();
  void SetupEnv();
  static void SetRefreshRateCallback(CVariant *rate);
  static void SetDisplayModeCallback(CVariant *mode);
  static void RegisterDisplayListener(CVariant*);

  static ANativeActivity *m_activity;
  IInputHandler& m_inputHandler;
  static CJNIWakeLock *m_wakeLock;
  static int m_batteryLevel;
  static bool m_hasFocus;
  static bool m_headsetPlugged;
  static bool m_hdmiPlugged;
  static bool m_hdmiReportedState;
  static bool m_hdmiSource;
  static IInputDeviceCallbacks* m_inputDeviceCallbacks;
  static IInputDeviceEventHandler* m_inputDeviceEventHandler;
  static bool m_hasReqVisible;
  bool m_videosurfaceInUse;
  bool m_firstrun;
  bool m_exiting;
  pthread_t m_thread;
  static CCriticalSection m_applicationsMutex;
  static std::vector<androidPackage> m_applications;
  static std::vector<CActivityResultEvent*> m_activityResultEvents;

  static ANativeWindow* m_window;

  static CVideoSyncAndroid* m_syncImpl;
  static CEvent m_vsyncEvent;
  static CEvent m_displayChangeEvent;

  std::unique_ptr<CJNIActivityManager> m_activityManager;

  void XBMC_Pause(bool pause);
  void XBMC_Stop();
  bool XBMC_DestroyDisplay();
  bool XBMC_SetupDisplay();

  static uint32_t m_playback_state;
  static int64_t m_frameTimeNanos;
  static float m_refreshRate;

public:
  // CJNISurfaceHolderCallback interface
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  void surfaceCreated(CJNISurfaceHolder holder) override;
  void surfaceDestroyed(CJNISurfaceHolder holder) override;
};
