#pragma once
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

class CJNISurfaceTexture;

class CJNISurfaceTextureOnFrameAvailableListener : public CJNIBase
{
public:
  CJNISurfaceTextureOnFrameAvailableListener(const jni::jhobject &object) : CJNIBase(object) {};
  virtual ~CJNISurfaceTextureOnFrameAvailableListener() {};

  static void _onFrameAvailable(JNIEnv *env, jobject context, jobject surface);

protected:
  CJNISurfaceTextureOnFrameAvailableListener();

  virtual void OnFrameAvailable(CJNISurfaceTexture &surface)=0;

private:
  static CJNISurfaceTextureOnFrameAvailableListener *m_listenerInstance;
};

class CJNISurfaceTexture : public CJNIBase
{
public:
  CJNISurfaceTexture(const jni::jhobject &object) : CJNIBase(object) {};
  CJNISurfaceTexture(int texName);
  ~CJNISurfaceTexture() {};

  void    setOnFrameAvailableListener(const CJNISurfaceTextureOnFrameAvailableListener &listener);
  void    setDefaultBufferSize(int width, int height);
  void    updateTexImage();
  void    detachFromGLContext();
  void    attachToGLContext(int texName);
  void    getTransformMatrix(float* mtx); // mtx MUST BE a preallocated 4x4 float array
  int64_t getTimestamp();
  void    release();
};
