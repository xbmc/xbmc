/*
*      Copyright (C) 2005-2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/


#include "system.h"

#if HAS_GLES == 2

#include "guilib/GraphicContext.h"
#include "settings/AdvancedSettings.h"
#include "RenderSystemGLES.h"
#include "guilib/MatrixGLES.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "utils/MathUtils.h"

static const char* ShaderNames[SM_ESHADERCOUNT] =
    {"guishader_frag_default.glsl",
     "guishader_frag_texture.glsl",
     "guishader_frag_multi.glsl",
     "guishader_frag_fonts.glsl",
     "guishader_frag_texture_noblend.glsl",
     "guishader_frag_multi_blendcolor.glsl",
     "guishader_frag_rgba.glsl",
     "guishader_frag_rgba_blendcolor.glsl"
    };

CRenderSystemGLES::CRenderSystemGLES()
 : CRenderSystemBase()
 , m_pGUIshader(0)
 , m_method(SM_DEFAULT)
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGLES;
}

CRenderSystemGLES::~CRenderSystemGLES()
{
}

bool CRenderSystemGLES::InitRenderSystem()
{
  GLint maxTextureSize;

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

  m_maxTextureSize = maxTextureSize;
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
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

  if (IsExtSupported("GL_EXT_texture_format_BGRA8888"))
  {
    m_renderCaps |= RENDER_CAPS_BGRA;
  }

  if (IsExtSupported("GL_IMG_texture_format_BGRA8888"))
  {
    m_renderCaps |= RENDER_CAPS_BGRA;
  }

  if (IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
  {
    m_renderCaps |= RENDER_CAPS_BGRA_APPLE;
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

  glEnable(GL_SCISSOR_TEST); 

  g_matrices.MatrixMode(MM_PROJECTION);
  g_matrices.LoadIdentity();

#ifdef TARGET_RASPBERRY_PI
  g_matrices.Ortho(0.0f, width-1, height-1, 0.0f, +1.0f, 1.0f);
#else
  g_matrices.Ortho(0.0f, width-1, height-1, 0.0f, -1.0f, 1.0f);
#endif

  g_matrices.MatrixMode(MM_MODELVIEW);
  g_matrices.LoadIdentity();
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);  
    
  return true;
}

bool CRenderSystemGLES::DestroyRenderSystem()
{
  CLog::Log(LOGDEBUG, "GUI Shader - Destroying Shader : %p", m_pGUIshader);

  if (m_pGUIshader)
  {
    for (int i = 0; i < SM_ESHADERCOUNT; i++)
    {
      if (m_pGUIshader[i])
      {
        m_pGUIshader[i]->Free();
        delete m_pGUIshader[i];
        m_pGUIshader[i] = NULL;
      }
    }
    delete[] m_pGUIshader;
    m_pGUIshader = NULL;
  }

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
  if (strcmp( extension, "GL_EXT_framebuffer_object" ) == 0)
  {
    // GLES has FBO as a core element, not an extension!
    return true;
  }
  else if (strcmp( extension, "GL_TEXTURE_NPOT" ) == 0)
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

static int64_t abs64(int64_t a)
{
  if(a < 0)
    return -a;
  return a;
}

bool CRenderSystemGLES::PresentRender(const CDirtyRegionList &dirty)
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
  
  bool result = PresentRenderImpl(dirty);
  
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    int64_t curr, diff;
    curr = CurrentHostCounter();

    diff = curr - m_iSwapStamp;
    m_iSwapStamp = curr;

    if (abs64(diff - m_iSwapRate) < abs64(diff))
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
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);  
  glClear(GL_DEPTH_BUFFER_BIT);
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

  glGetIntegerv(GL_VIEWPORT, m_viewPort);
  GLfloat* matx;
  matx = g_matrices.GetMatrix(MM_MODELVIEW);
  memcpy(m_view, matx, 16 * sizeof(GLfloat));
  matx = g_matrices.GetMatrix(MM_PROJECTION);
  memcpy(m_projection, matx, 16 * sizeof(GLfloat));

  g_graphicsContext.EndPaint();
}

void CRenderSystemGLES::Project(float &x, float &y, float &z)
{
  GLfloat coordX, coordY, coordZ;
  if (g_matrices.Project(x, y, z, m_view, m_projection, m_viewPort, &coordX, &coordY, &coordZ))
  {
    x = coordX;
    y = (float)(m_viewPort[3] - coordY);
    z = 0;
  }
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
  glGetIntegerv(GL_VIEWPORT, glvp);
  
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

void CRenderSystemGLES::SetScissors(const CRect &rect)
{
  if (!m_bRenderCreated)
    return;
  GLint x1 = MathUtils::round_int(rect.x1);
  GLint y1 = MathUtils::round_int(rect.y1);
  GLint x2 = MathUtils::round_int(rect.x2);
  GLint y2 = MathUtils::round_int(rect.y2);
  glScissor(x1, m_height - y2, x2-x1, y2-y1);
}

void CRenderSystemGLES::ResetScissors()
{
  SetScissors(CRect(0, 0, (float)m_width, (float)m_height));
}

void CRenderSystemGLES::InitialiseGUIShader()
{
  if (!m_pGUIshader)
  {
    m_pGUIshader = new CGUIShader*[SM_ESHADERCOUNT];
    for (int i = 0; i < SM_ESHADERCOUNT; i++)
    {
      m_pGUIshader[i] = new CGUIShader( ShaderNames[i] );

      if (!m_pGUIshader[i]->CompileAndLink())
      {
        m_pGUIshader[i]->Free();
        delete m_pGUIshader[i];
        m_pGUIshader[i] = NULL;
        CLog::Log(LOGERROR, "GUI Shader [%s] - Initialise failed", ShaderNames[i]);
      }
      else
      {
        CLog::Log(LOGDEBUG, "GUI Shader [%s]- Initialise successful : %p", ShaderNames[i], m_pGUIshader[i]);
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
  m_method = method;
  if (m_pGUIshader[m_method])
  {
    m_pGUIshader[m_method]->Enable();
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid GUI Shader selected - [%s]", ShaderNames[(int)method]);
  }
}

void CRenderSystemGLES::DisableGUIShader()
{
  if (m_pGUIshader[m_method])
  {
    m_pGUIshader[m_method]->Disable();
  }
  m_method = SM_DEFAULT;
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
