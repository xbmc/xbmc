/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystemGL.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/GUITextureGL.h"
#include "rendering/MatrixGL.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#if defined(TARGET_LINUX)
#include "utils/EGLUtils.h"
#endif

using namespace std::chrono_literals;

CRenderSystemGL::CRenderSystemGL() : CRenderSystemBase()
{
}

CRenderSystemGL::~CRenderSystemGL() = default;

bool CRenderSystemGL::InitRenderSystem()
{
  m_bVSync = false;
  m_bVsyncInit = false;
  m_maxTextureSize = 2048;

  // Get the GL version number
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
  {
    sscanf(ver, "%d.%d", &m_RenderVersionMajor, &m_RenderVersionMinor);
    m_RenderVersion = ver;
  }

  CLog::Log(LOGINFO, "CRenderSystemGL::{} - Version: {}, Major: {}, Minor: {}", __FUNCTION__, ver,
            m_RenderVersionMajor, m_RenderVersionMinor);

  m_RenderExtensions  = " ";
  if (m_RenderVersionMajor > 3 ||
      (m_RenderVersionMajor == 3 && m_RenderVersionMinor >= 2))
  {
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    if (n > 0)
    {
      GLint i;
      for (i = 0; i < n; i++)
      {
        m_RenderExtensions += (const char*) glGetStringi(GL_EXTENSIONS, i);
        m_RenderExtensions += " ";
      }
    }
  }
  else
  {
    auto extensions = (const char*) glGetString(GL_EXTENSIONS);
    if (extensions)
    {
      m_RenderExtensions += extensions;
    }
  }
  m_RenderExtensions += " ";

  ver = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  if (ver)
  {
    sscanf(ver, "%d.%d", &m_glslMajor, &m_glslMinor);
  }
  else
  {
    m_glslMajor = 1;
    m_glslMinor = 0;
  }

#if defined(GL_KHR_debug) && defined(TARGET_LINUX)
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_openGlDebugging)
  {
    if (IsExtSupported("GL_KHR_debug"))
    {
      auto glDebugMessageCallback =
          CEGLUtils::GetRequiredProcAddress<PFNGLDEBUGMESSAGECALLBACKPROC>(
              "glDebugMessageCallback");
      auto glDebugMessageControl =
          CEGLUtils::GetRequiredProcAddress<PFNGLDEBUGMESSAGECONTROLPROC>("glDebugMessageControl");

      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageCallback(KODI::UTILS::GL::GlErrorCallback, nullptr);

      // ignore shader compilation information
      glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0,
                            nullptr, GL_FALSE);

      CLog::Log(LOGDEBUG, "OpenGL: debugging enabled");
    }
    else
    {
      CLog::Log(LOGDEBUG, "OpenGL: debugging requested but the required extension isn't "
                          "available (GL_KHR_debug)");
    }
  }
#endif

  LogGraphicsInfo();

  // Get our driver vendor and renderer
  const char* tmpVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderVendor.clear();
  if (tmpVendor != NULL)
    m_RenderVendor = tmpVendor;

  const char* tmpRenderer = (const char*) glGetString(GL_RENDERER);
  m_RenderRenderer.clear();
  if (tmpRenderer != NULL)
    m_RenderRenderer = tmpRenderer;

  m_bRenderCreated = true;

  if (m_RenderVersionMajor > 3 ||
      (m_RenderVersionMajor == 3 && m_RenderVersionMinor >= 2))
  {
    glGenVertexArrays(1, &m_vertexArray);
    glBindVertexArray(m_vertexArray);
  }

  InitialiseShaders();

  CGUITextureGL::Register();

  return true;
}

bool CRenderSystemGL::ResetRenderSystem(int width, int height)
{
  if (!m_bRenderCreated)
    return false;

  m_width = width;
  m_height = height;

  if (m_RenderVersionMajor > 3 ||
      (m_RenderVersionMajor == 3 && m_RenderVersionMinor >= 2))
  {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_vertexArray);
    glGenVertexArrays(1, &m_vertexArray);
    glBindVertexArray(m_vertexArray);
  }

  ReleaseShaders();
  InitialiseShaders();

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

  if (IsExtSupported("GL_ARB_multitexture"))
  {
    //clear error flags
    ResetGLErrors();

    GLint maxtex;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxtex);

    //some sanity checks
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "ResetRenderSystem() GL_MAX_TEXTURE_IMAGE_UNITS returned error {}",
                (int)error);
      maxtex = 3;
    }
    else if (maxtex < 1 || maxtex > 32)
    {
      CLog::Log(LOGERROR,
                "ResetRenderSystem() GL_MAX_TEXTURE_IMAGE_UNITS returned invalid value {}",
                (int)maxtex);
      maxtex = 3;
    }

    //reset texture matrix for all textures
    for (GLint i = 0; i < maxtex; i++)
    {
      glActiveTexture(GL_TEXTURE0 + i);
      glMatrixTexture.Load();
    }
    glActiveTexture(GL_TEXTURE0);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND); // Turn Blending On

  return true;
}

bool CRenderSystemGL::DestroyRenderSystem()
{
  if (m_vertexArray != GL_NONE)
  {
    glDeleteVertexArrays(1, &m_vertexArray);
  }

  ReleaseShaders();
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGL::BeginRender()
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

bool CRenderSystemGL::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

void CRenderSystemGL::InvalidateColorBuffer()
{
  if (!m_bRenderCreated)
    return;

  /* clear is not affected by stipple pattern, so we can only clear on first frame */
  if (m_stereoMode == RENDER_STEREO_MODE_INTERLACED && m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    return;

  // some platforms prefer a clear, instead of rendering over
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiGeometryClear)
  {
    ClearBuffers(0);
    return;
  }

  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiFrontToBackRendering)
    return;

  glClearDepthf(0);
  glDepthMask(GL_TRUE);
  glClear(GL_DEPTH_BUFFER_BIT);
}

bool CRenderSystemGL::ClearBuffers(UTILS::COLOR::Color color)
{
  if (!m_bRenderCreated)
    return false;

  /* clear is not affected by stipple pattern, so we can only clear on first frame */
  if(m_stereoMode == RENDER_STEREO_MODE_INTERLACED && m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    return true;

  float r = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color) / 255.0f;
  float g = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color) / 255.0f;
  float b = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color) / 255.0f;
  float a = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color) / 255.0f;

  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiFrontToBackRendering)
  {
    glClearDepthf(0);
    glDepthMask(GL_TRUE);
    flags |= GL_DEPTH_BUFFER_BIT;
  }

  glClear(flags);

  return true;
}

bool CRenderSystemGL::IsExtSupported(const char* extension) const
{
  if (m_RenderVersionMajor > 3 ||
      (m_RenderVersionMajor == 3 && m_RenderVersionMinor >= 2))
  {
    if (strcmp( extension, "GL_EXT_framebuffer_object") == 0)
    {
      return true;
    }
    if (strcmp( extension, "GL_ARB_texture_non_power_of_two") == 0)
    {
      return true;
    }
  }

  std::string name;
  name  = " ";
  name += extension;
  name += " ";

  return m_RenderExtensions.find(name) != std::string::npos;
}

bool CRenderSystemGL::SupportsNPOT(bool dxt) const
{
  return true;
}

void CRenderSystemGL::PresentRender(bool rendered, bool videoLayer)
{
  SetVSync(true);

  if (!m_bRenderCreated)
    return;

  PresentRenderImpl(rendered);

  if (!rendered)
    KODI::TIME::Sleep(40ms);
}

void CRenderSystemGL::SetVSync(bool enable)
{
  if (m_bVSync == enable && m_bVsyncInit == true)
    return;

  if (!m_bRenderCreated)
    return;

  if (enable)
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");

  m_bVSync = enable;
  m_bVsyncInit = true;

  SetVSyncImpl(enable);
}

void CRenderSystemGL::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;

  glMatrixProject.Push();
  glMatrixModview.Push();
  glMatrixTexture.Push();

  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  glActiveTexture(GL_TEXTURE0);
}

void CRenderSystemGL::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  glBindVertexArray(m_vertexArray);

  glViewport(m_viewPort[0], m_viewPort[1], m_viewPort[2], m_viewPort[3]);

  glMatrixProject.PopLoad();
  glMatrixModview.PopLoad();
  glMatrixTexture.PopLoad();

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);
}

void CRenderSystemGL::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor)
{
  if (!m_bRenderCreated)
    return;

  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);


  float w = (float)m_viewPort[2]*0.5f;
  float h = (float)m_viewPort[3]*0.5f;

  glMatrixModview->LoadIdentity();
  glMatrixModview->Translatef(-(w + offset.x - stereoFactor), +(h + offset.y), 0);
  glMatrixModview->LookAt(0.0f, 0.0f, -2.0f * h, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f);
  glMatrixModview.Load();

  glMatrixProject->LoadIdentity();
  glMatrixProject->Frustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  glMatrixProject.Load();
}

void CRenderSystemGL::Project(float &x, float &y, float &z)
{
  GLfloat coordX, coordY, coordZ;
  if (CMatrixGL::Project(x, y, z, glMatrixModview.Get(), glMatrixProject.Get(), m_viewPort, &coordX, &coordY, &coordZ))
  {
    x = coordX;
    y = (float)(m_viewPort[1] + m_viewPort[3] - coordY);
    z = 0;
  }
}

void CRenderSystemGL::CalculateMaxTexturesize()
{
  GLint width = 256;

  // reset any previous GL errors
  ResetGLErrors();

  // max out at 2^(8+8)
  for (int i = 0 ; i<8 ; i++)
  {
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, width, width, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                             &width);

    // GMA950 on OS X sets error instead
    if (width == 0 || (glGetError() != GL_NO_ERROR) )
      break;

    m_maxTextureSize = width;
    width *= 2;
    if (width > 65536) // have an upper limit in case driver acts stupid
    {
      CLog::Log(LOGERROR, "GL: Could not determine maximum texture width, falling back to 2048");
      m_maxTextureSize = 2048;
      break;
    }
  }

  CLog::Log(LOGINFO, "GL: Maximum texture width: {}", m_maxTextureSize);
}

void CRenderSystemGL::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  viewPort.x1 = m_viewPort[0];
  viewPort.y1 = m_height - m_viewPort[1] - m_viewPort[3];
  viewPort.x2 = m_viewPort[0] + m_viewPort[2];
  viewPort.y2 = viewPort.y1 + m_viewPort[3];
}

void CRenderSystemGL::SetViewPort(const CRect& viewPort)
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

bool CRenderSystemGL::ScissorsCanEffectClipping()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->HardwareClipIsPossible();

  return false;
}

CRect CRenderSystemGL::ClipRectToScissorRect(const CRect &rect)
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

void CRenderSystemGL::SetScissors(const CRect &rect)
{
  if (!m_bRenderCreated)
    return;
  GLint x1 = MathUtils::round_int(static_cast<double>(rect.x1));
  GLint y1 = MathUtils::round_int(static_cast<double>(rect.y1));
  GLint x2 = MathUtils::round_int(static_cast<double>(rect.x2));
  GLint y2 = MathUtils::round_int(static_cast<double>(rect.y2));
  glScissor(x1, m_height - y2, x2-x1, y2-y1);
}

void CRenderSystemGL::ResetScissors()
{
  SetScissors(CRect(0, 0, (float)m_width, (float)m_height));
}

void CRenderSystemGL::SetDepthCulling(DEPTH_CULLING culling)
{
  if (culling == DEPTH_CULLING_OFF)
  {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
  }
  else if (culling == DEPTH_CULLING_BACK_TO_FRONT)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_GEQUAL);
  }
  else if (culling == DEPTH_CULLING_FRONT_TO_BACK)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_GREATER);
  }
}

void CRenderSystemGL::GetGLSLVersion(int& major, int& minor)
{
  major = m_glslMajor;
  minor = m_glslMinor;
}

void CRenderSystemGL::ResetGLErrors()
{
  int count = 0;
  while (glGetError() != GL_NO_ERROR)
  {
    count++;
    if (count >= 100)
    {
      CLog::Log(
          LOGWARNING,
          "CRenderSystemGL::ResetGLErrors glGetError didn't return GL_NO_ERROR after {} iterations",
          count);
      break;
    }
  }
}
static const GLubyte stipple_3d[] = {
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
};

void CRenderSystemGL::SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view)
{
  CRenderSystemBase::SetStereoMode(mode, view);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDrawBuffer(GL_BACK);

  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
  }
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
  }
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
  }

  if(m_stereoMode == RENDER_STEREO_MODE_INTERLACED)
  {
    glEnable(GL_POLYGON_STIPPLE);
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      glPolygonStipple(stipple_3d);
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      glPolygonStipple(stipple_3d+4);
  }

  if(m_stereoMode == RENDER_STEREO_MODE_HARDWAREBASED)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      glDrawBuffer(GL_BACK_LEFT);
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      glDrawBuffer(GL_BACK_RIGHT);
  }

}

bool CRenderSystemGL::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch(mode)
  {
    case RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN:
    case RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA:
    case RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE:
    case RENDER_STEREO_MODE_INTERLACED:
      return true;
    case RENDER_STEREO_MODE_HARDWAREBASED: {
      //This is called by setting init, at which point GL is not inited
      //luckily if GL doesn't support this, it will just behave as if
      //it was not in effect.
      //GLboolean stereo = GL_FALSE;
      //glGetBooleanv(GL_STEREO, &stereo);
      //return stereo == GL_TRUE ? true : false;
      return true;
    }
    default:
      return CRenderSystemBase::SupportsStereo(mode);
  }
}

// -----------------------------------------------------------------------------
// shaders
// -----------------------------------------------------------------------------
void CRenderSystemGL::InitialiseShaders()
{
  std::string defines;
  m_limitedColorRange = CServiceBroker::GetWinSystem()->UseLimitedColor();
  if (m_limitedColorRange)
  {
    defines += "#define KODI_LIMITED_RANGE 1\n";
  }

  m_pShader[ShaderMethodGL::SM_DEFAULT] = std::make_unique<CGLShader>(
      "gl_shader_vert_default.glsl", "gl_shader_frag_default.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_DEFAULT]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_DEFAULT]->Free();
    m_pShader[ShaderMethodGL::SM_DEFAULT].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_frag_default.glsl - compile and link failed");
  }

  m_pShader[ShaderMethodGL::SM_TEXTURE] =
      std::make_unique<CGLShader>("gl_shader_frag_texture.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_TEXTURE]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_TEXTURE]->Free();
    m_pShader[ShaderMethodGL::SM_TEXTURE].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_frag_texture.glsl - compile and link failed");
  }

  m_pShader[ShaderMethodGL::SM_TEXTURE_LIM] =
      std::make_unique<CGLShader>("gl_shader_frag_texture_lim.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_TEXTURE_LIM]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_TEXTURE_LIM]->Free();
    m_pShader[ShaderMethodGL::SM_TEXTURE_LIM].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_frag_texture_lim.glsl - compile and link failed");
  }

  m_pShader[ShaderMethodGL::SM_MULTI] =
      std::make_unique<CGLShader>("gl_shader_frag_multi.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_MULTI]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_MULTI]->Free();
    m_pShader[ShaderMethodGL::SM_MULTI].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_frag_multi.glsl - compile and link failed");
  }

  m_pShader[ShaderMethodGL::SM_FONTS] = std::make_unique<CGLShader>(
      "gl_shader_vert_simple.glsl", "gl_shader_frag_fonts.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_FONTS]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_FONTS]->Free();
    m_pShader[ShaderMethodGL::SM_FONTS].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_vert_simple.glsl + gl_shader_frag_fonts.glsl - "
                        "compile and link failed");
  }

  m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP] =
      std::make_unique<CGLShader>("gl_shader_vert_clip.glsl", "gl_shader_frag_fonts.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP]->Free();
    m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_vert_clip.glsl + gl_shader_frag_fonts.glsl - compile "
                        "and link failed");
  }

  m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND] =
      std::make_unique<CGLShader>("gl_shader_frag_texture_noblend.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND]->Free();
    m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_frag_texture_noblend.glsl - compile and link failed");
  }

  m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR] =
      std::make_unique<CGLShader>("gl_shader_frag_multi_blendcolor.glsl", defines);
  if (!m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR]->CompileAndLink())
  {
    m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR]->Free();
    m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR].reset();
    CLog::Log(LOGERROR, "GUI Shader gl_shader_frag_multi_blendcolor.glsl - compile and link failed");
  }
}

void CRenderSystemGL::ReleaseShaders()
{
  if (m_pShader[ShaderMethodGL::SM_DEFAULT])
    m_pShader[ShaderMethodGL::SM_DEFAULT]->Free();
  m_pShader[ShaderMethodGL::SM_DEFAULT].reset();

  if (m_pShader[ShaderMethodGL::SM_TEXTURE])
    m_pShader[ShaderMethodGL::SM_TEXTURE]->Free();
  m_pShader[ShaderMethodGL::SM_TEXTURE].reset();

  if (m_pShader[ShaderMethodGL::SM_TEXTURE_LIM])
    m_pShader[ShaderMethodGL::SM_TEXTURE_LIM]->Free();
  m_pShader[ShaderMethodGL::SM_TEXTURE_LIM].reset();

  if (m_pShader[ShaderMethodGL::SM_MULTI])
    m_pShader[ShaderMethodGL::SM_MULTI]->Free();
  m_pShader[ShaderMethodGL::SM_MULTI].reset();

  if (m_pShader[ShaderMethodGL::SM_FONTS])
    m_pShader[ShaderMethodGL::SM_FONTS]->Free();
  m_pShader[ShaderMethodGL::SM_FONTS].reset();

  if (m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP])
    m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP]->Free();
  m_pShader[ShaderMethodGL::SM_FONTS_SHADER_CLIP].reset();

  if (m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND])
    m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND]->Free();
  m_pShader[ShaderMethodGL::SM_TEXTURE_NOBLEND].reset();

  if (m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR])
    m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR]->Free();
  m_pShader[ShaderMethodGL::SM_MULTI_BLENDCOLOR].reset();
}

void CRenderSystemGL::EnableShader(ShaderMethodGL method)
{
  m_method = method;
  if (m_pShader[m_method])
  {
    m_pShader[m_method]->Enable();
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid GUI Shader selected {}", method);
  }
}

void CRenderSystemGL::DisableShader()
{
  if (m_pShader[m_method])
  {
    m_pShader[m_method]->Disable();
  }
  m_method = ShaderMethodGL::SM_DEFAULT;
}

GLint CRenderSystemGL::ShaderGetPos()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetPosLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetCol()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetColLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetCoord0()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetCord0Loc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetCoord1()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetCord1Loc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetDepth()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetDepthLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetUniCol()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetUniColLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetModel()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetModelLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetMatrix()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetMatrixLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetClip()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetShaderClipLoc();

  return -1;
}

GLint CRenderSystemGL::ShaderGetCoordStep()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetShaderCoordStepLoc();

  return -1;
}

std::string CRenderSystemGL::GetShaderPath(const std::string &filename)
{
  std::string path = "GL/1.2/";

  if (m_glslMajor >= 4)
  {
    std::string file = "special://xbmc/system/shaders/GL/4.0/" + filename;
    const CURL pathToUrl(file);
    if (CFileUtils::Exists(pathToUrl.Get()))
      return "GL/4.0/";
  }
  if (m_glslMajor >= 2 || (m_glslMajor == 1 && m_glslMinor >= 50))
    path = "GL/1.5/";

  return path;
}
