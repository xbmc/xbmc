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

#ifndef RENDER_SYSTEM_GL_H
#define RENDER_SYSTEM_GL_H

#pragma once

#ifdef HAS_GL

#include "RenderSystem.h"
#include "GLContext.h"

class CRenderSystemGL : public CRenderSystemBase
{
public:
  CRenderSystemGL();
  ~CRenderSystemGL();

  virtual bool Create();
  virtual bool Destroy();
  virtual bool AttachWindow(CWinSystem* winSystem);

  virtual void GetVersion(unsigned int& major, unsigned int& minor);

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool Present();

  virtual bool ClearBuffers(DWORD color);
  virtual bool ClearBuffers(float r, float g, float b, float a);
  virtual bool IsExtSupported(CStdString strExt);

  virtual bool Test();

protected:
  CGLContext m_glContext;

};

extern CRenderSystemGL g_RenderSystem;

#endif

#endif // RENDER_SYSTEM_H
