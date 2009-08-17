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

#include "stdafx.h"
#include <sstream>
#include "GLContextWGL.h"

#ifdef HAS_WGL

CGLContextWGL::CGLContextWGL()
{
  m_bCreated = false;
  m_pWinSystem = NULL;
  m_hglrc = NULL;
  m_hdc = NULL;

  m_HasPixelFormatARB = false;
  m_HasMultisample = false;
  m_HasHardwareGamma = false;
}

CGLContextWGL::~CGLContextWGL()
{
  Release();
}

bool CGLContextWGL::Create(CWinSystem* pWinSystem)
{
  if(!pWinSystem)
    return false;

  m_pWinSystem = pWinSystem;
  HWND hwnd = m_pWinSystem->GetHwnd();

  // no chance of failure and no need to release thanks to CS_OWNDC
  m_hdc = GetDC(hwnd); 

  PIXELFORMATDESCRIPTOR pfd;
  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 16;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;

  // if these fail, wglCreateContext will also quietly fail
  int format;
  if ((format = ChoosePixelFormat(m_hdc, &pfd)) != 0)
    SetPixelFormat(m_hdc, format, &pfd);
  else
    return false;

  m_hglrc = wglCreateContext(m_hdc);
  if (!m_hglrc)
    return false;

  // find out support for some frequent extensions
  PFNWGLGETEXTENSIONSSTRINGARBPROC _wglGetExtensionsStringARB =
    (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");

  // check for pixel format and multisampling support
  if (_wglGetExtensionsStringARB)
  {
    std::istringstream wglexts(_wglGetExtensionsStringARB(m_hdc));
    std::string ext;
    while (wglexts >> ext)
    {
      if (ext == "WGL_ARB_pixel_format")
        m_HasPixelFormatARB = true;
      else if (ext == "WGL_ARB_multisample")
        m_HasMultisample = true;
      else if (ext == "WGL_EXT_framebuffer_sRGB")
        m_HasHardwareGamma = true;
    }
  }

  wglMakeCurrent(m_hdc, m_hglrc);

  return true;
}


bool CGLContextWGL::Release()
{
  wglMakeCurrent(NULL, NULL);
  if(m_hglrc)
    wglDeleteContext(m_hglrc);

  m_hglrc = NULL;
  m_hdc = NULL;

  return true;
}

bool CGLContextWGL::SwapBuffers()
{
  if(m_hdc != NULL)
  {
    ::SwapBuffers(m_hdc);
    return true;
  }
  else
    return false;
}


bool CGLContextWGL::MakeCurrent()
{
  return true;
}

bool CGLContextWGL::GetPixelFormats()
{
  return true;
}

bool CGLContextWGL::IsCreated()
{
  return true;
}

#endif // HAS_WGL

