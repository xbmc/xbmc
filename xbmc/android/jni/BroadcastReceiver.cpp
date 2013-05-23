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

#include "BroadcastReceiver.h"
#include "Intent.h"
#include "Context.h"
#include "ClassLoader.h"
#include "jutils/jutils-details.hpp"

CJNIContext * CJNIBroadcastReceiver::jni_app_context=NULL;

using namespace jni;
CJNIBroadcastReceiver::CJNIBroadcastReceiver(CJNIContext *context) : CJNIBase("org/xbmc/xbmc/XBMCBroadcastReceiver")
{
  jni_app_context = context;
}

void CJNIBroadcastReceiver::InitializeBroadcastReceiver()
{
  // Convert "the/class/name" to "the.class.name" as loadClass() expects it.
  std::string className = GetClassName();
  for (std::string::iterator it = className.begin(); it != className.end(); ++it)
  {
    if (*it == '/')
      *it = '.';
  }
  m_object = new_object(jni_app_context->getClassLoader().loadClass(className));
  m_object.setGlobal();
}

void CJNIBroadcastReceiver::DestroyBroadcastReceiver()
{
  m_object.reset();
}

extern "C"
JNIEXPORT void JNICALL Java_org_xbmc_xbmc_XBMCBroadcastReceiver__1onReceive
  (JNIEnv *env, jobject context, jobject intent)
{
  if(CJNIBroadcastReceiver::jni_app_context)
    CJNIBroadcastReceiver::jni_app_context->onReceive(CJNIIntent(jhobject(intent)));
}
