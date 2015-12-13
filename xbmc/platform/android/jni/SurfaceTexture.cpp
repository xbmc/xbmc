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

#include "JNIBase.h"
#include "Context.h"
#include "Activity.h"
#include "ClassLoader.h"
#include "SurfaceTexture.h"

#include "jutils/jutils-details.hpp"

#include "platform/android/activity/JNIMainActivity.h"
#include <algorithm>

using namespace jni;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
CJNISurfaceTextureOnFrameAvailableListener* CJNISurfaceTextureOnFrameAvailableListener::m_listenerInstance(NULL);

CJNISurfaceTextureOnFrameAvailableListener::CJNISurfaceTextureOnFrameAvailableListener()
: CJNIBase(CJNIContext::getPackageName() + ".XBMCOnFrameAvailableListener")
{
  CJNIMainActivity *appInstance = CJNIMainActivity::GetAppInstance();
  if (!appInstance)
    return;

  // Convert "the/class/name" to "the.class.name" as loadClass() expects it.
  std::string dotClassName = GetClassName();
  std::replace(dotClassName.begin(), dotClassName.end(), '/', '.');
  m_object = new_object(appInstance->getClassLoader().loadClass(dotClassName));
  m_object.setGlobal();

  m_listenerInstance = this;
}

void CJNISurfaceTextureOnFrameAvailableListener::_onFrameAvailable(JNIEnv *env, jobject context, jobject surface)
{
  (void)env;
  (void)context;
  if (m_listenerInstance)
  {
    CJNISurfaceTexture jni_surface = jhobject(surface);
    m_listenerInstance->OnFrameAvailable(jni_surface);
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
CJNISurfaceTexture::CJNISurfaceTexture(int texName) : CJNIBase("android/graphics/SurfaceTexture")
{
  m_object = new_object(GetClassName(), "<init>", "(I)V", texName);
  m_object.setGlobal();
}

void CJNISurfaceTexture::setOnFrameAvailableListener(const CJNISurfaceTextureOnFrameAvailableListener &listener)
{
  call_method<void>(m_object,
    "setOnFrameAvailableListener",
    "(Landroid/graphics/SurfaceTexture$OnFrameAvailableListener;)V", listener.get_raw());
}

void CJNISurfaceTexture::setDefaultBufferSize(int width, int height)
{
  call_method<void>(m_object,
    "setDefaultBufferSize", "(II)V", width, height);
}

void CJNISurfaceTexture::updateTexImage()
{
  call_method<void>(m_object,
    "updateTexImage", "()V");
}

void CJNISurfaceTexture::detachFromGLContext()
{
  call_method<void>(m_object,
    "detachFromGLContext", "()V");
}

void CJNISurfaceTexture::attachToGLContext(int texName)
{
  call_method<void>(m_object,
    "attachToGLContext", "(I)V", texName);
}

void CJNISurfaceTexture::getTransformMatrix(float* mtx)
{
  jsize size = 16; // hard-coded 4x4 matrix.
  JNIEnv *env = xbmc_jnienv();
  jfloatArray floatarray = env->NewFloatArray(size);
  call_method<void>(m_object,
    "getTransformMatrix", "([F)V", floatarray);
  env->GetFloatArrayRegion(floatarray, 0, size, mtx);

  env->DeleteLocalRef(floatarray);
}

int64_t CJNISurfaceTexture::getTimestamp()
{
  return call_method<jlong>(m_object,
    "getTimestamp", "()J");
}

void CJNISurfaceTexture::release()
{
  call_method<void>(m_object,
    "release", "()V");
}
