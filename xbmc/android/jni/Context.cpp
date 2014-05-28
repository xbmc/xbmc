/*
 *      Copyright (C) 2013 Team XBMC
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

#ifndef JNI_CONTEXT_H_INCLUDED
#define JNI_CONTEXT_H_INCLUDED
#include "Context.h"
#endif

#ifndef JNI_PACKAGEMANAGER_H_INCLUDED
#define JNI_PACKAGEMANAGER_H_INCLUDED
#include "PackageManager.h"
#endif

#include <android/log.h>
#ifndef JNI_INTENT_H_INCLUDED
#define JNI_INTENT_H_INCLUDED
#include "Intent.h"
#endif

#ifndef JNI_INTENTFILTER_H_INCLUDED
#define JNI_INTENTFILTER_H_INCLUDED
#include "IntentFilter.h"
#endif

#ifndef JNI_CLASSLOADER_H_INCLUDED
#define JNI_CLASSLOADER_H_INCLUDED
#include "ClassLoader.h"
#endif

#include "jutils/jutils-details.hpp"
#ifndef JNI_BROADCASTRECEIVER_H_INCLUDED
#define JNI_BROADCASTRECEIVER_H_INCLUDED
#include "BroadcastReceiver.h"
#endif

#ifndef JNI_JNITHREADING_H_INCLUDED
#define JNI_JNITHREADING_H_INCLUDED
#include "JNIThreading.h"
#endif

#ifndef JNI_APPLICATIONINFO_H_INCLUDED
#define JNI_APPLICATIONINFO_H_INCLUDED
#include "ApplicationInfo.h"
#endif

#ifndef JNI_FILE_H_INCLUDED
#define JNI_FILE_H_INCLUDED
#include "File.h"
#endif

#ifndef JNI_CONTENTRESOLVER_H_INCLUDED
#define JNI_CONTENTRESOLVER_H_INCLUDED
#include "ContentResolver.h"
#endif

#ifndef JNI_BASECOLUMNS_H_INCLUDED
#define JNI_BASECOLUMNS_H_INCLUDED
#include "BaseColumns.h"
#endif

#ifndef JNI_MEDIASTORE_H_INCLUDED
#define JNI_MEDIASTORE_H_INCLUDED
#include "MediaStore.h"
#endif

#ifndef JNI_POWERMANAGER_H_INCLUDED
#define JNI_POWERMANAGER_H_INCLUDED
#include "PowerManager.h"
#endif

#ifndef JNI_CURSOR_H_INCLUDED
#define JNI_CURSOR_H_INCLUDED
#include "Cursor.h"
#endif

#ifndef JNI_CONNECTIVITYMANAGER_H_INCLUDED
#define JNI_CONNECTIVITYMANAGER_H_INCLUDED
#include "ConnectivityManager.h"
#endif

#ifndef JNI_AUDIOFORMAT_H_INCLUDED
#define JNI_AUDIOFORMAT_H_INCLUDED
#include "AudioFormat.h"
#endif

#ifndef JNI_AUDIOMANAGER_H_INCLUDED
#define JNI_AUDIOMANAGER_H_INCLUDED
#include "AudioManager.h"
#endif

#ifndef JNI_AUDIOTRACK_H_INCLUDED
#define JNI_AUDIOTRACK_H_INCLUDED
#include "AudioTrack.h"
#endif

#ifndef JNI_SURFACE_H_INCLUDED
#define JNI_SURFACE_H_INCLUDED
#include "Surface.h"
#endif

#ifndef JNI_MEDIACODEC_H_INCLUDED
#define JNI_MEDIACODEC_H_INCLUDED
#include "MediaCodec.h"
#endif

#ifndef JNI_MEDIACODECINFO_H_INCLUDED
#define JNI_MEDIACODECINFO_H_INCLUDED
#include "MediaCodecInfo.h"
#endif

#ifndef JNI_MEDIAFORMAT_H_INCLUDED
#define JNI_MEDIAFORMAT_H_INCLUDED
#include "MediaFormat.h"
#endif

#ifndef JNI_WINDOW_H_INCLUDED
#define JNI_WINDOW_H_INCLUDED
#include "Window.h"
#endif

#ifndef JNI_VIEW_H_INCLUDED
#define JNI_VIEW_H_INCLUDED
#include "View.h"
#endif

#ifndef JNI_BUILD_H_INCLUDED
#define JNI_BUILD_H_INCLUDED
#include "Build.h"
#endif


#include <android/native_activity.h>

using namespace jni;

jhobject CJNIContext::m_context(0);
CJNIContext* CJNIContext::m_appInstance(NULL);

CJNIContext::CJNIContext(const ANativeActivity *nativeActivity)
{
  m_context.reset(nativeActivity->clazz);
  xbmc_jni_on_load(nativeActivity->vm, nativeActivity->env);
  CJNIBase::SetSDKVersion(nativeActivity->sdkVersion);
  PopulateStaticFields();
  m_appInstance = this;
}

CJNIContext::~CJNIContext()
{
  m_appInstance = NULL;
  xbmc_jni_on_unload();
}

void CJNIContext::PopulateStaticFields()
{
  CJNIBaseColumns::PopulateStaticFields();
  CJNIMediaStoreMediaColumns::PopulateStaticFields();
  CJNIPowerManager::PopulateStaticFields();
  CJNIPackageManager::PopulateStaticFields();
  CJNIMediaStoreMediaColumns::PopulateStaticFields();
  CJNICursor::PopulateStaticFields();
  CJNIContentResolver::PopulateStaticFields();
  CJNIConnectivityManager::PopulateStaticFields();
  CJNIAudioFormat::PopulateStaticFields();
  CJNIAudioManager::PopulateStaticFields();
  CJNIAudioTrack::PopulateStaticFields();
  CJNISurface::PopulateStaticFields();
  CJNIMediaCodec::PopulateStaticFields();
  CJNIMediaCodecInfoCodecProfileLevel::PopulateStaticFields();
  CJNIMediaCodecInfoCodecCapabilities::PopulateStaticFields();
  CJNIMediaFormat::PopulateStaticFields();
  CJNIView::PopulateStaticFields();
  CJNIBuild::PopulateStaticFields();
}

CJNIPackageManager CJNIContext::GetPackageManager()
{
  return call_method<jhobject>(m_context,
    "getPackageManager", "()Landroid/content/pm/PackageManager;");
}

void CJNIContext::startActivity(const CJNIIntent &intent)
{
  call_method<void>(jhobject(m_context),
    "startActivity", "(Landroid/content/Intent;)V",
    intent.get_raw());
}

int CJNIContext::checkCallingOrSelfPermission(const std::string &permission)
{
  return call_method<int>(m_context,
    "checkCallingOrSelfPermission", "(Ljava/lang/String;)I",
    jcast<jhstring>(permission));
}

jhobject CJNIContext::getSystemService(const std::string &service)
{
  return call_method<jhobject>(m_context,
    "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
    jcast<jhstring>(service));
}

CJNIIntent CJNIContext::registerReceiver(const CJNIBroadcastReceiver &receiver, const CJNIIntentFilter &filter)
{
  return call_method<jhobject>(m_context,
    "registerReceiver", "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;",
    receiver.get_raw(), filter.get_raw());
}

CJNIIntent CJNIContext::registerReceiver(const CJNIIntentFilter &filter)
{
  return call_method<jhobject>(m_context,
    "registerReceiver", "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;",
    (jobject)NULL, filter.get_raw());
}

void CJNIContext::unregisterReceiver(const CJNIBroadcastReceiver &receiver)
{
  call_method<void>(m_context,
    "unregisterReceiver", "(Landroid/content/BroadcastReceiver;)V",
    receiver.get_raw());
}

CJNIIntent CJNIContext::sendBroadcast(const CJNIIntent &intent)
{
  return call_method<jhobject>(m_context,
    "sendBroadcast", "(Landroid/content/Intent;)V",
    intent.get_raw());
}

CJNIIntent CJNIContext::getIntent()
{
  return call_method<jhobject>(m_context,
    "getIntent", "()Landroid/content/Intent;");
}

CJNIClassLoader CJNIContext::getClassLoader()
{
  return call_method<jhobject>(m_context,
    "getClassLoader", "()Ljava/lang/ClassLoader;");
}

CJNIApplicationInfo CJNIContext::getApplicationInfo()
{
  return call_method<jhobject>(m_context,
    "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
}

std::string CJNIContext::getPackageResourcePath()
{
  return jcast<std::string>(call_method<jhstring>(m_context,
    "getPackageResourcePath", "()Ljava/lang/String;"));
}

CJNIFile CJNIContext::getCacheDir()
{
  return call_method<jhobject>(m_context,
    "getCacheDir", "()Ljava/io/File;");
}

CJNIFile CJNIContext::getDir(const std::string &path, int mode)
{
  return call_method<jhobject>(m_context,
    "getDir", "(Ljava/lang/String;I)Ljava/io/File;",
    jcast<jhstring>(path), mode);
}

CJNIFile CJNIContext::getExternalFilesDir(const std::string &path)
{
  return call_method<jhobject>(m_context,
    "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;",
    jcast<jhstring>(path));
}

CJNIContentResolver CJNIContext::getContentResolver()
{
  return call_method<jhobject>(m_context,
    "getContentResolver", "()Landroid/content/ContentResolver;");
}

CJNIWindow CJNIContext::getWindow()
{
  return call_method<jhobject>(m_context,
    "getWindow", "()Landroid/view/Window;");
}

void CJNIContext::_onNewIntent(JNIEnv *env, jobject context, jobject intent)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->onNewIntent(CJNIIntent(jhobject(intent)));
}
