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

#ifndef GL_CONTEXT_H
#define GL_CONTEXT_H

#pragma once

#include "WinSystem.h"


class CGLContextBase
{
public:
  virtual bool Create(CWinSystem* pWinSystem ){ return false; }
  virtual bool Release() { return false; }
  virtual bool MakeCurrent() { return false; }
  virtual bool GetPixelFormats() { return false; }
  virtual bool SwapBuffers() { return false; }
  virtual bool IsCreated() { return false; }

protected:
  bool m_bCreated;
  CWinSystem* m_pWinSystem;

  bool m_HasPixelFormatARB;
  bool m_HasMultisample;
  bool m_HasHardwareGamma;
};

#ifdef HAS_WGL
#include "GLContextWGL.h"
#define CGLContext CGLContextWGL 
#endif

#endif // GL_CONTEXT_H
