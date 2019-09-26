/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCMainView.h"

#include "CompileInfo.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cassert>
#include <list>

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCMainView";
CEvent CJNIXBMCMainView::m_surfaceCreated;
CJNIXBMCMainView* CJNIXBMCMainView::m_instance = nullptr;

void CJNIXBMCMainView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_attach", "()V", (void*)&CJNIXBMCMainView::_attach},
      {"_surfaceChanged", "(Landroid/view/SurfaceHolder;III)V", (void*)&CJNIXBMCMainView::_surfaceChanged},
      {"_surfaceCreated", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCMainView::_surfaceCreated},
      {"_surfaceDestroyed", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCMainView::_surfaceDestroyed}
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

CJNIXBMCMainView::CJNIXBMCMainView(CJNISurfaceHolderCallback* callback)
  : m_callback(callback)
{
  m_instance = this;
}

void CJNIXBMCMainView::_attach(JNIEnv* env, jobject thiz)
{
  (void)env;

  if (m_instance)
    m_instance->attach(thiz);
}

void CJNIXBMCMainView::_surfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  if (m_instance)
    m_instance->surfaceChanged(CJNISurfaceHolder(jhobject(holder)), format, width, height);
}

void CJNIXBMCMainView::_surfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  if (m_instance)
    m_instance->surfaceCreated(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCMainView::_surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  if (m_instance)
    m_instance->surfaceDestroyed(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCMainView::attach(const jobject& thiz)
{
  if (!m_object)
  {
    m_object = jhobject(thiz);
    m_object.setGlobal();
  }
}

void CJNIXBMCMainView::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCMainView::surfaceCreated(CJNISurfaceHolder holder)
{
  if (m_callback)
    m_callback->surfaceCreated(holder);
  m_surfaceCreated.Set();
}

void CJNIXBMCMainView::surfaceDestroyed(CJNISurfaceHolder holder)
{
  m_surfaceCreated.Reset();
  if (m_callback)
    m_callback->surfaceDestroyed(holder);
}

bool CJNIXBMCMainView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated.WaitMSec(millis);
}

bool CJNIXBMCMainView::isCreated() const
{
  if (!m_object)
    return false;
  return get_field<jboolean>(m_object, "mIsCreated");

}

