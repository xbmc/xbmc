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
#include <androidjni/ClassLoader.h>

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <list>
#include <algorithm>
#include <cassert>

using namespace jni;

// Map java object instances to C++ instances
std::list<std::pair<jhobject, CJNIXBMCVideoView*>> s_videoview_map;

CJNIXBMCVideoView* find_videoview(const jhobject& o)
{
  for( auto it = s_videoview_map.begin(); it != s_videoview_map.end(); ++it )
  {
    if (it->first == o)
      return it->second;
  }
  return nullptr;
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
  std::string dotClassName = CJNIContext::getPackageName() + ".XBMCVideoView";
  std::string slashClassName = dotClassName;
  std::replace(slashClassName.begin(), slashClassName.end(), '.', '/');
  std::string signature = "()L" + slashClassName + ";";

  CJNIXBMCVideoView* pvw = new CJNIXBMCVideoView(call_static_method<jhobject>(xbmc_jnienv(), CJNIContext::getClassLoader().loadClass(dotClassName),
                                                                              "createVideoView", signature.c_str()));
  if (!*pvw)
  {
    CLog::Log(LOGERROR, "Cannot instantiate VideoView!!");
    delete pvw;
    return nullptr;
  }

  s_videoview_map.push_back(std::pair<jhobject, CJNIXBMCVideoView*>(pvw->get_raw(), pvw));
  pvw->m_callback = callback;
  pvw->m_surfaceCreated = new CEvent;
  if (pvw->isCreated())
    pvw->m_surfaceCreated->Set();
  pvw->add();

  return pvw;
}

void CJNIXBMCVideoView::_OnSurfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  CJNIXBMCVideoView *inst = find_videoview(jhobject(thiz));
  if (inst)
    inst->OnSurfaceChanged(CJNISurfaceHolder(jhobject(holder)), format, width, height);
}

void CJNIXBMCVideoView::_OnSurfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_videoview(jhobject(thiz));
  if (inst)
    inst->OnSurfaceCreated(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCVideoView::_OnSurfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_videoview(jhobject(thiz));
  if (inst)
    inst->OnSurfaceDestroyed(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCVideoView::OnSurfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCVideoView::OnSurfaceCreated(CJNISurfaceHolder holder)
{
  if (m_surfaceCreated)
    m_surfaceCreated->Set();
  if (m_callback)
    m_callback->surfaceCreated(holder);
}

void CJNIXBMCVideoView::OnSurfaceDestroyed(CJNISurfaceHolder holder)
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
  for( auto it = s_videoview_map.begin(); it != s_videoview_map.end(); ++it )
  {
    if (it->second == this)
    {
      s_videoview_map.erase(it);
      break;
    }
  }

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

