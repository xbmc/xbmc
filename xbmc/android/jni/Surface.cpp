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

#include "Surface.h"
#include "SurfaceTexture.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNISurface::CJNISurface(CJNISurfaceTexture *surf_texture) : CJNIBase("android/view/Surface")
{
  m_object = new_object(GetClassName(),
    "<init>", "(Landroid/graphics/SurfaceTexture;)V", 
    surf_texture->get_raw());
  m_object.setGlobal();
}

CJNISurface::~CJNISurface()
{
  release();
}

void CJNISurface::release()
{
  call_method<jhobject>(m_object,
    "release", "()V");
}
