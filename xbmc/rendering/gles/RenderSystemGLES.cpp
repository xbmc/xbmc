/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystemGLES.h"

#include "guilib/DirtyRegion.h"
#include "guilib/GUITextureGLES.h"
#include "rendering/MatrixGL.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#if defined(TARGET_LINUX)
#include "utils/EGLUtils.h"
#endif

using namespace std::chrono_literals;

CRenderSystemGLES::CRenderSystemGLES()
 : CRenderSystemBase()
{
}

bool CRenderSystemGLES::InitRenderSystem()
{
  GLint maxTextureSize;

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

  m_maxTextureSize = maxTextureSize;

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

#if defined(GL_KHR_debug) && defined(TARGET_LINUX)
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_openGlDebugging)
  {
    if (IsExtSupported("GL_KHR_debug"))
    {
      auto glDebugMessageCallback = CEGLUtils::GetRequiredProcAddress<PFNGLDEBUGMESSAGECALLBACKKHRPROC>("glDebugMessageCallbackKHR");
      auto glDebugMessageControl = CEGLUtils::GetRequiredProcAddress<PFNGLDEBUGMESSAGECONTROLKHRPROC>("glDebugMessageControlKHR");

      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
      glDebugMessageCallback(KODI::UTILS::GL::GlErrorCallback, nullptr);

      // ignore shader compilation information
      glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER_KHR, GL_DEBUG_TYPE_OTHER_KHR, GL_DONT_CARE, 0, nullptr, GL_FALSE);

      CLog::Log(LOGDEBUG, "OpenGL(ES): debugging enabled");
    }
    else
    {
      CLog::Log(LOGDEBUG, "OpenGL(ES): debugging requested but the required extension isn't available (GL_KHR_debug)");
    }
  }
#endif

  LogGraphicsInfo();

  m_bRenderCreated = true;

  InitialiseShaders();

  CGUITextureGLES::Register();

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
  glEnable(GL_BLEND); // Turn Blending On

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

  const bool useLimited = CServiceBroker::GetWinSystem()->UseLimitedColor();
  const bool usePQ = CServiceBroker::GetWinSystem()->GetGfxContext().IsTransferPQ();

  if (m_limitedColorRange != useLimited || m_transferPQ != usePQ)
  {
    ReleaseShaders();

    m_limitedColorRange = useLimited;
    m_transferPQ = usePQ;

    InitialiseShaders();
  }

  return true;
}

bool CRenderSystemGLES::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

void CRenderSystemGLES::InvalidateColorBuffer()
{
  if (!m_bRenderCreated)
    return;

  // some platforms prefer a clear, instead of rendering over
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiGeometryClear)
    ClearBuffers(0);

  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiFrontToBackRendering)
    return;

  glClearDepthf(0);
  glDepthMask(true);
  glClear(GL_DEPTH_BUFFER_BIT);
}

bool CRenderSystemGLES::ClearBuffers(UTILS::COLOR::Color color)
{
  if (!m_bRenderCreated)
    return false;

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

bool CRenderSystemGLES::IsExtSupported(const char* extension) const
{
  if (strcmp( extension, "GL_EXT_framebuffer_object" ) == 0)
  {
    // GLES has FBO as a core element, not an extension!
    return true;
  }
  else
  {
    std::string name;
    name  = " ";
    name += extension;
    name += " ";

    return m_RenderExtensions.find(name) != std::string::npos;
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
    KODI::TIME::Sleep(40ms);
}

void CRenderSystemGLES::SetVSync(bool enable)
{
  if (m_bVsyncInit)
    return;

  if (!m_bRenderCreated)
    return;

  if (enable)
    CLog::Log(LOGINFO, "GLES: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GLES: Disabling VSYNC");

  m_bVsyncInit = true;

  SetVSyncImpl(enable);
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
  glMatrixModview->LookAt(0.0f, 0.0f, -2.0f * h, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f);
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
  CLog::Log(LOGINFO, "GLES: Maximum texture width: {}", m_maxTextureSize);
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
  GLint x1 = MathUtils::round_int(static_cast<double>(rect.x1));
  GLint y1 = MathUtils::round_int(static_cast<double>(rect.y1));
  GLint x2 = MathUtils::round_int(static_cast<double>(rect.x2));
  GLint y2 = MathUtils::round_int(static_cast<double>(rect.y2));
  glScissor(x1, m_height - y2, x2-x1, y2-y1);
}

void CRenderSystemGLES::ResetScissors()
{
  SetScissors(CRect(0, 0, (float)m_width, (float)m_height));
}

void CRenderSystemGLES::SetDepthCulling(DEPTH_CULLING culling)
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

void CRenderSystemGLES::InitialiseShaders()
{
  std::string defines;
  m_limitedColorRange = CServiceBroker::GetWinSystem()->UseLimitedColor();
  if (m_limitedColorRange)
  {
    defines += "#define KODI_LIMITED_RANGE 1\n";
  }

  if (m_transferPQ)
  {
    defines += "#define KODI_TRANSFER_PQ 1\n";
  }

  m_pShader[ShaderMethodGLES::SM_DEFAULT] =
      std::make_unique<CGLESShader>("gles_shader.vert", "gles_shader_default.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_DEFAULT]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_DEFAULT]->Free();
    m_pShader[ShaderMethodGLES::SM_DEFAULT].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_default.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_TEXTURE] =
      std::make_unique<CGLESShader>("gles_shader_texture.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_TEXTURE]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE]->Free();
    m_pShader[ShaderMethodGLES::SM_TEXTURE].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_MULTI] =
      std::make_unique<CGLESShader>("gles_shader_multi.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_MULTI]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_MULTI]->Free();
    m_pShader[ShaderMethodGLES::SM_MULTI].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_FONTS] =
      std::make_unique<CGLESShader>("gles_shader_simple.vert", "gles_shader_fonts.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_FONTS]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_FONTS]->Free();
    m_pShader[ShaderMethodGLES::SM_FONTS].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_fonts.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP] =
      std::make_unique<CGLESShader>("gles_shader_clip.vert", "gles_shader_fonts.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP]->Free();
    m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_clip.vert + gles_shader_fonts.frag - compile "
                        "and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND] =
      std::make_unique<CGLESShader>("gles_shader_texture_noblend.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND]->Free();
    m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture_noblend.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR] =
      std::make_unique<CGLESShader>("gles_shader_multi_blendcolor.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR]->Free();
    m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi_blendcolor.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA] =
      std::make_unique<CGLESShader>("gles_shader_rgba.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA]->Free();
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR] =
      std::make_unique<CGLESShader>("gles_shader_rgba_blendcolor.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR]->Free();
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_blendcolor.frag - compile and link failed");
  }

  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB] =
      std::make_unique<CGLESShader>("gles_shader_rgba_bob.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB]->Free();
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_bob.frag - compile and link failed");
  }

  if (IsExtSupported("GL_OES_EGL_image_external"))
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES] =
        std::make_unique<CGLESShader>("gles_shader_rgba_oes.frag", defines);
    if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES]->CompileAndLink())
    {
      m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES]->Free();
      m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES].reset();
      CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_oes.frag - compile and link failed");
    }


    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES] =
        std::make_unique<CGLESShader>("gles_shader_rgba_bob_oes.frag", defines);
    if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES]->CompileAndLink())
    {
      m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES]->Free();
      m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES].reset();
      CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_bob_oes.frag - compile and link failed");
    }
  }

  m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA] =
      std::make_unique<CGLESShader>("gles_shader_texture_noalpha.frag", defines);
  if (!m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA]->CompileAndLink())
  {
    m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA]->Free();
    m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA].reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture_noalpha.frag - compile and link failed");
  }
}

void CRenderSystemGLES::ReleaseShaders()
{
  if (m_pShader[ShaderMethodGLES::SM_DEFAULT])
    m_pShader[ShaderMethodGLES::SM_DEFAULT]->Free();
  m_pShader[ShaderMethodGLES::SM_DEFAULT].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE])
    m_pShader[ShaderMethodGLES::SM_TEXTURE]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE].reset();

  if (m_pShader[ShaderMethodGLES::SM_MULTI])
    m_pShader[ShaderMethodGLES::SM_MULTI]->Free();
  m_pShader[ShaderMethodGLES::SM_MULTI].reset();

  if (m_pShader[ShaderMethodGLES::SM_FONTS])
    m_pShader[ShaderMethodGLES::SM_FONTS]->Free();
  m_pShader[ShaderMethodGLES::SM_FONTS].reset();

  if (m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP])
    m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP]->Free();
  m_pShader[ShaderMethodGLES::SM_FONTS_SHADER_CLIP].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_NOBLEND].reset();

  if (m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR])
    m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR]->Free();
  m_pShader[ShaderMethodGLES::SM_MULTI_BLENDCOLOR].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_OES].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES].reset();

  if (m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA])
    m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA]->Free();
  m_pShader[ShaderMethodGLES::SM_TEXTURE_NOALPHA].reset();
}

void CRenderSystemGLES::EnableGUIShader(ShaderMethodGLES method)
{
  m_method = method;
  if (m_pShader[m_method])
  {
    m_pShader[m_method]->Enable();
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid GUI Shader selected - {}", method);
  }
}

void CRenderSystemGLES::DisableGUIShader()
{
  if (m_pShader[m_method])
  {
    m_pShader[m_method]->Disable();
  }
  m_method = ShaderMethodGLES::SM_DEFAULT;
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

GLint CRenderSystemGLES::GUIShaderGetDepth()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetDepthLoc();

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

GLint CRenderSystemGLES::GUIShaderGetMatrix()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetMatrixLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetClip()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetShaderClipLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoordStep()
{
  if (m_pShader[m_method])
    return m_pShader[m_method]->GetShaderCoordStepLoc();

  return -1;
}
