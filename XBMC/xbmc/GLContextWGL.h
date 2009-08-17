/*
*      Copyright (C) 2005-2008 Team XBMC
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

#ifndef GL_CONTEXTWGL_H
#define GL_CONTEXTWGL_H

#pragma once

#include "GLContext.h"

class CGLContextWGL : public CGLContextBase
{
public:
  CGLContextWGL();
  virtual ~CGLContextWGL();
  virtual bool Create(CWinSystem* pWinSystem );
  virtual bool Release();
  virtual bool SwapBuffers();
  virtual bool MakeCurrent();
  virtual bool GetPixelFormats();

  virtual bool IsCreated();

protected:
  HDC m_hdc;
  HGLRC m_hglrc;
};

#endif // GL_CONTEXTWGL_H


