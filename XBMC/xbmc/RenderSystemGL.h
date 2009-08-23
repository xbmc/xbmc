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

#include "RenderSystem.h"

class CRenderSystemGL : public CRenderSystemBase
{
public:
  CRenderSystemGL();
  virtual ~CRenderSystemGL();

  virtual bool InitRenderSystem();
  virtual bool DestroyRenderSystem();

  virtual void GetRenderVersion(unsigned int& major, unsigned int& minor);
  virtual CStdString GetRenderVendor();
  virtual CStdString GetRenderRenderer();
  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool PresentRender() = 0;
  virtual bool ClearBuffers(DWORD color);
  virtual bool ClearBuffers(float r, float g, float b, float a);
  virtual bool IsExtSupported(CStdString strExt);

  virtual void SetVSync(bool vsync){};
  virtual bool GetVSync() { return m_bVSync; }

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);

  virtual bool TestRender();

protected:

};

#endif // RENDER_SYSTEM_H
