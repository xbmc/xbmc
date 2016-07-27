#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <math.h>
#include <pthread.h>
#include <string>
#include <vector>

#include <android/native_activity.h>

#include "IActivityHandler.h"
#include "IInputHandler.h"

#include "platform/xbmc.h"
#include "platform/android/jni/Activity.h"
#include "platform/android/jni/BroadcastReceiver.h"
#include "platform/android/jni/AudioManager.h"
#include "platform/android/jni/View.h"
#include "threads/Event.h"

#include "JNIMainActivity.h"

// forward delares
class CJNIWakeLock;
class CAESinkAUDIOTRACK;
class CVariant;
class IInputDeviceCallbacks;
class IInputDeviceEventHandler;
class CVideoSyncAndroid;
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

class CXBMCApp : public IActivityHandler, public CJNIMainActivity,
                 public CJNIBroadcastReceiver,
                 public CJNIAudioManagerAudioFocusChangeListener
{
public:
  CXBMCApp(ANativeActivity *nativeActivity);
  virtual ~CXBMCApp();
  virtual void onReceive(CJNIIntent intent);
  virtual void onNewIntent(CJNIIntent intent);
  virtual void onVolumeChanged(int volume);
  virtual void onAudioFocusChange(int focusChange);
  virtual void doFrame(int64_t frameTimeNanos);

  // implementation of CJNIInputManagerInputDeviceListener
  void onInputDeviceAdded(int deviceId) override;
  void onInputDeviceChanged(int deviceId) override;
  void onInputDeviceRemoved(int deviceId) override;

  bool isValid() { return m_activity != NULL; }

  void onStart();
  void onResume();
  void onPause();
  void onStop();
  void onDestroy();

  void onSaveState(void **data, size_t *size);
  void onConfigurationChanged();
  void onLowMemory();

  void onCreateWindow(ANativeWindow* window);
  void onResizeWindow();
  void onDestroyWindow();
  void onGainFocus();
  void onLostFocus();


  static const ANativeWindow** GetNativeWindow(int timeout);
  static int SetBuffersGeometry(int width, int height, int format);
  static int android_printf(const char *format, ...);
  
  static int GetBatteryLevel();
  static bool EnableWakeLock(bool on);
  static bool HasFocus();
  static bool IsHeadsetPlugged();

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
  static void InitDirectories();

  static void SetRefreshRate(float rate);
  static int GetDPI();

  // Playback callbacks
  static void OnPlayBackStarted();
  static void OnPlayBackPaused();
  static void OnPlayBackResumed();
  static void OnPlayBackStopped();
  static void OnPlayBackEnded();

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

  static bool WaitVSync(unsigned int milliSeconds);

  static CXBMCApp* get() { return m_xbmcappinstance; }

protected:
  // limit who can access Volume
  friend class CAESinkAUDIOTRACK;

  static int GetMaxSystemVolume(JNIEnv *env);
  static bool AcquireAudioFocus();
  static bool ReleaseAudioFocus();

private:
  static CXBMCApp* m_xbmcappinstance;
  static bool HasLaunchIntent(const std::string &package);
  std::string GetFilenameFromIntent(const CJNIIntent &intent);
  void run();
  void stop();
  void SetupEnv();
  static void SetRefreshRateCallback(CVariant *rate);
  static ANativeActivity *m_activity;
  static CJNIWakeLock *m_wakeLock;
  static int m_batteryLevel;
  static bool m_hasFocus;
  static bool m_headsetPlugged;
  static IInputDeviceCallbacks* m_inputDeviceCallbacks;
  static IInputDeviceEventHandler* m_inputDeviceEventHandler;
  bool m_firstrun;
  bool m_exiting;
  pthread_t m_thread;
  static CCriticalSection m_applicationsMutex;
  static std::vector<androidPackage> m_applications;

  static ANativeWindow* m_window;
  static CEvent m_windowCreated;

  static CVideoSyncAndroid* m_syncImpl;
  static CEvent m_vsyncEvent;

  void XBMC_Pause(bool pause);
  void XBMC_Stop();
  bool XBMC_DestroyDisplay();
  bool XBMC_SetupDisplay();
};
