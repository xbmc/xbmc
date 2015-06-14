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

#include <sstream>

#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include <android/native_window.h>
#include <android/configuration.h>
#include <jni.h>

#include "XBMCApp.h"

#include "input/MouseStat.h"
#include "input/XBMC_keysym.h"
#include "input/Key.h"
#include "windowing/XBMC_events.h"
#include <android/log.h>

#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "xbmc.h"
#include "windowing/WinEvents.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "AppParamParser.h"
#include "XbmcContext.h"
#include <android/bitmap.h>
#include "android/jni/JNIThreading.h"
#include "android/jni/BroadcastReceiver.h"
#include "android/jni/Intent.h"
#include "android/jni/PackageManager.h"
#include "android/jni/Context.h"
#include "android/jni/PowerManager.h"
#include "android/jni/WakeLock.h"
#include "android/jni/Environment.h"
#include "android/jni/File.h"
#include "android/jni/IntentFilter.h"
#include "android/jni/NetworkInfo.h"
#include "android/jni/ConnectivityManager.h"
#include "android/jni/System.h"
#include "android/jni/ApplicationInfo.h"
#include "android/jni/StatFs.h"
#include "android/jni/BitmapDrawable.h"
#include "android/jni/Bitmap.h"
#include "android/jni/CharSequence.h"
#include "android/jni/URI.h"
#include "android/jni/Cursor.h"
#include "android/jni/ContentResolver.h"
#include "android/jni/MediaStore.h"
#include "android/jni/Build.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif
#include "android/jni/Window.h"
#include "android/jni/WindowManager.h"

#include "CompileInfo.h"

#define GIGABYTES       1073741824

using namespace std;

template<class T, void(T::*fn)()>
void* thread_run(void* obj)
{
  (static_cast<T*>(obj)->*fn)();
  return NULL;
}

CXBMCApp* CXBMCApp::m_xbmcappinstance = NULL;
CEvent CXBMCApp::m_windowCreated;
ANativeActivity *CXBMCApp::m_activity = NULL;
CJNIWakeLock *CXBMCApp::m_wakeLock = NULL;
ANativeWindow* CXBMCApp::m_window = NULL;
int CXBMCApp::m_batteryLevel = 0;
bool CXBMCApp::m_hasFocus = false;
CCriticalSection CXBMCApp::m_applicationsMutex;
std::vector<androidPackage> CXBMCApp::m_applications;


CXBMCApp::CXBMCApp(ANativeActivity* nativeActivity)
  : CJNIApplicationMainActivity(nativeActivity)
  , CJNIBroadcastReceiver("org/xbmc/kodi/XBMCBroadcastReceiver")
{
  m_xbmcappinstance = this;
  m_activity = nativeActivity;
  m_firstrun = true;
  m_exiting=false;
  if (m_activity == NULL)
  {
    android_printf("CXBMCApp: invalid ANativeActivity instance");
    exit(1);
    return;
  }
}

CXBMCApp::~CXBMCApp()
{
  m_xbmcappinstance = NULL;
  delete m_wakeLock;
}

void CXBMCApp::onStart()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

#if defined(HAS_LIBAMCODEC)
  if (aml_permissions())
  {
    // non-aml boxes will ignore this intent broadcast.
    // setup aml scalers to play video as is, unscaled.
    CJNIIntent intent_aml_video_on = CJNIIntent("android.intent.action.REALVIDEO_ON");
    sendBroadcast(intent_aml_video_on);
  }
#endif

  if (!m_firstrun)
  {
    android_printf("%s: Already running, ignoring request to start", __PRETTY_FUNCTION__);
    return;
  }
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_create(&m_thread, &attr, thread_run<CXBMCApp, &CXBMCApp::run>, this);
  pthread_attr_destroy(&attr);
}

void CXBMCApp::onResume()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  CJNIIntentFilter intentFilter;
  intentFilter.addAction("android.intent.action.BATTERY_CHANGED");
  intentFilter.addAction("android.intent.action.DREAMING_STOPPED");
  intentFilter.addAction("android.intent.action.SCREEN_ON");
  registerReceiver(*this, intentFilter);

  if (!g_application.IsInScreenSaver())
    EnableWakeLock(true);
  else
    g_application.WakeUpScreenSaverAndDPMS();

  // Clear the applications cache. We could have installed/deinstalled apps
  {
    CSingleLock lock(m_applicationsMutex);
    m_applications.clear();
  }
}

void CXBMCApp::onPause()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  unregisterReceiver(*this);

#if defined(HAS_LIBAMCODEC)
  if (aml_permissions())
  {
    // non-aml boxes will ignore this intent broadcast.
    CJNIIntent intent_aml_video_off = CJNIIntent("android.intent.action.REALVIDEO_OFF");
    sendBroadcast(intent_aml_video_off);
  }
#endif

  EnableWakeLock(false);
}

void CXBMCApp::onStop()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onDestroy()
{
  android_printf("%s", __PRETTY_FUNCTION__);

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
  if (window == NULL)
  {
    android_printf(" => invalid ANativeWindow object");
    return;
  }
  m_window = window;
  m_windowCreated.Set();
  if(!m_firstrun)
  {
    XBMC_SetupDisplay();
    XBMC_Pause(false);
  }
}

void CXBMCApp::onResizeWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_window=NULL;
  m_windowCreated.Reset();
  // no need to do anything because we are fixed in fullscreen landscape mode
}

void CXBMCApp::onDestroyWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  // If we have exited XBMC, it no longer exists.
  if (!m_exiting)
  {
    XBMC_DestroyDisplay();
    XBMC_Pause(true);
  }
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

bool CXBMCApp::EnableWakeLock(bool on)
{
  android_printf("%s: %s", __PRETTY_FUNCTION__, on ? "true" : "false");
  if (!m_wakeLock)
  {
    std::string appName = CCompileInfo::GetAppName();
    StringUtils::ToLower(appName);
    std::string className = "org.xbmc." + appName;
    // SCREEN_BRIGHT_WAKE_LOCK is marked as deprecated but there is no real alternatives for now
    m_wakeLock = new CJNIWakeLock(CJNIPowerManager(getSystemService("power")).newWakeLock(CJNIPowerManager::SCREEN_BRIGHT_WAKE_LOCK, className.c_str()));
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
  int result = audioManager.requestAudioFocus(*m_xbmcappinstance,
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
  int result = audioManager.abandonAudioFocus(*m_xbmcappinstance);
  if (result != CJNIAudioManager::AUDIOFOCUS_REQUEST_GRANTED)
  {
    CXBMCApp::android_printf("Audio Focus abandon failed");
    return false;
  }
  return true;
}

bool CXBMCApp::HasFocus()
{
  return m_hasFocus;
}

void CXBMCApp::run()
{
  int status = 0;

  SetupEnv();
  XBMC::Context context;

  CJNIIntent startIntent = getIntent();

  android_printf("%s Started with action: %s\n", CCompileInfo::GetAppName(), startIntent.getAction().c_str());

  std::string filenameToPlay = GetFilenameFromIntent(startIntent);
  if (!filenameToPlay.empty())
  {
    int argc = 2;
    const char** argv = (const char**) malloc(argc*sizeof(char*));

    std::string exe_name(CCompileInfo::GetAppName());
    argv[0] = exe_name.c_str();
    argv[1] = filenameToPlay.c_str();

    CAppParamParser appParamParser;
    appParamParser.Parse((const char **)argv, argc);

    free(argv);
  }

  m_firstrun=false;
  android_printf(" => running XBMC_Run...");
  try
  {
    status = XBMC_Run(true);
    android_printf(" => XBMC_Run finished with %d", status);
  }
  catch(...)
  {
    android_printf("ERROR: Exception caught on main loop. Exiting");
  }

  // If we are have not been force by Android to exit, notify its finish routine.
  // This will cause android to run through its teardown events, it calls:
  // onPause(), onLostFocus(), onDestroyWindow(), onStop(), onDestroy().
  ANativeActivity_finish(m_activity);
  m_exiting=true;
}

void CXBMCApp::XBMC_Pause(bool pause)
{
  android_printf("XBMC_Pause(%s)", pause ? "true" : "false");
  // Only send the PAUSE action if we are pausing XBMC and video is currently playing
  if (pause && g_application.m_pPlayer->IsPlayingVideo() && !g_application.m_pPlayer->IsPaused())
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PAUSE), WINDOW_INVALID, true);
}

void CXBMCApp::XBMC_Stop()
{
  CApplicationMessenger::Get().Quit();
}

bool CXBMCApp::XBMC_SetupDisplay()
{
  android_printf("XBMC_SetupDisplay()");
  return CApplicationMessenger::Get().SetupDisplay();
}

bool CXBMCApp::XBMC_DestroyDisplay()
{
  android_printf("XBMC_DestroyDisplay()");
  return CApplicationMessenger::Get().DestroyDisplay();
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
    if (params.getpreferredRefreshRate() != rate)
    {
      params.setpreferredRefreshRate(rate);
      if (params.getpreferredRefreshRate() > 0.0)
        window.setAttributes(params);
    }
  }
}

void CXBMCApp::SetRefreshRate(float rate)
{
  if (rate < 1.0)
    return;

  CVariant *variant = new CVariant(rate);
  runNativeOnUiThread(SetRefreshRateCallback, variant);
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

void CXBMCApp::OnPlayBackStarted()
{
  AcquireAudioFocus();
}

void CXBMCApp::OnPlayBackPaused()
{
  ReleaseAudioFocus();
}

void CXBMCApp::OnPlayBackResumed()
{
  AcquireAudioFocus();
}

void CXBMCApp::OnPlayBackStopped()
{
  ReleaseAudioFocus();
}

void CXBMCApp::OnPlayBackEnded()
{
  ReleaseAudioFocus();
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
      androidPackage newPackage;
      newPackage.packageName = packageList.get(i).packageName;
      newPackage.packageLabel = GetPackageManager().getApplicationLabel(packageList.get(i)).toString();
      CJNIIntent intent = GetPackageManager().getLaunchIntentForPackage(newPackage.packageName);
      if (!intent && CJNIBuild::SDK_INT >= 21)
        intent = GetPackageManager().getLeanbackLaunchIntentForPackage(newPackage.packageName);
      if (!intent)
        continue;

      m_applications.push_back(newPackage);
    }
  }

  return m_applications;
}

bool CXBMCApp::GetIconSize(const string &packageName, int *width, int *height)
{
  JNIEnv* env = xbmc_jnienv();
  AndroidBitmapInfo info;
  CJNIBitmapDrawable drawable = (CJNIBitmapDrawable)GetPackageManager().getApplicationIcon(packageName);
  CJNIBitmap icon(drawable.getBitmap());
  AndroidBitmap_getInfo(env, icon.get_raw(), &info);
  *width = info.width;
  *height = info.height;
  return true;
}

bool CXBMCApp::GetIcon(const string &packageName, void* buffer, unsigned int bufSize)
{
  void *bitmapBuf = NULL;
  JNIEnv* env = xbmc_jnienv();
  CJNIBitmapDrawable drawable = (CJNIBitmapDrawable)GetPackageManager().getApplicationIcon(packageName);
  CJNIBitmap bitmap(drawable.getBitmap());
  AndroidBitmap_lockPixels(env, bitmap.get_raw(), &bitmapBuf);
  if (bitmapBuf)
  {
    memcpy(buffer, bitmapBuf, bufSize);
    AndroidBitmap_unlockPixels(env, bitmap.get_raw());
    return true;
  }
  return false;
}

bool CXBMCApp::HasLaunchIntent(const string &package)
{
  return GetPackageManager().getLaunchIntentForPackage(package) != NULL;
}

// Note intent, dataType, dataURI all default to ""
bool CXBMCApp::StartActivity(const string &package, const string &intent, const string &dataType, const string &dataURI)
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
  std::string action = intent.getAction();
  android_printf("CXBMCApp::onReceive Got intent. Action: %s", action.c_str());
  if (action == "android.intent.action.BATTERY_CHANGED")
    m_batteryLevel = intent.getIntExtra("level",-1);
  else if (action == "android.intent.action.DREAMING_STOPPED" || action == "android.intent.action.SCREEN_ON")
    if (HasFocus())
      g_application.WakeUpScreenSaverAndDPMS();
}

void CXBMCApp::onNewIntent(CJNIIntent intent)
{
  std::string action = intent.getAction();
  if (action == "android.intent.action.VIEW")
  {
    std::string playFile = GetFilenameFromIntent(intent);
    CApplicationMessenger::Get().MediaPlay(playFile);
  }
}

void CXBMCApp::onVolumeChanged(int volume)
{
  CApplicationMessenger::Get().SendAction(CAction(ACTION_VOLUME_SET, (float)volume), WINDOW_INVALID, false);
}

void CXBMCApp::onAudioFocusChange(int focusChange)
{
  CXBMCApp::android_printf("Audio Focus changed: %d", focusChange);
  if (focusChange == CJNIAudioManager::AUDIOFOCUS_LOSS && g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PAUSE), WINDOW_INVALID, true);
}

void CXBMCApp::SetupEnv()
{
  setenv("XBMC_ANDROID_SYSTEM_LIBS", CJNISystem::getProperty("java.library.path").c_str(), 0);
  setenv("XBMC_ANDROID_LIBS", getApplicationInfo().nativeLibraryDir.c_str(), 0);
  setenv("XBMC_ANDROID_APK", getPackageResourcePath().c_str(), 0);

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  std::string className = "org.xbmc." + appName;

  std::string xbmcHome = CJNISystem::getProperty("xbmc.home", "");
  if (xbmcHome.empty())
  {
    std::string cacheDir = getCacheDir().getAbsolutePath();
    setenv("KODI_BIN_HOME", (cacheDir + "/apk/assets").c_str(), 0);
    setenv("KODI_HOME", (cacheDir + "/apk/assets").c_str(), 0);
  }
  else
  {
    setenv("KODI_BIN_HOME", (xbmcHome + "/assets").c_str(), 0);
    setenv("KODI_HOME", (xbmcHome + "/assets").c_str(), 0);
  }

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

  std::string apkPath = getenv("XBMC_ANDROID_APK");
  apkPath += "/assets/python2.6";
  setenv("PYTHONHOME", apkPath.c_str(), 1);
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

const ANativeWindow** CXBMCApp::GetNativeWindow(int timeout)
{
  if (m_window)
    return (const ANativeWindow**)&m_window;

  m_windowCreated.WaitMSec(timeout);
  return (const ANativeWindow**)&m_window;
}
