/*
 *      Copyright (C) 2012 Team XBMC
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

CXBMCApp::CXBMCApp(ANativeActivity *nativeActivity)
  : m_wakeLock(NULL)
{
  m_activity = nativeActivity;
  
  if (m_activity == NULL)
  {
    android_printf("CXBMCApp: invalid ANativeActivity instance");
    exit(1);
    return;
  }

  m_state.appState = Uninitialized;

  if (pthread_mutex_init(&m_state.mutex, NULL) != 0)
  {
    android_printf("CXBMCApp: pthread_mutex_init() failed");
    m_state.appState = Error;
    exit(1);
    return;
  }

}

CXBMCApp::~CXBMCApp()
{
  stop();

  pthread_mutex_destroy(&m_state.mutex);
}

ActivityResult CXBMCApp::onActivate()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);

  switch (m_state.appState)
  {
    case Uninitialized:
      acquireWakeLock();
      
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
      pthread_create(&m_state.thread, &attr, thread_run<CXBMCApp, &CXBMCApp::run>, this);
      pthread_attr_destroy(&attr);
      break;
      
    case Unfocused:
      XBMC_Pause(false);
      setAppState(Rendering);
      break;
      
    case Paused:
      acquireWakeLock();
      
      XBMC_SetupDisplay();
      XBMC_Pause(false);
      setAppState(Rendering);
      break;

    case Initialized:
    case Rendering:
    case Stopping:
    case Stopped:
    case Error:
    default:
      break;
  }
  
  return ActivityOK;
}

void CXBMCApp::onDeactivate()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // this is called on pause, stop and window destroy which
  // require specific (and different) actions
}

void CXBMCApp::onStart()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // wait for onCreateWindow() and onGainFocus()
}

void CXBMCApp::onResume()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // wait for onCreateWindow() and onGainFocus()
}

void CXBMCApp::onPause()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // wait for onDestroyWindow() and/or onLostFocus()
}

void CXBMCApp::onStop()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // everything has been handled in onLostFocus() so wait
  // if onDestroy() is called
}

void CXBMCApp::onDestroy()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  stop();
}

void CXBMCApp::onSaveState(void **data, size_t *size)
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // no need to save anything as XBMC is running in its own thread
}

void CXBMCApp::onConfigurationChanged()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // ignore any configuration changes like screen rotation etc
}

void CXBMCApp::onLowMemory()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // can't do much as we don't want to close completely
}

void CXBMCApp::onCreateWindow(ANativeWindow* window)
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  if (window == NULL)
  {
    android_printf(" => invalid ANativeWindow object");
    return;
  }
  m_window = window;
}

void CXBMCApp::onResizeWindow()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // no need to do anything because we are fixed in fullscreen landscape mode
}

void CXBMCApp::onDestroyWindow()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);

  if (m_state.appState < Paused)
  {
    XBMC_DestroyDisplay();
    setAppState(Paused);
    releaseWakeLock();
  }
}

void CXBMCApp::onGainFocus()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  // everything is handled in onActivate()
}

void CXBMCApp::onLostFocus()
{
  android_printf("%s: %d", __PRETTY_FUNCTION__, m_state.appState);
  switch (m_state.appState)
  {
    case Initialized:
    case Rendering:
      XBMC_Pause(true);
      setAppState(Unfocused);
      break;

    default:
      break;
  }
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

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

  if (!getWakeLock(env))
  {
    android_printf("%s: unable to acquire a WakeLock");
    return;
  }

  jclass cWakeLock = env->GetObjectClass(m_wakeLock);
  jmethodID midWakeLockAcquire = env->GetMethodID(cWakeLock, "acquire", "()V");
  env->CallVoidMethod(m_wakeLock, midWakeLockAcquire);
  env->DeleteLocalRef(cWakeLock);

  DetachCurrentThread();
}

void CXBMCApp::releaseWakeLock()
{
  if (m_activity == NULL)
    return;

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

  if (!getWakeLock(env))
  {
    android_printf("%s: unable to release a WakeLock");
    return;
  }

  jclass cWakeLock = env->GetObjectClass(m_wakeLock);
  jmethodID midWakeLockRelease = env->GetMethodID(cWakeLock, "release", "()V");
  env->CallVoidMethod(m_wakeLock, midWakeLockRelease);
  env->DeleteLocalRef(cWakeLock);

  DetachCurrentThread();
}

void CXBMCApp::run()
{
    int status = 0;
    setAppState(Initialized);

    android_printf(" => running XBMC_Run...");
    try
    {
      setAppState(Rendering);
      status = XBMC_Run(true);
      android_printf(" => XBMC_Run finished with %d", status);
    }
    catch(...)
    {
      android_printf("ERROR: Exception caught on main loop. Exiting");
      setAppState(Error);
    }

  bool finishActivity = false;
  pthread_mutex_lock(&m_state.mutex);
  finishActivity = m_state.appState != Stopping;
  m_state.appState = Stopped;
  pthread_mutex_unlock(&m_state.mutex);
  
  if (finishActivity)
  {
    android_printf(" => calling ANativeActivity_finish()");
    ANativeActivity_finish(m_activity);
  }
}

void CXBMCApp::stop()
{
  android_printf("%s", __PRETTY_FUNCTION__);

  pthread_mutex_lock(&m_state.mutex);
  if (m_state.appState < Stopped)
  {
    m_state.appState = Stopping;
    pthread_mutex_unlock(&m_state.mutex);
    
    android_printf(" => executing XBMC_Stop");
    XBMC_Stop();
    android_printf(" => waiting for XBMC to finish");
    pthread_join(m_state.thread, NULL);
    android_printf(" => XBMC finished");
  }
  else
    pthread_mutex_unlock(&m_state.mutex);
  
  if (m_wakeLock != NULL && m_activity != NULL)
  {
    JNIEnv *env = NULL;
    m_activity->vm->AttachCurrentThread(&env, NULL);
    
    env->DeleteGlobalRef(m_wakeLock);
    m_wakeLock = NULL;
  }
}

void CXBMCApp::setAppState(AppState state)
{
  pthread_mutex_lock(&m_state.mutex);
  m_state.appState = state;
  pthread_mutex_unlock(&m_state.mutex);
}

void CXBMCApp::XBMC_Pause(bool pause)
{
  android_printf("XBMC_Pause(%s)", pause ? "true" : "false");
  // Only send the PAUSE action if we are pausing XBMC and something is currently playing
  if (pause && g_application.IsPlaying() && !g_application.IsPaused())
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PAUSE), WINDOW_INVALID, true);

  g_application.m_AppActive = g_application.m_AppFocused = !pause;
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

int CXBMCApp::AttachCurrentThread(JNIEnv** p_env, void* thr_args /* = NULL */)
{
  // Until a thread is attached, it has no JNIEnv, and cannot make JNI calls.
  // The JNIEnv is used for thread-local storage. For this reason,
  //  you cannot share a JNIEnv between threads.
  // If a thread is attached to JNIEnv and garbage collection is in progress,
  //  or the debugger has issued a suspend request, Android will
  //  pause the thread the next time it makes a JNI call.
  return m_activity->vm->AttachCurrentThread(p_env, thr_args);
}

int CXBMCApp::DetachCurrentThread()
{
  // Threads attached through JNIEnv must
  // call DetachCurrentThread before they exit
  return m_activity->vm->DetachCurrentThread();
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

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);
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
  DetachCurrentThread();
  return true;
}

bool CXBMCApp::GetIconSize(const string &packageName, int *width, int *height)
{
  if (!m_activity)
    return false;

  jthrowable exc;
  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

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
    DetachCurrentThread();
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
    DetachCurrentThread();
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
    DetachCurrentThread();
    return false;
  }

  DetachCurrentThread();
  return true;
}

bool CXBMCApp::GetIcon(const string &packageName, void* buffer, unsigned int bufSize)
{
  if (!m_activity)
    return false;

  jthrowable exc;
  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

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
    DetachCurrentThread();
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
    DetachCurrentThread();
    return false;
  }
  DetachCurrentThread();
  return true;
}


bool CXBMCApp::HasLaunchIntent(const string &package)
{
  if (!m_activity)
    return false;

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

  jthrowable exc;
  jobject oActivity = m_activity->clazz;
  jclass cActivity = env->GetObjectClass(oActivity);

  // oPackageManager = new PackageManager();
  jmethodID mgetPackageManager = env->GetMethodID(cActivity, "getPackageManager", "()Landroid/content/pm/PackageManager;");
  jobject oPackageManager = (jobject)env->CallObjectMethod(oActivity, mgetPackageManager);

  // oPackageIntent = oPackageManager.getLaunchIntentForPackage(package);
  jclass cPackageManager = env->GetObjectClass(oPackageManager);
  jmethodID mgetLaunchIntentForPackage = env->GetMethodID(cPackageManager, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
  jstring sPackageName = env->NewStringUTF(package.c_str());
  jobject oPackageIntent = env->CallObjectMethod(oPackageManager, mgetLaunchIntentForPackage, sPackageName);
  env->DeleteLocalRef(sPackageName);
  env->DeleteLocalRef(cPackageManager);
  env->DeleteLocalRef(oPackageManager);

  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::HasLaunchIntent Error checking for  Launch Intent for %s. Exception follows:", package.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  if (!oPackageIntent)
  {
    return false;
  }

  env->DeleteLocalRef(oPackageIntent);
  return true;
}

bool CXBMCApp::StartActivity(const string &package)
{
  if (!m_activity || !package.size())
    return false;

  jthrowable exc;
  JNIEnv *env = NULL;
  AttachCurrentThread(&env);
  jobject oActivity = m_activity->clazz;
  jclass cActivity = env->GetObjectClass(oActivity);

  // oPackageManager = new PackageManager();
  jmethodID mgetPackageManager = env->GetMethodID(cActivity, "getPackageManager", "()Landroid/content/pm/PackageManager;");
  jobject oPackageManager = (jobject)env->CallObjectMethod(oActivity, mgetPackageManager);

  // oPackageIntent = oPackageManager.getLaunchIntentForPackage(package);
  jclass cPackageManager = env->GetObjectClass(oPackageManager);
  jmethodID mgetLaunchIntentForPackage = env->GetMethodID(cPackageManager, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
  jstring sPackageName = env->NewStringUTF(package.c_str());
  jobject oPackageIntent = env->CallObjectMethod(oPackageManager, mgetLaunchIntentForPackage, sPackageName);
  env->DeleteLocalRef(cPackageManager);
  env->DeleteLocalRef(sPackageName);
  env->DeleteLocalRef(oPackageManager);

  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::StartActivity Failed to load %s. Exception follows:", package.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(cActivity);
    DetachCurrentThread();
    return false;
  }
  if (!oPackageIntent)
  {
    CLog::Log(LOGERROR, "CXBMCApp::StartActivity %s has no Launch Intent", package.c_str());
    env->DeleteLocalRef(cActivity);
    DetachCurrentThread();
    return false;
  }
  // startActivity(oIntent);
  jmethodID mStartActivity = env->GetMethodID(cActivity, "startActivity", "(Landroid/content/Intent;)V");
  env->CallVoidMethod(oActivity, mStartActivity, oPackageIntent);
  env->DeleteLocalRef(cActivity);
  env->DeleteLocalRef(oPackageIntent);

  exc = env->ExceptionOccurred();
  if (exc)
  {
    CLog::Log(LOGERROR, "CXBMCApp::StartActivity Failed to load %s. Exception follows:", package.c_str());
    env->ExceptionDescribe();
    env->ExceptionClear();
    DetachCurrentThread();
    return false;
  }

  DetachCurrentThread();
  return true;
}

int CXBMCApp::GetBatteryLevel()
{
  if (m_activity == NULL)
    return -1;

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);
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

  DetachCurrentThread();

  if (iLevel <= 0 || iScale < 0)
    return iLevel;

  return ((int)iLevel * 100) / (int)iScale;
}

bool CXBMCApp::GetExternalStorage(std::string &path, const std::string &type /* = "" */)
{
  if (m_activity == NULL)
    return false;

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

  // check if external storage is available
  // String sStorageState = android.os.Environment.getExternalStorageState();
  jclass cEnvironment = env->FindClass("android/os/Environment");
  jmethodID midEnvironmentGetExternalStorageState = env->GetStaticMethodID(cEnvironment, "getExternalStorageState", "()Ljava/lang/String;");
  jstring sStorageState = (jstring)env->CallStaticObjectMethod(cEnvironment, midEnvironmentGetExternalStorageState);
  // if (sStorageState != Environment.MEDIA_MOUNTED && sStorageState != Environment.MEDIA_MOUNTED_READ_ONLY) return false;
  const char* storageState = env->GetStringUTFChars(sStorageState, NULL);
  bool mounted = strcmp(storageState, "mounted") == 0 || strcmp(storageState, "mounted_ro") == 0;
  env->ReleaseStringUTFChars(sStorageState, storageState);
  env->DeleteLocalRef(sStorageState);

  if (mounted)
  {
    jobject oExternalStorageDirectory = NULL;
    if (type.empty() || type == "files")
    {
      // File oExternalStorageDirectory = Environment.getExternalStorageDirectory();
      jmethodID midEnvironmentGetExternalStorageDirectory = env->GetStaticMethodID(cEnvironment, "getExternalStorageDirectory", "()Ljava/io/File;");
      oExternalStorageDirectory = env->CallStaticObjectMethod(cEnvironment, midEnvironmentGetExternalStorageDirectory);
    }
    else if (type == "music" || type == "videos" || type == "pictures" || type == "photos" || type == "downloads")
    {
      jstring sType = NULL;
      if (type == "music")
        sType = env->NewStringUTF("Music"); // Environment.DIRECTORY_MUSIC
      else if (type == "videos")
        sType = env->NewStringUTF("Movies"); // Environment.DIRECTORY_MOVIES
      else if (type == "pictures")
        sType = env->NewStringUTF("Pictures"); // Environment.DIRECTORY_PICTURES
      else if (type == "photos")
        sType = env->NewStringUTF("DCIM"); // Environment.DIRECTORY_DCIM
      else if (type == "downloads")
        sType = env->NewStringUTF("Download"); // Environment.DIRECTORY_DOWNLOADS

      // File oExternalStorageDirectory = Environment.getExternalStoragePublicDirectory(sType);
      jmethodID midEnvironmentGetExternalStoragePublicDirectory = env->GetStaticMethodID(cEnvironment, "getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;");
      oExternalStorageDirectory = env->CallStaticObjectMethod(cEnvironment, midEnvironmentGetExternalStoragePublicDirectory, sType);
      env->DeleteLocalRef(sType);
    }

    if (oExternalStorageDirectory != NULL)
    {
      // path = oExternalStorageDirectory.getAbsolutePath();
      jclass cFile = env->GetObjectClass(oExternalStorageDirectory);
      jmethodID midFileGetAbsolutePath = env->GetMethodID(cFile, "getAbsolutePath", "()Ljava/lang/String;");
      env->DeleteLocalRef(cFile);
      jstring sPath = (jstring)env->CallObjectMethod(oExternalStorageDirectory, midFileGetAbsolutePath);
      const char* cPath = env->GetStringUTFChars(sPath, NULL);
      path = cPath;
      env->ReleaseStringUTFChars(sPath, cPath);
      env->DeleteLocalRef(sPath);
      env->DeleteLocalRef(oExternalStorageDirectory);
    }
    else
      mounted = false;
  }

  env->DeleteLocalRef(cEnvironment);

  DetachCurrentThread();

  return mounted && !path.empty();
}

bool CXBMCApp::GetStorageUsage(const std::string &path, std::string &usage)
{
  if (m_activity == NULL)
    return false;

  if (path.empty())
  {
    ostringstream fmt;
    fmt.width(24);  fmt << left  << "Filesystem";
    fmt.width(12);  fmt << right << "Size";
    fmt.width(12);  fmt << "Used";
    fmt.width(12);  fmt << "Avail";
    fmt.width(12);  fmt << "Use %";

    usage = fmt.str();
    return false;
  }

  JNIEnv *env = NULL;
  AttachCurrentThread(&env);

  // android.os.StatFs oStats = new android.os.StatFs(sPath);
  jclass cStatFs = env->FindClass("android/os/StatFs");
  jmethodID midStatFsCtor = env->GetMethodID(cStatFs, "<init>", "(Ljava/lang/String;)V");
  jstring sPath = env->NewStringUTF(path.c_str());
  jobject oStats = env->NewObject(cStatFs, midStatFsCtor, sPath);
  env->DeleteLocalRef(sPath);

  // int iBlockSize = oStats.getBlockSize();
  jmethodID midStatFsGetBlockSize = env->GetMethodID(cStatFs, "getBlockSize", "()I");
  jint iBlockSize = env->CallIntMethod(oStats, midStatFsGetBlockSize);
  
  // int iBlocksTotal = oStats.getBlockCount();
  jmethodID midStatFsGetBlockCount = env->GetMethodID(cStatFs, "getBlockCount", "()I");
  jint iBlocksTotal = env->CallIntMethod(oStats, midStatFsGetBlockCount);
  
  // int iBlocksFree = oStats.getFreeBlocks();
  jmethodID midStatFsGetFreeBlocks = env->GetMethodID(cStatFs, "getFreeBlocks", "()I");
  jint iBlocksFree = env->CallIntMethod(oStats, midStatFsGetFreeBlocks);

  env->DeleteLocalRef(oStats);
  env->DeleteLocalRef(cStatFs);

  DetachCurrentThread();

  if (iBlockSize <= 0 || iBlocksTotal <= 0 || iBlocksFree < 0)
    return false;
  
  float totalSize = (float)iBlockSize * iBlocksTotal / GIGABYTES;
  float freeSize = (float)iBlockSize * iBlocksFree / GIGABYTES;
  float usedSize = totalSize - freeSize;
  float usedPercentage = usedSize / totalSize * 100;

  ostringstream fmt;
  fmt << fixed;
  fmt.precision(1);
  fmt.width(24);  fmt << left  << path;
  fmt.width(12);  fmt << right << totalSize << "G"; // size in GB
  fmt.width(12);  fmt << usedSize << "G"; // used in GB
  fmt.width(12);  fmt << freeSize << "G"; // free
  fmt.precision(0);
  fmt.width(12);  fmt << usedPercentage << "%"; // percentage used
  
  usage = fmt.str();
  return true;
}
