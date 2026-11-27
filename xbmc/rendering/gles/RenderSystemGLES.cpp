/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystemGLES.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/DirtyRegion.h"
#include "guilib/GUITextureGLES.h"
#include "platform/MessagePrinter.h"
#include "rendering/GLExtensions.h"
#include "rendering/MatrixGL.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#if defined(TARGET_LINUX)
#include "utils/EGLUtils.h"
#endif

#include <array>

using namespace std::chrono_literals;

namespace
{
void ClearBuffersWithStencil(GLbitfield flags)
{
  GLint stencilMask, stencilBackMask, stencilClear;
  glGetIntegerv(GL_STENCIL_WRITEMASK, &stencilMask);
  glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &stencilBackMask);
  glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &stencilClear);
  glStencilMask(0xFF);
  glClearStencil(0);

  GLboolean depthMask{GL_FALSE};
  GLfloat depthClear{0.0f};
  if (flags & GL_DEPTH_BUFFER_BIT)
  {
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depthClear);
    glDepthMask(GL_TRUE);
    glClearDepthf(0.0f);
  }

  glClear(flags | GL_STENCIL_BUFFER_BIT);

  if (flags & GL_DEPTH_BUFFER_BIT)
  {
    glDepthMask(depthMask);
    glClearDepthf(depthClear);
  }
  glStencilMaskSeparate(GL_FRONT, stencilMask);
  glStencilMaskSeparate(GL_BACK, stencilBackMask);
  glClearStencil(stencilClear);
}
} // namespace

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
  m_RenderVersion = "<none>";
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != NULL)
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

  m_RenderExtensions = "";

  const char *tmpExtensions = (const char*) glGetString(GL_EXTENSIONS);
  if (tmpExtensions != NULL)
  {
    m_RenderExtensions += tmpExtensions;
    m_RenderExtensions += " ";
  }

#if defined(GL_KHR_debug) && defined(TARGET_LINUX)
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_openGlDebugging)
  {
    if (CGLExtensions::IsExtensionSupported(CGLExtensions::KHR_debug))
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

  // Shut down gracefully if OpenGL context could not be allocated
  if (m_RenderVersionMajor == 0)
  {
    CLog::Log(LOGFATAL, "Can not initialize OpenGL context. Exiting");
    CMessagePrinter::DisplayError("ERROR: Can not initialize OpenGL context. Exiting");
    return false;
  }

  LogGraphicsInfo();

  m_bRenderCreated = true;

  InitialiseShaders();

  CGUITextureGLES::Register(m_quadDrawer);

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
  // The DrawQuad callback holds a reference to m_quadDrawer; nothing may draw through it from now on.
  CGUITexture::UnregisterDrawQuad();
  m_guiQuadIndexBuffer.Destroy();
  m_guiUnitQuad.Destroy();
  m_bRenderCreated = false;

  return true;
}

void CRenderSystemGLES::BindGUIQuadIndices(std::size_t quadCount)
{
  const std::size_t needed = quadCount * 6;
  if (m_guiQuadIndexBuffer && m_guiQuadIndices.size() >= needed)
  {
    m_guiQuadIndexBuffer.Bind();
    return;
  }

  static constexpr std::array<GLushort, 6> pattern{0, 1, 2, 2, 3, 0};
  m_guiQuadIndices.reserve(needed);
  for (std::size_t quad = m_guiQuadIndices.size() / 6; quad < quadCount; ++quad)
  {
    for (GLushort offset : pattern)
      m_guiQuadIndices.push_back(static_cast<GLushort>(quad * 4 + offset));
  }
  m_guiQuadIndexBuffer.SetData(m_guiQuadIndices.data(), m_guiQuadIndices.size(), GL_STATIC_DRAW);
}

void CRenderSystemGLES::BindGUIUnitQuad()
{
  // Triangle strip order: top left, top right, bottom left, bottom right.
  static constexpr GLfloat unitQuad[4][2] = {
      {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
  m_guiUnitQuad.SetDataOnce(unitQuad);
}

void CRenderSystemGLES::DrawGUIQuad(const CPoint& origin,
                                    const CPoint& right,
                                    const CPoint& down,
                                    const CRect* texCoords)
{
  glUniformMatrix4fv(GUIShaderGetGUIMatrix(), 1, GL_FALSE,
                     KODI::UTILS::GL::QuadTransform(origin, right, down).data());

  BindGUIUnitQuad();
  const GLint posLoc = GUIShaderGetPos();
  glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(posLoc);

  const GLint texLoc = texCoords ? GUIShaderGetCoord0() : -1;
  if (texLoc >= 0)
  {
    const CRect& tex = *texCoords;
    glUniformMatrix4fv(
        GUIShaderGetCoord0Matrix(), 1, GL_FALSE,
        KODI::UTILS::GL::QuadTransform({tex.x1, tex.y1}, {tex.x2, tex.y1}, {tex.x1, tex.y2})
            .data());
    glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(texLoc);
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(posLoc);
  if (texLoc >= 0)
    glDisableVertexAttribArray(texLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CRenderSystemGLES::DrawGUIQuad(const CRect& rect, const CRect* texCoords)
{
  DrawGUIQuad({rect.x1, rect.y1}, {rect.x2, rect.y1}, {rect.x1, rect.y2}, texCoords);
}

bool CRenderSystemGLES::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  m_GUIElementCount = 0;

  const bool useLimited = CServiceBroker::GetWinSystem()->UseLimitedColor() &&
                          !CServiceBroker::GetWinSystem()->IsHdrComposite();
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

  ClearBuffers(0);
}

bool CRenderSystemGLES::ClearBuffers(KODI::UTILS::COLOR::Color color)
{
  if (!m_bRenderCreated)
    return false;

  float r = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color) / 255.0f;
  float g = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color) / 255.0f;
  float b = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color) / 255.0f;
  float a = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color) / 255.0f;

  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;

  if (GetEnabledFrontToBackRendering())
    flags |= GL_DEPTH_BUFFER_BIT;

  ClearBuffersWithStencil(flags);

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
  if (Shader(m_method))
    return Shader(m_method)->HardwareClipIsPossible();

  return false;
}

CRect CRenderSystemGLES::ClipRectToScissorRect(const CRect &rect)
{
  if (!Shader(m_method))
    return CRect();
  float xFactor = Shader(m_method)->GetClipXFactor();
  float xOffset = Shader(m_method)->GetClipXOffset();
  float yFactor = Shader(m_method)->GetClipYFactor();
  float yOffset = Shader(m_method)->GetClipYOffset();
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

void CRenderSystemGLES::SetDepthCulling(DepthCulling culling)
{
  if (culling == DepthCulling::OFF)
  {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
  }
  else if (culling == DepthCulling::BACK_TO_FRONT)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_GEQUAL);
  }
  else if (culling == DepthCulling::FRONT_TO_BACK)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_GREATER);
  }
}

void CRenderSystemGLES::InitialiseShaders()
{
  std::string defines;
  m_limitedColorRange = CServiceBroker::GetWinSystem()->UseLimitedColor() &&
                        !CServiceBroker::GetWinSystem()->IsHdrComposite();
  if (m_limitedColorRange)
  {
    defines += "#define KODI_LIMITED_RANGE 1\n";
  }

  if (m_transferPQ)
  {
    defines += "#define KODI_TRANSFER_PQ 1\n";
  }

  Shader(ShaderMethodGLES::SM_DEFAULT) =
      std::make_unique<CGLESShader>("gles_shader.vert", "gles_shader_default.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_DEFAULT)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_DEFAULT)->Free();
    Shader(ShaderMethodGLES::SM_DEFAULT).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_default.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_TEXTURE) =
      std::make_unique<CGLESShader>("gles_shader_texture.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_TEXTURE_111R) =
      std::make_unique<CGLESShader>("gles_shader_texture_111r.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE_111R)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_111R)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE_111R).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture_111r.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_MULTI) =
      std::make_unique<CGLESShader>("gles_shader_multi.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_MULTI)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_MULTI)->Free();
    Shader(ShaderMethodGLES::SM_MULTI).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R) =
      std::make_unique<CGLESShader>("gles_shader_multi_rgba_111r.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R)->Free();
    Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi_rgba_111r.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_FONTS) =
      std::make_unique<CGLESShader>("gles_shader_simple.vert", "gles_shader_fonts.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_FONTS)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_FONTS)->Free();
    Shader(ShaderMethodGLES::SM_FONTS).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_fonts.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP) =
      std::make_unique<CGLESShader>("gles_shader_clip.vert", "gles_shader_fonts.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP)->Free();
    Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_clip.vert + gles_shader_fonts.frag - compile "
                        "and link failed");
  }

  Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND) =
      std::make_unique<CGLESShader>("gles_shader_texture_noblend.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture_noblend.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR) =
      std::make_unique<CGLESShader>("gles_shader_multi_blendcolor.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR)->Free();
    Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_multi_blendcolor.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR) =
      std::make_unique<CGLESShader>("gles_shader_multi_rgba_111r_blendcolor.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR)->Free();
    Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR).reset();
    CLog::Log(LOGERROR,
              "GUI Shader gles_shader_multi_rgba_111r_blendcolor.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR) =
      std::make_unique<CGLESShader>("gles_shader_multi_111r_111r_blendcolor.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR)->Free();
    Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR).reset();
    CLog::Log(LOGERROR,
              "GUI Shader gles_shader_multi_111r_111r_blendcolor.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA) =
      std::make_unique<CGLESShader>("gles_shader_rgba.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE_RGBA)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR) =
      std::make_unique<CGLESShader>("gles_shader_rgba_blendcolor.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_blendcolor.frag - compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB) =
      std::make_unique<CGLESShader>("gles_shader_rgba_bob.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_bob.frag - compile and link failed");
  }

  if (CGLExtensions::IsExtensionSupported(CGLExtensions::OES_EGL_image_external))
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES) =
        std::make_unique<CGLESShader>("gles_shader_rgba_oes.frag", defines);
    if (!Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES)->CompileAndLink())
    {
      Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES)->Free();
      Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES).reset();
      CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_oes.frag - compile and link failed");
    }

    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES) =
        std::make_unique<CGLESShader>("gles_shader_rgba_bob_oes.frag", defines);
    if (!Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES)->CompileAndLink())
    {
      Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES)->Free();
      Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES).reset();
      CLog::Log(LOGERROR, "GUI Shader gles_shader_rgba_bob_oes.frag - compile and link failed");
    }
  }

  Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA) =
      std::make_unique<CGLESShader>("gles_shader_texture_noalpha.frag", defines);
  if (!Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA)->Free();
    Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_texture_noalpha.frag - compile and link failed");
  }

  const std::string definesFontsRGBA = defines + "#define KODI_FONTS_RGBA 1\n";

  Shader(ShaderMethodGLES::SM_FONTS_RGBA) = std::make_unique<CGLESShader>(
      "gles_shader_simple.vert", "gles_shader_fonts.frag", definesFontsRGBA);
  if (!Shader(ShaderMethodGLES::SM_FONTS_RGBA)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_FONTS_RGBA)->Free();
    Shader(ShaderMethodGLES::SM_FONTS_RGBA).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_simple.vert + gles_shader_fonts.frag (RGBA) - "
                        "compile and link failed");
  }

  Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP) = std::make_unique<CGLESShader>(
      "gles_shader_clip.vert", "gles_shader_fonts.frag", definesFontsRGBA);
  if (!Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP)->CompileAndLink())
  {
    Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP)->Free();
    Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP).reset();
    CLog::Log(LOGERROR, "GUI Shader gles_shader_clip.vert + gles_shader_fonts.frag (RGBA) - "
                        "compile and link failed");
  }
}

void CRenderSystemGLES::ReleaseShaders()
{
  glUseProgram(0);

  if (Shader(ShaderMethodGLES::SM_DEFAULT))
    Shader(ShaderMethodGLES::SM_DEFAULT)->Free();
  Shader(ShaderMethodGLES::SM_DEFAULT).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE))
    Shader(ShaderMethodGLES::SM_TEXTURE)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_111R))
    Shader(ShaderMethodGLES::SM_TEXTURE_111R)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_111R).reset();

  if (Shader(ShaderMethodGLES::SM_MULTI))
    Shader(ShaderMethodGLES::SM_MULTI)->Free();
  Shader(ShaderMethodGLES::SM_MULTI).reset();

  if (Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R))
    Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R)->Free();
  Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R).reset();

  if (Shader(ShaderMethodGLES::SM_FONTS))
    Shader(ShaderMethodGLES::SM_FONTS)->Free();
  Shader(ShaderMethodGLES::SM_FONTS).reset();

  if (Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP))
    Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP)->Free();
  Shader(ShaderMethodGLES::SM_FONTS_SHADER_CLIP).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND))
    Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_NOBLEND).reset();

  if (Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR))
    Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR)->Free();
  Shader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR).reset();

  if (Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR))
    Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR)->Free();
  Shader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR).reset();

  if (Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR))
    Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR)->Free();
  Shader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_RGBA))
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR))
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BLENDCOLOR).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB))
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES))
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES))
    Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES).reset();

  if (Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA))
    Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA)->Free();
  Shader(ShaderMethodGLES::SM_TEXTURE_NOALPHA).reset();

  if (Shader(ShaderMethodGLES::SM_FONTS_RGBA))
    Shader(ShaderMethodGLES::SM_FONTS_RGBA)->Free();
  Shader(ShaderMethodGLES::SM_FONTS_RGBA).reset();

  if (Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP))
    Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP)->Free();
  Shader(ShaderMethodGLES::SM_FONTS_RGBA_SHADER_CLIP).reset();
}

void CRenderSystemGLES::EnableGUIShader(ShaderMethodGLES method)
{
  m_method = method;
  if (Shader(m_method))
  {
    // Binding the program that is already current still makes some drivers revalidate it, and
    // other renderers and add-ons may have bound theirs since the last GUI draw.
    GLint current = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    if (Shader(m_method)->OK() && static_cast<GLuint>(current) == Shader(m_method)->ProgramHandle())
      Shader(m_method)->OnEnabled();
    else
      Shader(m_method)->Enable();
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid GUI Shader selected - {}", method);
  }
}

void CRenderSystemGLES::DisableGUIShader()
{
  // The program stays bound: unbinding it makes the driver revalidate all of its state when the
  // next GUI draw binds it again, which is costly on tile-based GPUs such as Mali.
  m_method = ShaderMethodGLES::SM_DEFAULT;
}

GLint CRenderSystemGLES::GUIShaderGetPos()
{
  if (Shader(m_method))
    return Shader(m_method)->GetPosLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCol()
{
  if (Shader(m_method))
    return Shader(m_method)->GetColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0()
{
  if (Shader(m_method))
    return Shader(m_method)->GetCord0Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord1()
{
  if (Shader(m_method))
    return Shader(m_method)->GetCord1Loc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetDepth()
{
  if (Shader(m_method))
    return Shader(m_method)->GetDepthLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetPma()
{
  if (Shader(m_method))
    return Shader(m_method)->GetPmaLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetGUIMatrix()
{
  if (Shader(m_method))
    return Shader(m_method)->GetGUIMatrixLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetSnap()
{
  if (Shader(m_method))
    return Shader(m_method)->GetSnapLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetAttrSnap()
{
  if (Shader(m_method))
    return Shader(m_method)->GetAttrSnapLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetQuadClip()
{
  if (Shader(m_method))
    return Shader(m_method)->GetQuadClipLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetQuadRect()
{
  if (Shader(m_method))
    return Shader(m_method)->GetQuadRectLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetTexSwap()
{
  if (Shader(m_method))
    return Shader(m_method)->GetTexSwapLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetUniCol()
{
  if (Shader(m_method))
    return Shader(m_method)->GetUniColLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoord0Matrix()
{
  if (Shader(m_method))
    return Shader(m_method)->GetCoord0MatrixLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetField()
{
  if (Shader(m_method))
    return Shader(m_method)->GetFieldLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetStep()
{
  if (Shader(m_method))
    return Shader(m_method)->GetStepLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetContrast()
{
  if (Shader(m_method))
    return Shader(m_method)->GetContrastLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetBrightness()
{
  if (Shader(m_method))
    return Shader(m_method)->GetBrightnessLoc();

  return -1;
}

bool CRenderSystemGLES::SupportsStereo(RenderStereoMode mode) const
{
  return CRenderSystemBase::SupportsStereo(mode);
}

GLint CRenderSystemGLES::GUIShaderGetMatrix()
{
  if (Shader(m_method))
    return Shader(m_method)->GetMatrixLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetClip()
{
  if (Shader(m_method))
    return Shader(m_method)->GetShaderClipLoc();

  return -1;
}

GLint CRenderSystemGLES::GUIShaderGetCoordStep()
{
  if (Shader(m_method))
    return Shader(m_method)->GetShaderCoordStepLoc();

  return -1;
}

std::string CRenderSystemGLES::GetShaderPath(const std::string& filename)
{
  std::string path = "GLES/2.0/";

  if (m_RenderVersionMajor >= 3 && m_RenderVersionMinor >= 1)
  {
    std::string file = "special://xbmc/system/shaders/GLES/3.1/" + filename;
    const CURL pathToUrl(file);
    if (CFileUtils::Exists(pathToUrl.Get()))
      return "GLES/3.1/";
  }

  return path;
}
