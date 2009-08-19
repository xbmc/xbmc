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

#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#pragma once

#include "WinSystem.h"
#include "Geometry.h"
#include "StdString.h"


typedef enum _RenderingSystemType
{
  RENDERING_SYSTEM_OPENGL,
  RENDERING_SYSTEM_DIRECTX
} RenderingSystemType;


class CRenderSystemBase
{
public:
  CRenderSystemBase();
  virtual ~CRenderSystemBase();

  RenderingSystemType GetRenderingSystemType() { return m_enumRenderingSystem; }
  bool GetVSync() { return m_bVSync; }

  virtual bool Create(){ return false; }
  virtual bool Destroy() { return false; }
  virtual void GetVersion(unsigned int& major, unsigned int& minor) { major = 0; minor = 0; }
  virtual bool AttachWindow(CWinSystem* winSystem) { return false; }
  virtual bool BeginRender() { return false; }
  virtual bool EndRender() { return false; }
  virtual bool Present() { return false; }
  virtual bool ClearBuffers(DWORD color) { return false; }
  virtual bool ClearBuffers(float r, float g, float b, float a) { return false; }
  virtual bool IsExtSupported(CStdString strExt) { return false; }

  virtual bool Test() { return false; }

  virtual void SetViewPort(CRect& viewPort) {}
  virtual void GetViewPort(CRect& viewPort) {}

protected:
  CWinSystem* m_pWinSystem;
  bool m_bCreated;
  RenderingSystemType m_enumRenderingSystem;
  bool m_bVSync;
};

#ifdef HAS_DX
#include "RenderSystemDX.h"
#define CRenderSystem CRenderSystemDX 
#elif defined(HAS_GL)
#include "RenderSystemGL.h"
#define CRenderSystem CRenderSystemGL 
#endif

#endif // RENDER_SYSTEM_H
