/*
 *      Copyright (C) 2014 Team XBMC
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

#include "Activity.h"
#include "Intent.h"

#include "jutils/jutils-details.hpp"

#include <android/native_activity.h>

using namespace jni;

CJNIActivity::CJNIActivity(const ANativeActivity *nativeActivity) : CJNIContext(nativeActivity)
{
}

CJNIActivity::~CJNIActivity()
{
}

bool CJNIActivity::moveTaskToBack(bool nonRoot)
{
  return call_method<jboolean>(jhobject(m_context),
    "moveTaskToBack", "(Z)Z",
    nonRoot);
}

/////////////////////////////////////////////////
/// \brief CJNIApplicationMainActivity::CJNIApplicationMainActivity
/// \param nativeActivity
///

CJNIApplicationMainActivity* CJNIApplicationMainActivity::m_appInstance(NULL);

CJNIApplicationMainActivity::CJNIApplicationMainActivity(const ANativeActivity *nativeActivity)
  : CJNIActivity(nativeActivity)
{
  m_appInstance = this;
}

CJNIApplicationMainActivity::~CJNIApplicationMainActivity()
{
  m_appInstance = NULL;
}

void CJNIApplicationMainActivity::_onNewIntent(JNIEnv *env, jobject context, jobject intent)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onNewIntent(CJNIIntent(jhobject(intent)));
}

void CJNIApplicationMainActivity::_callNative(JNIEnv *env, jobject context, jlong funcAddr, jlong variantAddr)
{
  (void)env;
  (void)context;
  ((void (*)(CVariant *))funcAddr)((CVariant *)variantAddr);
}

void CJNIApplicationMainActivity::runNativeOnUiThread(void (*callback)(CVariant *), CVariant* variant)
{
  call_method<void>(m_context,
                    "runNativeOnUiThread", "(JJ)V", (jlong)callback, (jlong)variant);
}

void CJNIApplicationMainActivity::_onVolumeChanged(JNIEnv *env, jobject context, jint volume)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->onVolumeChanged(volume);
}

void CJNIApplicationMainActivity::_onAudioFocusChange(JNIEnv *env, jobject context, jint focusChange)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->onAudioFocusChange(focusChange);
}
