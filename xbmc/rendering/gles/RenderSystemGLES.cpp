/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "windowing/GraphicContext.h"
#include "settings/AdvancedSettings.h"
#include "RenderSystemGLES.h"
#include "rendering/MatrixGL.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/TimeUtils.h"
#include "utils/SystemInfo.h"
#include "utils/MathUtils.h"
#ifdef TARGET_POSIX
#include "XTimeUtils.h"
#endif

CRenderSystemGLES::CRenderSystemGLES()
 : CRenderSystemBase()
{
  m_pShader.reset(new CGLESShader*[SM_MAX]);
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

  m_bRenderCreated = true;

  InitialiseShaders();

  return true;
}

bool CRenderSystemGLES::ResetRenderSystem(int width, int height)
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
  ResetScissors();
  CDirtyRegionList dirtyRegions;
  CDirtyRegion dirtyWindow(CServiceBroker::GetWinSystem()->GetGfxContext().GetViewWindow());
  dirtyRegions.push_back(dirtyWindow);

  ClearBuffers(0);
  glFinish();
  PresentRenderImpl(true);

  ReleaseShaders();
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGLES::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  bool useLimited = CServiceBroker::GetWinSystem()->UseLimitedColor();

  if (m_limitedColorRange != useLimited)
  {
    ReleaseShaders();
    InitialiseShaders();
  }

  m_limitedColorRange = useLimited;

  return true;
}

bool CRenderSystemGLES::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGLES::ClearBuffers(UTILS::Color color)
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

bool CRenderSystemGLES::IsExtSupported(const char* extension) const
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

void CRenderSystemGLES::SetViewPort(const CRect& viewPort)
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
  if (m_pShader[m_method])
    return m_pShader[m_method]->HardwareClipIsPossible();

  return false;
}

CRect CRenderSystemGLES::ClipRectToScissorRect(const CRect &rect)
{
  if (!m_pShader[m_method])
    return CRect();
  float xFactor = m_pShader[m_method]->GetClipXFactor();
  float xOffset = m_pShader[m_method]->GetClipXOffset();
  float yFactor = m_pShader[m_method]->GetClipYFactor();
  float yOffset = m_pShader[m_method]->GetClipYOffset();
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

void CRenderSystemGLES::InitialiseShaders()
{
  std::string defines;
  m_limitedColorRange = CServiceBroker::GetWinSystem()->UseLimitedColor();
  if (m_limitedColorRange)
  {
    defines += "#define KODI_LIMITED_RANGE 1\n";
  }

  m_pShader[SM_DEFAULT] = new CGLESShader("gles_shader.vert", "gles_shader_default.frag", defines);
  if (!m_pShader[SM_DEFAULT]->CompileAndLink())
  {
    m_pShader[SM_DEFAULT]->Free();
    delete m_pShader[SM_DEFAULT];
    m_pShader[SM_DEFAULT] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_default.frag - compile and link failed");
  }

  m_pShader[SM_TEXTURE] = new CGLESShader("gles_shader_texture.frag", defines);
  if (!m_pShader[SM_TEXTURE]->CompileAndLink())
  {
    m_pShader[SM_TEXTURE]->Free();
    delete m_pShader[SM_TEXTURE];
    m_pShader[SM_TEXTURE] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture.frag - compile and link failed");
  }

  m_pShader[SM_MULTI] = new CGLESShader("gles_shader_multi.frag", defines);
  if (!m_pShader[SM_MULTI]->CompileAndLink())
  {
    m_pShader[SM_MULTI]->Free();
    delete m_pShader[SM_MULTI];
    m_pShader[SM_MULTI] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi.frag - compile and link failed");
  }

  m_pShader[SM_FONTS] = new CGLESShader("gles_shader_fonts.frag", defines);
  if (!m_pShader[SM_FONTS]->CompileAndLink())
  {
    m_pShader[SM_FONTS]->Free();
    delete m_pShader[SM_FONTS];
    m_pShader[SM_FONTS] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_fonts.frag - compile and link failed");
  }

  m_pShader[SM_TEXTURE_NOBLEND] = new CGLESShader("gles_shader_texture_noblend.frag", defines);
  if (!m_pShader[SM_TEXTURE_NOBLEND]->CompileAndLink())
  {
    m_pShader[SM_TEXTURE_NOBLEND]->Free();
    delete m_pShader[SM_TEXTURE_NOBLEND];
    m_pShader[SM_TEXTURE_NOBLEND] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture_noblend.frag - compile and link failed");
  }

  m_pShader[SM_MULTI_BLENDCOLOR] = new CGLESShader("gles_shader_multi_blendcolor.frag", defines);
  if (!m_pShader[SM_MULTI_BLENDCOLOR]->CompileAndLink())
  {
    m_pShader[SM_MULTI_BLENDCOLOR]->Free();
    delete m_pShader[SM_MULTI_BLENDCOLOR];
    m_pShader[SM_MULTI_BLENDCOLOR] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi_blendcolor.frag - compile and link failed");
  }

  m_pShader[SM_TEXTURE_RGBA] = new CGLESShader("gles_shader_rgba.frag", defines);
  if (!m_pShader[SM_TEXTURE_RGBA]->CompileAndLink())
  {
    m_pShader[SM_TEXTURE_RGBA]->Free();
    delete m_pShader[SM_TEXTURE_RGBA];
    m_pShader[SM_TEXTURE_RGBA] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba.frag - compile and link failed");
  }

  m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR] = new CGLESShader("gles_shader_rgba_blendcolor.frag", defines);
  if (!m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR]->CompileAndLink())
  {
    m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR]->Free();
    delete m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR];
    m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_blendcolor.frag - compile and link failed");
  }

  m_pShader[SM_TEXTURE_RGBA_BOB] = new CGLESShader("gles_shader_rgba_bob.frag", defines);
  if (!m_pShader[SM_TEXTURE_RGBA_BOB]->CompileAndLink())
  {
    m_pShader[SM_TEXTURE_RGBA_BOB]->Free();
    delete m_pShader[SM_TEXTURE_RGBA_BOB];
    m_pShader[SM_TEXTURE_RGBA_BOB] = nullptr;
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_bob.frag - compile and link failed");
  }

  if (IsExtSupported("GL_OES_EGL_image_external"))
  {
    m_pShader[SM_TEXTURE_RGBA_OES] = new CGLESShader("gles_shader_rgba_oes.frag", defines);
    if (!m_pShader[SM_TEXTURE_RGBA_OES]->CompileAndLink())
    {
      m_pShader[SM_TEXTURE_RGBA_OES]->Free();
      delete m_pShader[SM_TEXTURE_RGBA_OES];
      m_pShader[SM_TEXTURE_RGBA_OES] = nullptr;
      CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_oes.frag - compile and link failed");
    }


    m_pShader[SM_TEXTURE_RGBA_BOB_OES] = new CGLESShader("gles_shader_rgba_bob_oes.frag", defines);
    if (!m_pShader[SM_TEXTURE_RGBA_BOB_OES]->CompileAndLink())
    {
      m_pShader[SM_TEXTURE_RGBA_BOB_OES]->Free();
      delete m_pShader[SM_TEXTURE_RGBA_BOB_OES];
      m_pShader[SM_TEXTURE_RGBA_BOB_OES] = nullptr;
      CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_bob_oes.frag - compile and link failed");
    }
  }
  else
  {
    m_pShader[SM_TEXTURE_RGBA_OES] = nullptr;
    m_pShader[SM_TEXTURE_RGBA_BOB_OES] = nullptr;
  }
}

void CRenderSystemGLES::ReleaseShaders()
{
  if (m_pShader[SM_DEFAULT])
    m_pShader[SM_DEFAULT]->Free();
  delete m_pShader[SM_DEFAULT];
  m_pShader[SM_DEFAULT] = nullptr;

  if (m_pShader[SM_TEXTURE])
    m_pShader[SM_TEXTURE]->Free();
  delete m_pShader[SM_TEXTURE];
  m_pShader[SM_TEXTURE] = nullptr;

  if (m_pShader[SM_MULTI])
    m_pShader[SM_MULTI]->Free();
  delete m_pShader[SM_MULTI];
  m_pShader[SM_MULTI] = nullptr;

  if (m_pShader[SM_FONTS])
    m_pShader[SM_FONTS]->Free();
  delete m_pShader[SM_FONTS];
  m_pShader[SM_FONTS] = nullptr;

  if (m_pShader[SM_TEXTURE_NOBLEND])
    m_pShader[SM_TEXTURE_NOBLEND]->Free();
  delete m_pShader[SM_TEXTURE_NOBLEND];
  m_pShader[SM_TEXTURE_NOBLEND] = nullptr;

  if (m_pShader[SM_MULTI_BLENDCOLOR])
    m_pShader[SM_MULTI_BLENDCOLOR]->Free();
  delete m_pShader[SM_MULTI_BLENDCOLOR];
  m_pShader[SM_MULTI_BLENDCOLOR] = nullptr;

  if (m_pShader[SM_TEXTURE_RGBA])
    m_pShader[SM_TEXTURE_RGBA]->Free();
  delete m_pShader[SM_TEXTURE_RGBA];
  m_pShader[SM_TEXTURE_RGBA] = nullptr;

  if (m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR])
    m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR]->Free();
  delete m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR];
  m_pShader[SM_TEXTURE_RGBA_BLENDCOLOR] = nullptr;

  if (m_pShader[SM_TEXTURE_RGBA_BOB])
    m_pShader[SM_TEXTURE_RGBA_BOB]->Free();
  delete m_pShader[SM_TEXTURE_RGBA_BOB];
  m_pShader[SM_TEXTURE_RGBA_BOB] = nullptr;

  if (m_pShader[SM_TEXTURE_RGBA_OES])
    m_pShader[SM_TEXTURE_RGBA_OES]->Free();
  delete m_pShader[SM_TEXTURE_RGBA_OES];
  m_pShader[SM_TEXTURE_RGBA_OES] = nullptr;

  if (m_pShader[SM_TEXTURE_RGBA_BOB_OES])
    m_pShader[SM_TEXTURE_RGBA_BOB_OES]->Free();
  delete m_pShader[SM_TEXTURE_RGBA_BOB_OES];
  m_pShader[SM_TEXTURE_RGBA_BOB_OES] = nullptr;
}

void CRenderSystemGLES::EnableGUIShader(ESHADERMETHOD method)
{
  m_method = method;
  if (m_pShader[m_method])
  {
    m_pShader[m_method]->Enable();
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid GUI Shader selected - %d", method);
  }
}

void CRenderSystemGLES::DisableGUIShader()
{
  if (m_pShader[m_method])
  {
    m_pShader[m_method]->Disable();
  }
  m_method = SM_DEFAULT;
}

GLint CRenderSystemGLES::GUIShaderGetPos()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetPosLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCol()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetCord0Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord1()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetCord1Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetUniCol()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetUniColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0Matrix()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetCoord0MatrixLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetField()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetFieldLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetStep()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetStepLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetContrast()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetContrastLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetBrightness()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetBrightnessLoc();

  return -1;
}

bool CRenderSystemGLES::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  return CRenderSystemBase::SupportsStereo(mode);
}

GLint CRenderSystemGLES::GUIShaderGetModel()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetModelLoc();

  return -1;
}
