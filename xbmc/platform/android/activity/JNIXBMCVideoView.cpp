/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCVideoView.h"

#include "CompileInfo.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cassert>
#include <list>

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCVideoView";

void CJNIXBMCVideoView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_surfaceChanged", "(Landroid/view/SurfaceHolder;III)V", (void*)&CJNIXBMCVideoView::_surfaceChanged},
      {"_surfaceCreated", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCVideoView::_surfaceCreated},
      {"_surfaceDestroyed", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCVideoView::_surfaceDestroyed}
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

CJNIXBMCVideoView::CJNIXBMCVideoView(const jni::jhobject &object)
  : CJNIBase(object)
{
}

CJNIXBMCVideoView* CJNIXBMCVideoView::createVideoView(CJNISurfaceHolderCallback* callback)
{
  std::string signature = "()L" + s_className + ";";

  CJNIXBMCVideoView* pvw = new CJNIXBMCVideoView(call_static_method<jhobject>(xbmc_jnienv(), CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)),
                                                                              "createVideoView", signature.c_str()));
  if (!*pvw)
  {
    CLog::Log(LOGERROR, "Cannot instantiate VideoView!!");
    delete pvw;
    return nullptr;
  }

  add_instance(pvw->get_raw(), pvw);
  pvw->m_callback = callback;
  if (pvw->isCreated())
    pvw->m_surfaceCreated.Set();
  pvw->add();

  return pvw;
}

void CJNIXBMCVideoView::_surfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(thiz);
  if (inst)
    inst->surfaceChanged(CJNISurfaceHolder(jhobject::fromJNI(holder)), format, width, height);
}

void CJNIXBMCVideoView::_surfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(thiz);
  if (inst)
    inst->surfaceCreated(CJNISurfaceHolder(jhobject::fromJNI(holder)));
}

void CJNIXBMCVideoView::_surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(thiz);
  if (inst)
    inst->surfaceDestroyed(CJNISurfaceHolder(jhobject::fromJNI(holder)));
}

void CJNIXBMCVideoView::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCVideoView::surfaceCreated(CJNISurfaceHolder holder)
{
  if (m_callback)
    m_callback->surfaceCreated(holder);
  m_surfaceCreated.Set();
}

void CJNIXBMCVideoView::surfaceDestroyed(CJNISurfaceHolder holder)
{
  m_surfaceCreated.Reset();
  if (m_callback)
    m_callback->surfaceDestroyed(holder);
}

bool CJNIXBMCVideoView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated.Wait(std::chrono::milliseconds(millis));
}

void CJNIXBMCVideoView::add()
{
  call_method<void>(m_object,
                    "add", "()V");
}

void CJNIXBMCVideoView::release()
{
  remove_instance(this);
  call_method<void>(m_object,
                    "release", "()V");
}

CJNISurface CJNIXBMCVideoView::getSurface()
{
  return call_method<jhobject>(m_object,
                               "getSurface", "()Landroid/view/Surface;");
}

const CRect& CJNIXBMCVideoView::getSurfaceRect()
{
  return m_surfaceRect;
}

void CJNIXBMCVideoView::setSurfaceRect(const CRect& rect)
{
  call_method<void>(m_object,
                    "setSurfaceRect", "(IIII)V", int(rect.x1), int(rect.y1), int(rect.x2), int(rect.y2));
  m_surfaceRect = rect;
}

bool CJNIXBMCVideoView::isCreated() const
{
  return get_field<jboolean>(m_object, "mIsCreated");
}

