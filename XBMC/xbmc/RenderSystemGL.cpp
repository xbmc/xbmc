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

#ifdef HAS_GL

#include "RenderSystemGL.h"

#if defined(_WIN32)
  #pragma comment (lib,"opengl32.lib")
  #pragma comment (lib,"glu32.lib")
  #pragma comment (lib,"../../xbmc/lib/libglew/glew32.lib") 
#endif

CRenderSystemGL::CRenderSystemGL() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGL;
}

CRenderSystemGL::~CRenderSystemGL()
{
  DestroyRenderSystem();
}

bool CRenderSystemGL::InitRenderSystem()
{
  m_bRenderCreated = true;

  // init glew library
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    // Problem: glewInit failed, something is seriously wrong
    return false;
  }
 
  // Get the GL version number 
  m_RenderVerdenVersionMajor = 0;
  m_RenderVerdenVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
    sscanf(ver, "%d.%d", &m_RenderVerdenVersionMajor, &m_RenderVerdenVersionMinor);
  
  // Check if we need DPOT  
  m_NeedPower2Texture = true;
  if (m_RenderVerdenVersionMajor >= 2 && GLEW_ARB_texture_non_power_of_two)
    m_NeedPower2Texture = false;
  
  // Get our driver vendor and renderer
  m_RenderVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);
  
  LogGraphicsInfo();
  
#if defined(_WIN32PC)
  CWIN32Util::CheckGLVersion();
#endif
  
  return true;
}

bool CRenderSystemGL::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return true;
}

void CRenderSystemGL::GetRenderVersion(unsigned int& major, unsigned int& minor)
{
  major = m_RenderVerdenVersionMajor;
  minor = m_RenderVerdenVersionMinor;
}

void CRenderSystemGL::SetViewPort(CRect& viewPort)
{
  if(!m_bRenderCreated)
    return;
}

void CRenderSystemGL::GetViewPort(CRect& viewPort)
{
  if(!m_bRenderCreated)
    return;
}

bool CRenderSystemGL::BeginRender()
{
  if(!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::EndRender()
{
  if(!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::ClearBuffers(DWORD color)
{
  if(!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::ClearBuffers(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;
  glClear(flags);

  return true;
}

bool CRenderSystemGL::IsExtSupported(CStdString strExt)
{
  return true;
}

bool CRenderSystemGL::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution];
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  glPushMatrix();
  glRotatef( theta, 0.0f, 0.0f, 1.0f );
  glBegin( GL_TRIANGLES );
  glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
  glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
  glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
  glEnd();
  glPopMatrix();

  theta += 1.0f;

  return true;
}

#endif
