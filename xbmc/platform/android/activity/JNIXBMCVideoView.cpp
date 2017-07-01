/*
 *      Copyright (C) 2016 Christian Browet
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

#include "JNIXBMCVideoView.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <list>
#include <algorithm>
#include <cassert>

#include "CompileInfo.h"

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

CJNIXBMCVideoView::CJNIXBMCVideoView()
  : m_callback(nullptr)
  , m_surfaceCreated(nullptr)
{
}

CJNIXBMCVideoView::CJNIXBMCVideoView(const jni::jhobject &object)
  : CJNIBase(object)
  , m_callback(nullptr)
  , m_surfaceCreated(nullptr)
{
}

CJNIXBMCVideoView::~CJNIXBMCVideoView()
{
  delete m_surfaceCreated;
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
  pvw->m_surfaceCreated = new CEvent;
  if (pvw->isCreated())
    pvw->m_surfaceCreated->Set();
  pvw->add();

  return pvw;
}

void CJNIXBMCVideoView::_surfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->surfaceChanged(CJNISurfaceHolder(jhobject(holder)), format, width, height);
}

void CJNIXBMCVideoView::_surfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->surfaceCreated(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCVideoView::_surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->surfaceDestroyed(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCVideoView::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCVideoView::surfaceCreated(CJNISurfaceHolder holder)
{
  if (m_surfaceCreated)
    m_surfaceCreated->Set();
  if (m_callback)
    m_callback->surfaceCreated(holder);
}

void CJNIXBMCVideoView::surfaceDestroyed(CJNISurfaceHolder holder)
{
  if (m_surfaceCreated)
    m_surfaceCreated->Reset();
  if (m_callback)
    m_callback->surfaceDestroyed(holder);
}

bool CJNIXBMCVideoView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated->WaitMSec(millis);
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

