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


#include "system.h"

#if HAS_GLES == 2

#include "GraphicContext.h"
#include "AdvancedSettings.h"
#include "RenderSystemGLES.h"
#include "utils/log.h"

CRenderSystemGLES::CRenderSystemGLES() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGLES;
  //TODO: GLES Rendering System
}

CRenderSystemGLES::~CRenderSystemGLES()
{
  DestroyRenderSystem();
  //TODO: GLES Rendering System
}

bool CRenderSystemGLES::InitRenderSystem()
{
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
  m_maxTextureSize = 2048;

  // Get the GL version number 
  m_RenderVerdenVersionMajor = 0;
  m_RenderVerdenVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
  {
    sscanf(ver, "%d.%d", &m_RenderVerdenVersionMajor, &m_RenderVerdenVersionMinor);
    if (!m_RenderVerdenVersionMajor)
      sscanf(ver, "%*s %*s %d.%d", &m_RenderVerdenVersionMajor, &m_RenderVerdenVersionMinor);
  }
  
  // Check if we need DPOT  
  m_NeedPower2Texture = true;
  if (GL_OES_texture_npot)
    m_NeedPower2Texture = false;
  
  // Get our driver vendor and renderer
  m_RenderVendor   = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);
  
  LogGraphicsInfo();
  
  m_bRenderCreated = true;
  
  return true;
}

bool CRenderSystemGLES::ResetRenderSystem(int width, int height)
{
  //TODO: GLES Rendering System
    
  return true;
}

bool CRenderSystemGLES::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGLES::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGLES::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGLES::ClearBuffers(color_t color)
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGLES::ClearBuffers(float r, float g, float b, float a)
{
  if (!m_bRenderCreated)
    return false;
  
  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;
  glClear(flags);

  return true;
}

bool CRenderSystemGLES::IsExtSupported(const char* extension)
{
  return false;
}

static int64_t abs(int64_t a)
{
  if(a < 0)
    return -a;
  return a;
}

bool CRenderSystemGLES::PresentRender()
{
  if (!m_bRenderCreated)
    return false;

  if (m_iVSyncMode != 0 && m_iSwapRate != 0) 
  {
    int64_t curr, diff, freq;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr);
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

    if(m_iSwapStamp == 0)
      m_iSwapStamp = curr;

    /* calculate our next swap timestamp */
    diff = curr - m_iSwapStamp;
    diff = m_iSwapRate - diff % m_iSwapRate;
    m_iSwapStamp = curr + diff;

    /* sleep as close as we can before, assume 1ms precision of sleep *
     * this should always awake so that we are guaranteed the given   *
     * m_iSwapTime to do our swap                                     */
    diff = (diff - m_iSwapTime) * 1000 / freq;
    if (diff > 0)
      Sleep((DWORD)diff);
  }
  
  bool result = PresentRenderImpl();
  
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    int64_t curr, diff;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr);

    diff = curr - m_iSwapStamp;
    m_iSwapStamp = curr;

    if (abs(diff - m_iSwapRate) < abs(diff))
      CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
  }
  
  return result;
}

void CRenderSystemGLES::SetVSync(bool enable)
{
  if (m_bVSync==enable && m_bVsyncInit == true)
    return;

  if (!m_bRenderCreated)
    return;
  
  if (enable)
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");

  m_iVSyncMode   = 0;
  m_iVSyncErrors = 0;
  m_iSwapRate    = 0;
  m_bVSync       = enable;
  m_bVsyncInit   = true;

  SetVSyncImpl(enable);
  
  if (!enable)
    return;

  if (g_advancedSettings.m_ForcedSwapTime != 0.0)
  {
    /* some hardware busy wait on swap/glfinish, so we must manually sleep to avoid 100% cpu */
    double rate = g_graphicsContext.GetFPS();
    if (rate <= 0.0 || rate > 1000.0)
    {
      CLog::Log(LOGWARNING, "Unable to determine a valid horizontal refresh rate, vsync workaround disabled %.2g", rate);
      m_iSwapRate = 0;
    }
    else
    {
      int64_t freq;
      QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
      m_iSwapRate   = (int64_t)((double)freq / rate);
      m_iSwapTime   = (int64_t)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
      m_iSwapStamp  = 0;
      CLog::Log(LOGINFO, "GL: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }
    
  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGINFO, "GL: Selected vsync mode %d", m_iVSyncMode);  
}

void CRenderSystemGLES::CaptureStateBlock()
{
  //TODO: GLES Rendering System
}

void CRenderSystemGLES::ApplyStateBlock()
{
  //TODO: GLES Rendering System 
}

void CRenderSystemGLES::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight)
{ 
  //TODO: GLES Rendering System
}

bool CRenderSystemGLES::TestRender()
{
  //TODO: GLES Rendering System

  return true;
}

void CRenderSystemGLES::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{ 
  //TODO: GLES Rendering System
}

void CRenderSystemGLES::RestoreHardwareTransform()
{
  //TODO: GLES Rendering System
}

void CRenderSystemGLES::CalculateMaxTexturesize()
{
  // GLES cannot do PROXY textures to determine maximum size,
  // so set it to default maximum size instead.
  m_maxTextureSize = 2048;
  CLog::Log(LOGINFO, "GL: Maximum texture width: %u", m_maxTextureSize);
}

void CRenderSystemGLES::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;
  
  GLint glvp[4];
  glGetIntegerv(GL_SCISSOR_BOX, glvp);
  
  viewPort.x1 = glvp[0];
  viewPort.y1 = m_height - glvp[1] - glvp[3];
  viewPort.x2 = glvp[0] + glvp[2];
  viewPort.y2 = viewPort.y1 + glvp[3];
}

void CRenderSystemGLES::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;
  
  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

#endif
