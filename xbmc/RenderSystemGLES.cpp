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
#include "MatrixGLES.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"


CRenderSystemGLES::CRenderSystemGLES() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGLES;
}

CRenderSystemGLES::~CRenderSystemGLES()
{
  DestroyRenderSystem();
  CLog::Log(LOGDEBUG, "GUI Shader - Destroying Shader : %p", m_pGUIshader);
  for (int i = 0; i < 4; i++)
  {
    if (m_pGUIshader[i])
    {
      m_pGUIshader[i]->Free();
      delete m_pGUIshader[i];
      m_pGUIshader[i] = NULL;
    }
  }
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
  m_renderCaps = 0;
  // Get the GLES version number
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
  {
    sscanf(ver, "%d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
    if (!m_RenderVersionMajor)
      sscanf(ver, "%*s %*s %d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
    m_RenderVersion = ver;
  }
  
  // Get our driver vendor and renderer
  m_RenderVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);

  m_RenderExtensions  = " ";
  m_RenderExtensions += (const char*) glGetString(GL_EXTENSIONS);
  m_RenderExtensions += " ";

  LogGraphicsInfo();
  
  if (IsExtSupported("GL_TEXTURE_NPOT"))
  {
    m_renderCaps |= RENDER_CAPS_NPOT;
  }

  m_bRenderCreated = true;
  
  InitialiseGUIShader();

  return true;
}

bool CRenderSystemGLES::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  m_width = width;
  m_height = height;
  
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  
  CalculateMaxTexturesize();

  CRect rect( 0, 0, width, height );
  SetViewPort( rect );

  glEnable(GL_TEXTURE_2D); 
  glEnable(GL_SCISSOR_TEST); 

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.LoadIdentity();

  g_matrices.Ortho(0.0f, width-1, height-1, 0.0f, -1.0f, 1.0f);

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.LoadIdentity();
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);  
    
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

  float r = GET_R(color) / 255.0f;
  float g = GET_G(color) / 255.0f;
  float b = GET_B(color) / 255.0f;
  float a = GET_A(color) / 255.0f;

  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;
  glClear(flags);

  return true;
}

bool CRenderSystemGLES::IsExtSupported(const char* extension)
{
  if (extension == "GL_EXT_framebuffer_object")
  {
    // GLES has FBO as a core element, not an extension!
    return true;
  }
  else if (extension == "GL_TEXTURE_NPOT")
  {
    // GLES supports non-power-of-two textures as standard.
	return true;
	/* Note: The wrap mode can only be GL_CLAMP_TO_EDGE and the minification filter can only be
	 * GL_NEAREST or GL_LINEAR (in other words, not mipmapped). The extension GL_OES_texture_npot
	 * relaxes these restrictions and allows wrap modes of GL_REPEAT and GL_MIRRORED_REPEAT and
	 * also	allows npot textures to be mipmapped with the full set of minification filters
	 */
  }
  else
  {
    CStdString name;
    name  = " ";
    name += extension;
    name += " ";

    bool supported = m_RenderExtensions.find(name) != std::string::npos;
    CLog::Log(LOGDEBUG, "GLES: Extension Support Test - %s %s", extension, supported ? "YES" : "NO");
    return supported;
  }
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
    curr = CurrentHostCounter();
    freq = CurrentHostFrequency();

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
    curr = CurrentHostCounter();

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
    CLog::Log(LOGINFO, "GLES: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GLES: Disabling VSYNC");

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
      freq = CurrentHostFrequency();
      m_iSwapRate   = (int64_t)((double)freq / rate);
      m_iSwapTime   = (int64_t)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
      m_iSwapStamp  = 0;
      CLog::Log(LOGINFO, "GLES: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }
    
  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GLES: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGINFO, "GLES: Selected vsync mode %d", m_iVSyncMode);
}

void CRenderSystemGLES::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.PushMatrix();
  g_matrices.MatrixMode(MM_TEXTURE);
  g_matrices.PushMatrix();
  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PushMatrix();
  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
//TODO - NOTE: Only for Screensavers & Visualisations
//  glColor3f(1.0, 1.0, 1.0);
}

void CRenderSystemGLES::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.PopMatrix();
  g_matrices.MatrixMode(MM_TEXTURE);
  g_matrices.PopMatrix();
  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PopMatrix();
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);  
}

void CRenderSystemGLES::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight)
{ 
  if (!m_bRenderCreated)
    return;
  
  g_graphicsContext.BeginPaint();
  
  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);
  
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  float w = (float)viewport[2]*0.5f;
  float h = (float)viewport[3]*0.5f;

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.LoadIdentity();
  g_matrices.Translatef(-(viewport[0] + w + offset.x), +(viewport[1] + h + offset.y), 0);
  g_matrices.LookAt(0.0, 0.0, -2.0*h, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.LoadIdentity();
  g_matrices.Frustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  g_matrices.MatrixMode(MM_MODELVIEW);

  g_graphicsContext.EndPaint();
}

bool CRenderSystemGLES::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution];
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  g_matrices.PushMatrix();
  g_matrices.Rotatef( theta, 0.0f, 0.0f, 1.0f );

  EnableGUIShader(SM_DEFAULT);

  GLfloat col[3][4];
  GLfloat ver[3][2];
  GLint   posLoc = GUIShaderGetPos();
  GLint   colLoc = GUIShaderGetCol();

  glVertexAttribPointer(posLoc,  2, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc,  4, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);

  // Setup Colour values
  col[0][0] = col[0][3] = col[1][1] = col[1][3] = col[2][2] = col[2][3] = 1.0f;
  col[0][1] = col[0][2] = col[1][0] = col[1][2] = col[2][0] = col[2][1] = 0.0f;

  // Setup vertex position values
  ver[0][0] =  0.0f;
  ver[0][1] =  1.0f;
  ver[1][0] =  0.87f;
  ver[1][1] = -0.5f;
  ver[2][0] = -0.87f;
  ver[2][1] = -0.5f;

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);

  DisableGUIShader();

  g_matrices.PopMatrix();

  theta += 1.0f;

  return true;
}

void CRenderSystemGLES::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{ 
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PushMatrix();
  GLfloat matrix[4][4];

  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      matrix[j][i] = finalMatrix.m[i][j];

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;

  g_matrices.MultMatrixf(&matrix[0][0]);
}

void CRenderSystemGLES::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.PopMatrix();
}

void CRenderSystemGLES::CalculateMaxTexturesize()
{
  // GLES cannot do PROXY textures to determine maximum size,
  CLog::Log(LOGINFO, "GLES: Maximum texture width: %u", m_maxTextureSize);
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

// FIXME make me const so that I can accept temporary objects
void CRenderSystemGLES::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

void CRenderSystemGLES::InitialiseGUIShader()
{
  if (!m_pGUIshader[0])
  {
    for (int i = 0;i < 4; i++)
    {
      char shaderName[512];
      sprintf( shaderName, "guishader_frag%d.glsl", i );
      //sprintf( shaderName, "guishader_frag.glsl", i );

      m_pGUIshader[i] = new CGUIShader( shaderName );
      if (!m_pGUIshader[i]->CompileAndLink())
      {
        m_pGUIshader[i]->Free();
        delete m_pGUIshader[i];
        m_pGUIshader[i] = NULL;
        CLog::Log(LOGERROR, "GUI Shader - Initialise failed");
      }

      else
      {
        CLog::Log(LOGDEBUG, "GUI Shader - Initialise successful : %p", m_pGUIshader[i]);
      }
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GUI Shader - Tried to Initialise again. Was this intentional?");
  }
}

void CRenderSystemGLES::EnableGUIShader(ESHADERMETHOD method)
{
  m_method = (int)method;
  if (m_pGUIshader[m_method])
  {
    m_pGUIshader[m_method]->Setup(method);
    m_pGUIshader[m_method]->Enable();
  }
  else {
    printf("invalid gui shader %d\n", method);
  }
}

void CRenderSystemGLES::DisableGUIShader()
{
  if (m_pGUIshader[m_method]) {
    m_pGUIshader[m_method]->Disable();
  }
  m_method = 0;
}

GLint CRenderSystemGLES::GUIShaderGetPos()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetPosLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCol()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetCord0Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord1()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetCord1Loc();

  return -1;
}

#endif
