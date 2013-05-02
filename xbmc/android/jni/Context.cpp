/*
 *      Copyright (C) 2013 Team XBMC
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
#include "Context.h"
#include "PackageManager.h"
#include <android/log.h>
#include "Intent.h"
#include "IntentFilter.h"
#include "ClassLoader.h"
#include "jutils/jutils-details.hpp"
#include "BroadcastReceiver.h"
#include "JNIThreading.h"
#include "ApplicationInfo.h"
#include "File.h"
#include <android/native_activity.h>

using namespace jni;

jhobject CJNIContext::m_context(0);
CJNIContext::CJNIContext(const ANativeActivity *nativeActivity) : CJNIBroadcastReceiver(this)
{
  m_context.reset(nativeActivity->clazz);
  xbmc_jni_on_load(nativeActivity->vm, nativeActivity->env);
  InitializeBroadcastReceiver();
}

CJNIContext::~CJNIContext()
{
  m_context.release();
  DestroyBroadcastReceiver();
  xbmc_jni_on_unload();
}

CJNIPackageManager CJNIContext::GetPackageManager()
{
  return (CJNIPackageManager)call_method<jhobject>(m_context, "getPackageManager", "()Landroid/content/pm/PackageManager;");
}

void CJNIContext::startActivity(const CJNIIntent &intent)
{
  call_method<void>(jhobject(m_context), "startActivity", "(Landroid/content/Intent;)V", intent.get());
}

int CJNIContext::checkCallingOrSelfPermission(const std::string &permission)
{
  return call_method<int>(m_context, "checkCallingOrSelfPermission", "(Ljava/lang/String;)I", jcast<jhstring>(permission));
}

jhobject CJNIContext::getSystemService(const std::string &service)
{
  return call_method<jhobject>(m_context, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", jcast<jhstring>(service));
}

CJNIIntent CJNIContext::registerReceiver(const CJNIBroadcastReceiver &receiver, const CJNIIntentFilter &filter)
{
  return (CJNIIntent)call_method<jhobject>(m_context, "registerReceiver", \
                             "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;", receiver.get(), filter.get());
}

CJNIIntent CJNIContext::registerReceiver(const CJNIIntentFilter &filter)
{
  return (CJNIIntent)call_method<jhobject>(m_context, "registerReceiver", \
                             "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;", (jobject)NULL, filter.get());
}

CJNIIntent CJNIContext::sendBroadcast(const CJNIIntent &intent)
{
  return (CJNIIntent)call_method<jhobject>(m_context, "sendBroadcast", "(Landroid/content/Intent;)V", intent.get());
}

CJNIIntent CJNIContext::getIntent()
{
  return (CJNIIntent)call_method<jhobject>(m_context, "getIntent", "()Landroid/content/Intent;");
}

CJNIClassLoader CJNIContext::getClassLoader()
{
  return (CJNIClassLoader)call_method<jhobject>(m_context, "getClassLoader", "()Ljava/lang/ClassLoader;");
}

CJNIApplicationInfo CJNIContext::getApplicationInfo()
{
  return (CJNIApplicationInfo)call_method<jhobject>(m_context, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
}

std::string CJNIContext::getPackageResourcePath()
{
  return jcast<std::string>(call_method<jhstring>(m_context, "getPackageResourcePath", "()Ljava/lang/String;"));
}

CJNIFile CJNIContext::getCacheDir()
{
  return (CJNIFile)call_method<jhobject>(m_context, "getCacheDir", "()Ljava/io/File;");
}

CJNIFile CJNIContext::getDir(const std::string &path, int mode)
{
  return (CJNIFile)call_method<jhobject>(m_context, "getDir", "(Ljava/lang/String;I)Ljava/io/File;", jcast<jhstring>(path), mode);
}

CJNIFile CJNIContext::getExternalFilesDir(const std::string &path)
{
  return (CJNIFile)call_method<jhobject>(m_context, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;", jcast<jhstring>(path));
}
