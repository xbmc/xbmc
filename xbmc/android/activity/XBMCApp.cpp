/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#include "guilib/Key.h"
#include "windowing/XBMC_events.h"
#include <android/log.h>

#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "xbmc.h"
#include "windowing/WinEvents.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "ApplicationMessenger.h"
#include <android/bitmap.h>
#include "android/jni/JNIThreading.h"
#include "android/jni/BroadcastReceiver.h"
#include "android/jni/Intent.h"
#include "android/jni/PackageManager.h"
#include "android/jni/Context.h"
#include "android/jni/AudioManager.h"
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

#define GIGABYTES       1073741824

using namespace std;

template<class T, void(T::*fn)()>
void* thread_run(void* obj)
{
  (static_cast<T*>(obj)->*fn)();
  return NULL;
}

ANativeActivity *CXBMCApp::m_activity = NULL;
ANativeWindow* CXBMCApp::m_window = NULL;

CXBMCApp::CXBMCApp(ANativeActivity* nativeActivity)
  : CJNIContext(nativeActivity), m_wakeLock(NULL)
{
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
}

void CXBMCApp::onStart()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
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
}

void CXBMCApp::onPause()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
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

  if (m_wakeLock != NULL && m_activity != NULL)
  {
    JNIEnv* env = xbmc_jnienv();
    env->DeleteGlobalRef(m_wakeLock);
    m_wakeLock = NULL;
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
  acquireWakeLock();
  if(!m_firstrun)
  {
    XBMC_SetupDisplay();
    XBMC_Pause(false);
  }
}

void CXBMCApp::onResizeWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
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

  releaseWakeLock();
  m_window=NULL;
}

void CXBMCApp::onGainFocus()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onLostFocus()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

bool CXBMCApp::getWakeLock(JNIEnv *env)
{
  android_printf("%s", __PRETTY_FUNCTION__);
  if (m_activity == NULL)
  {
    android_printf("  missing activity => unable to use WakeLocks");
    return false;
  }

  if (env == NULL)
    return false;

  if (m_wakeLock == NULL)
  {
    jobject oActivity = m_activity->clazz;
    jclass cActivity = env->GetObjectClass(oActivity);

    // get the wake lock
    jmethodID midActivityGetSystemService = env->GetMethodID(cActivity, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring sPowerService = env->NewStringUTF("power"); // POWER_SERVICE
    jobject oPowerManager = env->CallObjectMethod(oActivity, midActivityGetSystemService, sPowerService);

    jclass cPowerManager = env->GetObjectClass(oPowerManager);
    jmethodID midNewWakeLock = env->GetMethodID(cPowerManager, "newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;");
    jstring sXbmcPackage = env->NewStringUTF("org.xbmc.xbmc");
    jobject oWakeLock = env->CallObjectMethod(oPowerManager, midNewWakeLock, (jint)0x1a /* FULL_WAKE_LOCK */, sXbmcPackage);
    m_wakeLock = env->NewGlobalRef(oWakeLock);

    env->DeleteLocalRef(oWakeLock);
    env->DeleteLocalRef(cPowerManager);
    env->DeleteLocalRef(oPowerManager);
    env->DeleteLocalRef(sPowerService);
    env->DeleteLocalRef(sXbmcPackage);
    env->DeleteLocalRef(cActivity);
  }

  return m_wakeLock != NULL;
}

void CXBMCApp::acquireWakeLock()
{
  if (m_activity == NULL)
    return;

  JNIEnv* env = xbmc_jnienv();

  if (!getWakeLock(env))
  {
    android_printf("%s: unable to acquire a WakeLock");
    return;
  }

  jclass cWakeLock = env->GetObjectClass(m_wakeLock);
  jmethodID midWakeLockAcquire = env->GetMethodID(cWakeLock, "acquire", "()V");
  env->CallVoidMethod(m_wakeLock, midWakeLockAcquire);
  env->DeleteLocalRef(cWakeLock);
}

void CXBMCApp::releaseWakeLock()
{
  if (m_activity == NULL)
    return;

  JNIEnv* env = xbmc_jnienv();
  if (!getWakeLock(env))
  {
    android_printf("%s: unable to release a WakeLock");
    return;
  }

  jclass cWakeLock = env->GetObjectClass(m_wakeLock);
  jmethodID midWakeLockRelease = env->GetMethodID(cWakeLock, "release", "()V");
  env->CallVoidMethod(m_wakeLock, midWakeLockRelease);
  env->DeleteLocalRef(cWakeLock);
}

void CXBMCApp::run()
{
  int status = 0;

  SetupEnv();
  android_printf(" => waiting for a window");
  // Hack!
  // TODO: Change EGL startup so that we can start headless, then create the
  // window once android gives us a surface to play with.
  while(!m_window)
    usleep(1000);
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
  // Only send the PAUSE action if we are pausing XBMC and something is currently playing
  if (pause && g_application.IsPlaying() && !g_application.IsPaused())
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

int CXBMCApp::android_printf(const char *format, ...)
{
  // For use before CLog is setup by XBMC_Run()
  va_list args;
  va_start(args, format);
  int result = __android_log_vprint(ANDROID_LOG_VERBOSE, "XBMC", format, args);
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

bool CXBMCApp::ListApplications(vector<androidPackage> *applications)
{
  if (!m_activity)
    return false;

  JNIEnv* env = xbmc_jnienv();
  jobject oActivity = m_activity->clazz;
  jclass cActivity = env->GetObjectClass(oActivity);

  // oPackageManager = new PackageManager();
  jmethodID mgetPackageManager = env->GetMethodID(cActivity, "getPackageManager", "()Landroid/content/pm/PackageManager;");
  jobject oPackageManager = (jobject)env->CallObjectMethod(oActivity, mgetPackageManager);
  env->DeleteLocalRef(cActivity);

  // adata[] = oPackageManager.getInstalledApplications(0);
  jclass cPackageManager = env->GetObjectClass(oPackageManager);
  jmethodID mgetInstalledApplications = env->GetMethodID(cPackageManager, "getInstalledApplications", "(I)Ljava/util/List;");
  jmethodID mgetApplicationLabel = env->GetMethodID(cPackageManager, "getApplicationLabel", "(Landroid/content/pm/ApplicationInfo;)Ljava/lang/CharSequence;");
  jobject odata = env->CallObjectMethod(oPackageManager, mgetInstalledApplications, 0);
  jclass cdata = env->GetObjectClass(odata);
  jmethodID mtoArray = env->GetMethodID(cdata, "toArray", "()[Ljava/lang/Object;");
  jobjectArray adata = (jobjectArray)env->CallObjectMethod(odata, mtoArray);
  env->DeleteLocalRef(cdata);
  env->DeleteLocalRef(odata);
  env->DeleteLocalRef(cPackageManager);

  int size = env->GetArrayLength(adata);
  for (int i = 0; i < size; i++)
  {
    // oApplicationInfo = adata[i];
    jobject oApplicationInfo = env->GetObjectArrayElement(adata, i);
    jclass cApplicationInfo = env->GetObjectClass(oApplicationInfo);
    jfieldID mclassName = env->GetFieldID(cApplicationInfo, "packageName", "Ljava/lang/String;");
    jstring sapplication = (jstring)env->GetObjectField(oApplicationInfo, mclassName);

    if (!sapplication)
    {
      env->DeleteLocalRef(cApplicationInfo);
      env->DeleteLocalRef(oApplicationInfo);
      continue;
    }
    // cname = oApplicationInfo.packageName;
    const char* cname = env->GetStringUTFChars(sapplication, NULL);
    androidPackage desc;
    desc.packageName = cname;
    env->ReleaseStringUTFChars(sapplication, cname);
    env->DeleteLocalRef(sapplication);
    env->DeleteLocalRef(cApplicationInfo);

    jstring spackageLabel = (jstring) env->CallObjectMethod(oPackageManager, mgetApplicationLabel, oApplicationInfo);
    if (!spackageLabel)
    {
      env->DeleteLocalRef(oApplicationInfo);
      continue;
    }
    // cname = opackageManager.getApplicationLabel(oApplicationInfo);
    const char* cpackageLabel = env->GetStringUTFChars(spackageLabel, NULL);
    desc.packageLabel = cpackageLabel;
    env->ReleaseStringUTFChars(spackageLabel, cpackageLabel);
    env->DeleteLocalRef(spackageLabel);
    env->DeleteLocalRef(oApplicationInfo);

    if (!HasLaunchIntent(desc.packageName))
      continue;

    applications->push_back(desc);
  }
  env->DeleteLocalRef(oPackageManager);
  return true;
}

bool CXBMCApp::GetIconSize(const string &packageName, int *width, int *height)
{
  if (!m_activity)
    return false;

  jthrowable exc;
  JNIEnv* env = xbmc_jnienv();

  jobject oActivity = m_activity->clazz;
  jclass cActivity = env->GetObjectClass(oActivity);

  // oPackageManager = new PackageManager();
  jmethodID mgetPackageManager = env->GetMethodID(cActivity, "getPackageManager", "()Landroid/content/pm/PackageManager;");
  jobject oPackageManager = (jobject)env->CallObjectMethod(oActivity, mgetPackageManager);
  env->DeleteLocalRef(cActivity);

  jclass cPackageManager = env->GetObjectClass(oPackageManager);
  jmethodID mgetApplicationIcon = env->GetMethodID(cPackageManager, "getApplicationIcon", "(Ljava/lang/String;)Landroid/graphics/drawable/Drawable;");
  env->DeleteLocalRef(cPackageManager);

  jclass cBitmapDrawable = env->FindClass("android/graphics/drawable/BitmapDrawable");
  jmethodID mBitmapDrawableCtor = env->GetMethodID(cBitmapDrawable, "<init>", "()V");
  jmethodID mgetBitmap = env->GetMethodID(cBitmapDrawable, "getBitmap", "()Landroid/graphics/Bitmap;");

  // BitmapDrawable oBitmapDrawable;
  jobject oBitmapDrawable = env->NewObject(cBitmapDrawable, mBitmapDrawableCtor);
  jstring sPackageName = env->NewStringUTF(packageName.c_str());

  // oBitmapDrawable = oPackageManager.getApplicationIcon(sPackageName)
  oBitmapDrawable =  env->CallObjectMethod(oPackageManager, mgetApplicationIcon, sPackageName);
  jobject oBitmap = env->CallObjectMethod(oBitmapDrawable, mgetBitmap);
  env->DeleteLocalRef(sPackageName);
  env->DeleteLocalRef(cBitmapDrawable);
  env->DeleteLocalRef(oBitmapDrawable);
  env->DeleteLocalRef(oPackageManager);
  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::GetIconSize Error getting icon size for  %s. Exception follows:", packageName.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(oBitmap);
    return false;
  } 
  jclass cBitmap = env->GetObjectClass(oBitmap);
  jmethodID mgetWidth = env->GetMethodID(cBitmap, "getWidth", "()I");
  jmethodID mgetHeight = env->GetMethodID(cBitmap, "getHeight", "()I");
  env->DeleteLocalRef(cBitmap);

  // width = oBitmap.getWidth;
  *width = (int)env->CallIntMethod(oBitmap, mgetWidth);

  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::GetIconSize Error getting icon width for %s. Exception follows:", packageName.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(oBitmap);
    return false;
  }
  // height = oBitmap.getHeight;
  *height = (int)env->CallIntMethod(oBitmap, mgetHeight);
  env->DeleteLocalRef(oBitmap);

  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::GetIconSize Error getting icon height for %s. Exception follows:", packageName.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  return true;
}

bool CXBMCApp::GetIcon(const string &packageName, void* buffer, unsigned int bufSize)
{
  if (!m_activity)
    return false;

  jthrowable exc;
  JNIEnv* env = xbmc_jnienv();

  CLog::Log(LOGERROR, "CXBMCApp::GetIconSize Looking for: %s", packageName.c_str());

  jobject oActivity = m_activity->clazz;
  jclass cActivity = env->GetObjectClass(oActivity);

  // oPackageManager = new PackageManager();
  jmethodID mgetPackageManager = env->GetMethodID(cActivity, "getPackageManager", "()Landroid/content/pm/PackageManager;");
  jobject oPackageManager = (jobject)env->CallObjectMethod(oActivity, mgetPackageManager);
  env->DeleteLocalRef(cActivity);

  jclass cPackageManager = env->GetObjectClass(oPackageManager);
  jmethodID mgetApplicationIcon = env->GetMethodID(cPackageManager, "getApplicationIcon", "(Ljava/lang/String;)Landroid/graphics/drawable/Drawable;");
  env->DeleteLocalRef(cPackageManager);

  jclass cBitmapDrawable = env->FindClass("android/graphics/drawable/BitmapDrawable");
  jmethodID mBitmapDrawableCtor = env->GetMethodID(cBitmapDrawable, "<init>", "()V");
  jmethodID mgetBitmap = env->GetMethodID(cBitmapDrawable, "getBitmap", "()Landroid/graphics/Bitmap;");

   // BitmapDrawable oBitmapDrawable;
  jobject oBitmapDrawable = env->NewObject(cBitmapDrawable, mBitmapDrawableCtor);
  jstring sPackageName = env->NewStringUTF(packageName.c_str());

  // oBitmapDrawable = oPackageManager.getApplicationIcon(sPackageName)
  oBitmapDrawable =  env->CallObjectMethod(oPackageManager, mgetApplicationIcon, sPackageName);
  env->DeleteLocalRef(sPackageName);
  env->DeleteLocalRef(cBitmapDrawable);
  env->DeleteLocalRef(oPackageManager);
  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::GetIcon Error getting icon for  %s. Exception follows:", packageName.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  jobject oBitmap = env->CallObjectMethod(oBitmapDrawable, mgetBitmap);
  env->DeleteLocalRef(oBitmapDrawable);
  jclass cBitmap = env->GetObjectClass(oBitmap);
  jmethodID mcopyPixelsToBuffer = env->GetMethodID(cBitmap, "copyPixelsToBuffer", "(Ljava/nio/Buffer;)V");
  jobject oPixels = env->NewDirectByteBuffer(buffer,bufSize);
  env->DeleteLocalRef(cBitmap);

  // memcpy(buffer,oPixels,bufSize); 
  env->CallVoidMethod(oBitmap, mcopyPixelsToBuffer, oPixels);
  env->DeleteLocalRef(oPixels);
  env->DeleteLocalRef(oBitmap);
  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::GetIcon Error copying icon for  %s. Exception follows:", packageName.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  return true;
}


bool CXBMCApp::HasLaunchIntent(const string &package)
{
  return GetPackageManager().getLaunchIntentForPackage(package) != NULL;
}

// Note intent, dataType, dataURI all default to ""
bool CXBMCApp::StartActivity(const string &package, const string &intent, const string &dataType, const string &dataURI)
{
  CJNIIntent newIntent = GetPackageManager().getLaunchIntentForPackage(package);
  if (!newIntent)
    return false;

  if (!dataURI.empty())
    newIntent.setData(dataURI);

  if (!intent.empty())
    newIntent.setAction(intent);

   startActivity(newIntent);
  return true;
}

int CXBMCApp::GetBatteryLevel()
{
  if (m_activity == NULL)
    return -1;

  JNIEnv* env = xbmc_jnienv();

  jobject oActivity = m_activity->clazz;

  // IntentFilter oIntentFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
  jclass cIntentFilter = env->FindClass("android/content/IntentFilter");
  jmethodID midIntentFilterCtor = env->GetMethodID(cIntentFilter, "<init>", "(Ljava/lang/String;)V");
  jstring sIntentBatteryChanged = env->NewStringUTF("android.intent.action.BATTERY_CHANGED"); // Intent.ACTION_BATTERY_CHANGED
  jobject oIntentFilter = env->NewObject(cIntentFilter, midIntentFilterCtor, sIntentBatteryChanged);
  env->DeleteLocalRef(cIntentFilter);
  env->DeleteLocalRef(sIntentBatteryChanged);

  // Intent oBatteryStatus = activity.registerReceiver(null, oIntentFilter);
  jclass cActivity = env->GetObjectClass(oActivity);
  jmethodID midActivityRegisterReceiver = env->GetMethodID(cActivity, "registerReceiver", "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
  env->DeleteLocalRef(cActivity);
  jobject oBatteryStatus = env->CallObjectMethod(oActivity, midActivityRegisterReceiver, NULL, oIntentFilter);

  jclass cIntent = env->GetObjectClass(oBatteryStatus);
  jmethodID midIntentGetIntExtra = env->GetMethodID(cIntent, "getIntExtra", "(Ljava/lang/String;I)I");
  env->DeleteLocalRef(cIntent);
  
  // int iLevel = oBatteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
  jstring sBatteryManagerExtraLevel = env->NewStringUTF("level"); // BatteryManager.EXTRA_LEVEL
  jint iLevel = env->CallIntMethod(oBatteryStatus, midIntentGetIntExtra, sBatteryManagerExtraLevel, (jint)-1);
  env->DeleteLocalRef(sBatteryManagerExtraLevel);
  // int iScale = oBatteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
  jstring sBatteryManagerExtraScale = env->NewStringUTF("scale"); // BatteryManager.EXTRA_SCALE
  jint iScale = env->CallIntMethod(oBatteryStatus, midIntentGetIntExtra, sBatteryManagerExtraScale, (jint)-1);
  env->DeleteLocalRef(sBatteryManagerExtraScale);
  env->DeleteLocalRef(oBatteryStatus);
  env->DeleteLocalRef(oIntentFilter);

  if (iLevel <= 0 || iScale < 0)
    return iLevel;

  return ((int)iLevel * 100) / (int)iScale;
}

bool CXBMCApp::GetExternalStorage(std::string &path, const std::string &type /* = "" */)
{
  std::string sType;
  std::string mountedState;
  bool mounted = false;
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

  path = CJNIEnvironment::getExternalStoragePublicDirectory(sType).getAbsolutePath();
  mountedState = CJNIEnvironment::getExternalStorageState();
  mounted = (mountedState == "mounted" || mountedState == "mounted_ro");
  return mounted && !path.empty();
}

bool CXBMCApp::GetStorageUsage(const std::string &path, std::string &usage)
{
  if (path.empty())
  {
    std::ostringstream fmt;
    fmt.width(24);  fmt << std::left  << "Filesystem";
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
  fmt.width(24);  fmt << std::left  << path;
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
  android_printf("CXBMCApp::GetMaxSystemVolume: %i",maxVolume);
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

void CXBMCApp::SetSystemVolume(JNIEnv *env, float percent)
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
}

void CXBMCApp::SetupEnv()
{
  setenv("XBMC_ANDROID_SYSTEM_LIBS", CJNISystem::getProperty("java.library.path").c_str(), 0);
  setenv("XBMC_ANDROID_DATA", getApplicationInfo().dataDir.c_str(), 0);
  setenv("XBMC_ANDROID_LIBS", getApplicationInfo().nativeLibraryDir.c_str(), 0);
  setenv("XBMC_ANDROID_APK", getPackageResourcePath().c_str(), 0);

  std::string cacheDir = getCacheDir().getAbsolutePath();
  setenv("XBMC_TEMP", (cacheDir + "/temp").c_str(), 0);
  setenv("XBMC_BIN_HOME", (cacheDir + "/apk/assets").c_str(), 0);
  setenv("XBMC_HOME", (cacheDir + "/apk/assets").c_str(), 0);

  std::string externalDir = getExternalFilesDir("").getAbsolutePath();
  if (!externalDir.size())
    externalDir = getDir("org.xbmc.xbmc", 1).getAbsolutePath();

  if (externalDir.size())
    setenv("HOME", externalDir.c_str(), 0);
  else
    setenv("HOME", getenv("XBMC_TEMP"), 0);
}
