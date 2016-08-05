/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "utils/MathUtils.h"
#ifdef TARGET_POSIX
#include "XTimeUtils.h"
#endif

static const char* ShaderNames[SM_ESHADERCOUNT] =
    {"guishader_frag_default.glsl",
     "guishader_frag_texture.glsl",
     "guishader_frag_multi.glsl",
     "guishader_frag_fonts.glsl",
     "guishader_frag_texture_noblend.glsl",
     "guishader_frag_multi_blendcolor.glsl",
     "guishader_frag_rgba.glsl",
     "guishader_frag_rgba_oes.glsl",
     "guishader_frag_rgba_blendcolor.glsl",
     "guishader_frag_rgba_bob.glsl",
     "guishader_frag_rgba_bob_oes.glsl"
    };

CRenderSystemGLES::CRenderSystemGLES()
 : CRenderSystemBase()
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
  const char *tmpVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderVendor.clear();
  if (tmpVendor != NULL)
    m_RenderVendor = tmpVendor;

  const char *tmpRenderer = (const char*) glGetString(GL_RENDERER);
  m_RenderRenderer.clear();
  if (tmpRenderer != NULL)
    m_RenderRenderer = tmpRenderer;

  m_RenderExtensions  = " ";

  const char *tmpExtensions = (const char*) glGetString(GL_EXTENSIONS);
  if (tmpExtensions != NULL)
  {
    m_RenderExtensions += tmpExtensions;
  }

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

  glMatrixProject.Clear();
  glMatrixProject->LoadIdentity();
  glMatrixProject->Ortho(0.0f, width-1, height-1, 0.0f, -1.0f, 1.0f);
  glMatrixProject.Load();

  glMatrixModview.Clear();
  glMatrixModview->LoadIdentity();
  glMatrixModview.Load();

  glMatrixTexture.Clear();
  glMatrixTexture->LoadIdentity();
  glMatrixTexture.Load();

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

  ResetScissors();
  CDirtyRegionList dirtyRegions;
  CDirtyRegion dirtyWindow(g_graphicsContext.GetViewWindow());
  dirtyRegions.push_back(dirtyWindow);

  ClearBuffers(0);
  glFinish();
  PresentRenderImpl(true);

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
    std::string name;
    name  = " ";
    name += extension;
    name += " ";

    bool supported = m_RenderExtensions.find(name) != std::string::npos;
    CLog::Log(LOGDEBUG, "GLES: Extension Support Test - %s %s", extension, supported ? "YES" : "NO");
    return supported;
  }
}

void CRenderSystemGLES::PresentRender(bool rendered, bool videoLayer)
{
  SetVSync(true);

  if (!m_bRenderCreated)
    return;

  PresentRenderImpl(rendered);

  // if video is rendered to a separate layer, we should not block this thread
  if (!rendered && !videoLayer)
    Sleep(40);
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
  m_bVSync       = enable;
  m_bVsyncInit   = true;

  SetVSyncImpl(enable);
  
  if (!enable)
    return;

  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GLES: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGINFO, "GLES: Selected vsync mode %d", m_iVSyncMode);
}

void CRenderSystemGLES::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;

  glMatrixProject.Push();
  glMatrixModview.Push();
  glMatrixTexture.Push();

  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  glActiveTexture(GL_TEXTURE0);
//! @todo - NOTE: Only for Screensavers & Visualisations
//  glColor3f(1.0, 1.0, 1.0);
}

void CRenderSystemGLES::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  glMatrixProject.PopLoad();
  glMatrixModview.PopLoad();
  glMatrixTexture.PopLoad();
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);  
  glClear(GL_DEPTH_BUFFER_BIT);
}

void CRenderSystemGLES::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor)
{ 
  if (!m_bRenderCreated)
    return;
  
  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);
  
  float w = (float)m_viewPort[2]*0.5f;
  float h = (float)m_viewPort[3]*0.5f;

  glMatrixModview->LoadIdentity();
  glMatrixModview->Translatef(-(w + offset.x - stereoFactor), +(h + offset.y), 0);
  glMatrixModview->LookAt(0.0, 0.0, -2.0*h, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
  glMatrixModview.Load();

  glMatrixProject->LoadIdentity();
  glMatrixProject->Frustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  glMatrixProject.Load();
}

void CRenderSystemGLES::Project(float &x, float &y, float &z)
{
  GLfloat coordX, coordY, coordZ;
  if (CMatrixGL::Project(x, y, z, glMatrixModview.Get(), glMatrixProject.Get(), m_viewPort, &coordX, &coordY, &coordZ))
  {
    x = coordX;
    y = (float)(m_viewPort[1] + m_viewPort[3] - coordY);
    z = 0;
  }
}

bool CRenderSystemGLES::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = CDisplaySettings::GetInstance().GetCurrentResolutionInfo();
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  glMatrixModview.Push();
  glMatrixModview->Rotatef( theta, 0.0f, 0.0f, 1.0f );

  EnableGUIShader(SM_DEFAULT);

  GLfloat col[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  GLfloat ver[3][2];
  GLint   posLoc = GUIShaderGetPos();
  GLint   colLoc = GUIShaderGetCol();

  glVertexAttribPointer(posLoc,  2, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc,  4, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);

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

  glMatrixModview.Pop();

  theta += 1.0f;

  return true;
}

void CRenderSystemGLES::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{ 
  if (!m_bRenderCreated)
    return;

  glMatrixModview.Push();
  GLfloat matrix[4][4];

  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      matrix[j][i] = finalMatrix.m[i][j];

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;

  glMatrixModview->MultMatrixf(&matrix[0][0]);
  glMatrixModview.Load();
}

void CRenderSystemGLES::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;

  glMatrixModview.PopLoad();
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

  viewPort.x1 = m_viewPort[0];
  viewPort.y1 = m_height - m_viewPort[1] - m_viewPort[3];
  viewPort.x2 = m_viewPort[0] + m_viewPort[2];
  viewPort.y2 = viewPort.y1 + m_viewPort[3];
}

// FIXME make me const so that I can accept temporary objects
void CRenderSystemGLES::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  m_viewPort[0] = viewPort.x1;
  m_viewPort[1] = m_height - viewPort.y1 - viewPort.Height();
  m_viewPort[2] = viewPort.Width();
  m_viewPort[3] = viewPort.Height();
}

bool CRenderSystemGLES::ScissorsCanEffectClipping()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->HardwareClipIsPossible();

  return false;
}

CRect CRenderSystemGLES::ClipRectToScissorRect(const CRect &rect)
{
  if (!m_pGUIshader[m_method])
    return CRect();
  float xFactor = m_pGUIshader[m_method]->GetClipXFactor();
  float xOffset = m_pGUIshader[m_method]->GetClipXOffset();
  float yFactor = m_pGUIshader[m_method]->GetClipYFactor();
  float yOffset = m_pGUIshader[m_method]->GetClipYOffset();
  return CRect(rect.x1 * xFactor + xOffset,
               rect.y1 * yFactor + yOffset,
               rect.x2 * xFactor + xOffset,
               rect.y2 * yFactor + yOffset);
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
      if (i == SM_TEXTURE_RGBA_OES || i == SM_TEXTURE_RGBA_BOB_OES)
      {
        if (!g_Windowing.IsExtSupported("GL_OES_EGL_image_external"))
        {
          m_pGUIshader[i] = NULL;
          continue;
        }
      }

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

GLint CRenderSystemGLES::GUIShaderGetUniCol()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetUniColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0Matrix()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetCoord0MatrixLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetField()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetFieldLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetStep()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetStepLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetContrast()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetContrastLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetBrightness()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetBrightnessLoc();

  return -1;
}

bool CRenderSystemGLES::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch(mode)
  {
    case RENDER_STEREO_MODE_INTERLACED:
      if (g_sysinfo.HasHW3DInterlaced())
        return true;

    default:
      return CRenderSystemBase::SupportsStereo(mode);
  }
}

GLint CRenderSystemGLES::GUIShaderGetModel()
{
  if (m_pGUIshader[m_method])
    return m_pGUIshader[m_method]->GetModelLoc();

  return -1;
}

#endif
