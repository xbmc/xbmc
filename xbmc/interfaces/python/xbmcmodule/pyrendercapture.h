/*
 *      Copyright (C) 2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <Python.h>
#include "cores/VideoRenderers/RenderCapture.h"

//PyByteArray_FromStringAndSize is only available in python 2.6 and up
#if PY_VERSION_HEX >= 0x02060000 

#define HAS_PYRENDERCAPTURE

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD
    CRenderCapture* capture;
  } RenderCapture;

  extern PyTypeObject RenderCapture_Type;
  void initRenderCapture_Type();
}

#ifdef __cplusplus
}
#endif

#endif //PY_VERSION_HEX >= 0x02060000

