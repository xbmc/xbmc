/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(HAS_EGL) && defined(HAS_GLES)

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoRenderers/FrameBufferObject.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupMask.h"
#include "guilib/GUIFontTTFGLES.h"
#include "guilib/GUITextureGLES.h"
#include "guilib/Texture.h"
#include "guilib/TextureManager.h"
#include "rendering/MatrixGL.h"
#include "rendering/gles/GuiCompositeShaderGLES.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "test/TestUtils.h"
#include "windowing/WinSystem.h"

#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <EGL/egl.h>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace
{
constexpr int SIZE = 128;
// A 3:4 slope keeps pixel centers away from rasterization ties at the mask edges.
const float ANGLE = std::atan(0.75f);
const CRect SCREEN{0, 0, SIZE, SIZE};
const CRect MASK{32, 32, 96, 96};
using Pixels = std::array<std::array<GLubyte, 4>, SIZE * SIZE>;

bool InsideMask(int x, int y, const CRect& rect = MASK, float angle = ANGLE)
{
  const float dx = x + 0.5f - 64;
  const float dy = SIZE - y - 0.5f - 64;
  const float localX = std::cos(angle) * dx + std::sin(angle) * dy + 64;
  const float localY = -std::sin(angle) * dx + std::cos(angle) * dy + 64;
  return localX > rect.x1 && localX < rect.x2 && localY > rect.y1 && localY < rect.y2;
}

class CTestWinSystem : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CRenderSystemBase* GetRenderSystem() override { return this; }
  bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return false; }
  bool ResizeWindow(int, int, int, int) override { return false; }
  bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return false; }
  void Register(IDispResource*) override {}
  void Unregister(IDispResource*) override {}
  void SetVSyncImpl(bool) override {}
  void PresentRenderImpl(bool) override {}
  bool GetEnabledFrontToBackRendering() override { return frontToBack; }
  ClearFunction GetClearFunction() override { return clearFunction; }
  bool IsExtSupported(const char* extension) const override
  {
    if (separateStencil && std::strcmp(extension, "GL_OES_packed_depth_stencil") == 0)
      return false;
    return CRenderSystemGLES::IsExtSupported(extension);
  }
  void UseSeparateStencil()
  {
    separateStencil = true;
    m_RenderVersionMajor = 2;
  }
  bool frontToBack{false};
  ClearFunction clearFunction{ClearFunction::FIXED_FUNCTION};
  bool separateStencil{false};
};

class CTestGUI : public CGUIComponent
{
public:
  CTestGUI() : CGUIComponent(false) { m_pTextureManager = std::make_unique<CGUITextureManager>(); }
};

struct GLState
{
  GLState()
  {
    constexpr GLenum fields[]{GL_STENCIL_FUNC,
                              GL_STENCIL_REF,
                              GL_STENCIL_VALUE_MASK,
                              GL_STENCIL_WRITEMASK,
                              GL_STENCIL_FAIL,
                              GL_STENCIL_PASS_DEPTH_FAIL,
                              GL_STENCIL_PASS_DEPTH_PASS,
                              GL_STENCIL_BACK_FUNC,
                              GL_STENCIL_BACK_REF,
                              GL_STENCIL_BACK_VALUE_MASK,
                              GL_STENCIL_BACK_WRITEMASK,
                              GL_STENCIL_BACK_FAIL,
                              GL_STENCIL_BACK_PASS_DEPTH_FAIL,
                              GL_STENCIL_BACK_PASS_DEPTH_PASS,
                              GL_STENCIL_CLEAR_VALUE,
                              GL_DEPTH_WRITEMASK};
    for (size_t i = 0; i < std::size(fields); ++i)
      glGetIntegerv(fields[i], &values[i]);
    glGetBooleanv(GL_COLOR_WRITEMASK, color.data());
    glGetIntegerv(GL_SCISSOR_BOX, scissor.data());
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depthClear);
    stencil = glIsEnabled(GL_STENCIL_TEST);
    depth = glIsEnabled(GL_DEPTH_TEST);
  }
  bool operator==(const GLState&) const = default;
  std::array<GLint, 16> values{};
  std::array<GLboolean, 4> color{};
  std::array<GLint, 4> scissor{};
  GLfloat depthClear{};
  GLboolean stencil{};
  GLboolean depth{};
};

class CTestTexture : public CGUITextureGLES
{
public:
  CTestTexture() : CGUITextureGLES(0, 0, SIZE, SIZE, {})
  {
    auto texture = CTexture::CreateTexture();
    const unsigned char white[]{255, 255, 255, 255};
    texture->LoadFromMemory(1, 1, 4, XB_FMT_A8R8G8B8, false, white);
    m_texture.Add(std::move(texture), 0);
  }
  void Paint()
  {
    Begin(0xffffffff);
    DrawQuads({{CRect(0, 0, 1, 1), {}, CRect(0, 0, 1, 1), {}}}, 1, SCREEN);
    End();
  }
};

class CTestFont : public CGUIFontTTFGLES
{
public:
  CTestFont() : CGUIFontTTFGLES("groupmask-test") {}
  void Paint(CGraphicContext& context)
  {
    const std::array<KODI::UTILS::COLOR::Color, 1> colors{0xffffffff};
    const vecText text{'M', 'A', 'S', 'K', 'M', 'A', 'S', 'K'};
    DrawTextInternal(context, 0, 48, colors, text, 0, 0, false, false);
  }
};

class CPaintControl : public CGUIControl
{
public:
  CPaintControl() : CGUIControl(0, 0, 0, 0, SIZE, SIZE) {}
  CPaintControl* Clone() const override { return new CPaintControl(*this); }
  void Render() override { CGUITexture::DrawQuad(SCREEN, 0xffffffff); }
};

class CRotatingMask : public CGUIControlGroupMask
{
public:
  CRotatingMask() : CGUIControlGroupMask(0, 0, MASK.x1, MASK.y1, MASK.Width(), MASK.Height()) {}
  bool Animate(unsigned int) override
  {
    m_transform.SetZRotation(angle, 64, 64);
    return true;
  }
  float angle{ANGLE};
};

class TestGUIControlGroupMaskGLES : public testing::TestWithParam<bool>
{
protected:
  void SetUp() override
  {
    m_previousWinSystem = CServiceBroker::GetWinSystem();
    m_previousGUI = CServiceBroker::GetGUI();
    m_previousDisplay = eglGetCurrentDisplay();
    m_previousContext = eglGetCurrentContext();
    m_previousDraw = eglGetCurrentSurface(EGL_DRAW);
    m_previousRead = eglGetCurrentSurface(EGL_READ);
    m_previousAPI = eglQueryAPI();
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
      GTEST_SKIP() << "EGL display unavailable (Mesa can use EGL_PLATFORM=surfaceless)";
    ASSERT_EQ(eglBindAPI(EGL_OPENGL_ES_API), EGL_TRUE);
    const EGLint attributes[]{EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,
                              EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RED_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_ALPHA_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              16,
                              EGL_STENCIL_SIZE,
                              8,
                              EGL_NONE};
    EGLConfig config{};
    EGLint count = 0;
    ASSERT_EQ(eglChooseConfig(m_display, attributes, &config, 1, &count), EGL_TRUE);
    if (count == 0)
      GTEST_SKIP() << "EGL pbuffer with eight stencil bits unavailable";
    const EGLint contextAttributes[]{EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, contextAttributes);
    ASSERT_NE(m_context, EGL_NO_CONTEXT);
    const EGLint surfaceAttributes[]{EGL_WIDTH, SIZE, EGL_HEIGHT, SIZE, EGL_NONE};
    m_surface = eglCreatePbufferSurface(m_display, config, surfaceAttributes);
    ASSERT_NE(m_surface, EGL_NO_SURFACE);
    ASSERT_EQ(eglMakeCurrent(m_display, m_surface, m_surface, m_context), EGL_TRUE);

    m_window = std::make_unique<CTestWinSystem>();
    CServiceBroker::RegisterWinSystem(m_window.get());
    m_gui = std::make_unique<CTestGUI>();
    CServiceBroker::RegisterGUI(m_gui.get());
    m_window->frontToBack = GetParam();
    ASSERT_TRUE(m_window->InitRenderSystem());
    ASSERT_TRUE(m_window->ResetRenderSystem(SIZE, SIZE));
    glMatrixProject->LoadIdentity();
    glMatrixProject->Ortho(0, SIZE, SIZE, 0, -1, 1);
    Context().SetScissors(SCREEN);
    Context().SetClipRegion(0, 0, SIZE, SIZE);
    Context().SetRenderOrder(GetParam() ? RENDER_ORDER_BACK_TO_FRONT
                                        : RENDER_ORDER_ALL_BACK_TO_FRONT);
    ASSERT_TRUE(m_window->ClearBuffers(0));
    for (auto shader : {ShaderMethodGLES::SM_DEFAULT, ShaderMethodGLES::SM_TEXTURE_NOBLEND,
                        ShaderMethodGLES::SM_FONTS})
    {
      m_window->EnableGUIShader(shader);
      ASSERT_GE(m_window->GUIShaderGetPos(), 0);
      m_window->DisableGUIShader();
    }
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    Paint();
    ASSERT_EQ(ReadPixels()[64 * SIZE + 64][0], 255);
    ASSERT_TRUE(m_window->ClearBuffers(0));
    RecordProperty("renderer", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    RecordProperty("version", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
  }

  void TearDown() override
  {
    if (m_window)
    {
      CGUIFontTTFGLES::DestroyStaticVertexBuffers();
      m_window->DestroyRenderSystem();
      CGUITexture::Register({}, {}, {});
      m_gui.reset();
      if (m_previousGUI)
        CServiceBroker::RegisterGUI(m_previousGUI);
      m_window.reset();
      if (m_previousWinSystem)
        CServiceBroker::RegisterWinSystem(m_previousWinSystem);
      else
        CServiceBroker::UnregisterWinSystem();
    }
    if (m_display != EGL_NO_DISPLAY)
    {
      eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);
      if (m_context != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_context);
      if (m_previousDisplay == EGL_NO_DISPLAY)
        eglTerminate(m_display);
    }
    eglBindAPI(m_previousAPI);
    if (m_previousDisplay != EGL_NO_DISPLAY)
      eglMakeCurrent(m_previousDisplay, m_previousDraw, m_previousRead, m_previousContext);
  }

  CGraphicContext& Context() { return m_window->GetGfxContext(); }
  void Mask(const CRect& rect = MASK, STENCIL_LAYER layer = STENCIL_LAYER::LAYER0)
  {
    TransformMatrix transform;
    transform.SetZRotation(ANGLE, 64, 64);
    Context().AddTransform(transform);
    CGUITexture::DrawStencilQuad(rect, layer);
    Context().RemoveTransform();
  }
  Pixels ReadPixels()
  {
    Pixels pixels{};
    glReadPixels(0, 0, SIZE, SIZE, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    return pixels;
  }
  void ExpectMask(const Pixels& pixels, const CRect& rect = MASK, float angle = ANGLE)
  {
    unsigned int wrong = 0;
    for (int y = 0; y < SIZE; ++y)
    {
      for (int x = 0; x < SIZE; ++x)
      {
        wrong += pixels[y * SIZE + x][0] != (InsideMask(x, y, rect, angle) ? 255 : 0);
      }
    }
    EXPECT_EQ(wrong, 0u);
  }
  void CompositeGui(GLuint texture)
  {
    CGuiCompositeShaderGLES composite("");
    ASSERT_TRUE(composite.CompileAndLink());
    ASSERT_TRUE(composite.CreateLUTs(AVCOL_TRC_SMPTE2084));
    const GLfloat projection[]{2, 0, 0, 0, 0, 2, 0, 0, 0, 0, -1, 0, -1, -1, 0, 1};
    composite.SetProjection(projection);
    ASSERT_TRUE(composite.Enable());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    m_window->BindGUIUnitQuad();
    for (GLint location : {composite.GetPosLoc(), composite.GetTexLoc()})
    {
      glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
      glEnableVertexAttribArray(location);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    for (GLint location : {composite.GetPosLoc(), composite.GetTexLoc()})
      glDisableVertexAttribArray(location);
    composite.Disable();
  }

  void Paint() { CGUITexture::DrawQuad(SCREEN, 0xffffffff); }

  std::unique_ptr<CTestWinSystem> m_window;

private:
  CWinSystemBase* m_previousWinSystem{nullptr};
  CGUIComponent* m_previousGUI{nullptr};
  std::unique_ptr<CTestGUI> m_gui;
  EGLDisplay m_previousDisplay{EGL_NO_DISPLAY};
  EGLContext m_previousContext{EGL_NO_CONTEXT};
  EGLSurface m_previousDraw{EGL_NO_SURFACE};
  EGLSurface m_previousRead{EGL_NO_SURFACE};
  EGLenum m_previousAPI{EGL_OPENGL_ES_API};
  EGLDisplay m_display{EGL_NO_DISPLAY};
  EGLContext m_context{EGL_NO_CONTEXT};
  EGLSurface m_surface{EGL_NO_SURFACE};
};
} // namespace

TEST_P(TestGUIControlGroupMaskGLES, RotatedMaskClipsDirectAndBatchedQuads)
{
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  ExpectMask(ReadPixels());
  glClear(GL_COLOR_BUFFER_BIT);
  CTestTexture texture;
  texture.Paint();
  ExpectMask(ReadPixels());
  Context().RestoreStencilLayer();
  Paint();
  const auto pixels = ReadPixels();
  for (const auto& pixel : pixels)
    ASSERT_EQ(pixel[0], 255);
}

TEST_P(TestGUIControlGroupMaskGLES, NestedAndOverlappingLayersKeepTheirOwnMasks)
{
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Mask(CRect(64, 16, 112, 112), STENCIL_LAYER::LAYER1);
  Context().SetStencilLayer(STENCIL_LAYER::LAYER1);
  Paint();
  ExpectMask(ReadPixels(), CRect(64, 32, 96, 96));
  Context().RestoreStencilLayer();
  glClear(GL_COLOR_BUFFER_BIT);
  Paint();
  ExpectMask(ReadPixels());
  Context().RestoreStencilLayer();
  Mask(CRect(64, 16, 112, 112), STENCIL_LAYER::LAYER2);
  Context().SetStencilLayer(STENCIL_LAYER::LAYER2);
  glClear(GL_COLOR_BUFFER_BIT);
  Paint();
  ExpectMask(ReadPixels(), CRect(64, 16, 112, 112));
  Context().RestoreStencilLayer();
  for (unsigned int i = 3; i < 8; ++i)
  {
    const auto layer = static_cast<STENCIL_LAYER>(i);
    Mask(MASK, layer);
    Context().SetStencilLayer(layer);
    glClear(GL_COLOR_BUFFER_BIT);
    Paint();
    ExpectMask(ReadPixels());
    Context().RestoreStencilLayer();
  }
}

TEST_P(TestGUIControlGroupMaskGLES, RotationTransitionsAndBucketExhaustion)
{
  CRotatingMask group;
  group.AddControl(new CPaintControl);
  CDirtyRegionList dirtyRegions;
  for (const float angle : {0.0f, ANGLE, 0.0f, ANGLE})
  {
    Context().ClearStencilBuckets();
    group.angle = angle;
    group.DoProcess(0, dirtyRegions);
    m_window->ClearBuffers(0);
    if (GetParam())
    {
      Context().SetRenderOrder(RENDER_ORDER_FRONT_TO_BACK);
      group.DoRender();
      Context().SetRenderOrder(RENDER_ORDER_BACK_TO_FRONT);
    }
    group.DoRender();
    ExpectMask(ReadPixels(), MASK, angle);
    EXPECT_EQ(Context().GetScissors(), SCREEN);
    EXPECT_EQ(Context().GetStencilLayer(), STENCIL_LAYER::NONE);
  }

  Context().ClearStencilBuckets();
  for (unsigned int i = 0; i < 8; ++i)
    ASSERT_EQ(Context().AddStencilRectToBucket(SCREEN), static_cast<STENCIL_LAYER>(i));
  group.DoProcess(0, dirtyRegions);
  m_window->ClearBuffers(0);
  group.DoRender();
  EXPECT_EQ(ReadPixels(), Pixels{});
}

TEST_P(TestGUIControlGroupMaskGLES, DisjointMasksShareABucket)
{
  const CRect first{24, 24, 40, 40};
  const CRect second{88, 88, 104, 104};
  TransformMatrix transform;
  transform.SetZRotation(ANGLE, 64, 64);
  Context().AddTransform(transform);
  const auto firstLayer = Context().AddStencilRectToBucket(Context().GenerateAABB(first));
  const auto secondLayer = Context().AddStencilRectToBucket(Context().GenerateAABB(second));
  EXPECT_EQ(firstLayer, secondLayer);
  CGUITexture::DrawStencilQuad(first, firstLayer);
  CGUITexture::DrawStencilQuad(second, secondLayer);
  Context().RemoveTransform();
  Context().SetStencilLayer(firstLayer);
  Paint();
  Context().RestoreStencilLayer();
  const auto pixels = ReadPixels();
  for (int y = 0; y < SIZE; ++y)
    for (int x = 0; x < SIZE; ++x)
      ASSERT_EQ(pixels[y * SIZE + x][0],
                InsideMask(x, y, first) || InsideMask(x, y, second) ? 255 : 0);
}

TEST_P(TestGUIControlGroupMaskGLES, TextUsesTheSameStencilMask)
{
  CTestFont font;
  ASSERT_TRUE(font.Load(XBMC_REF_FILE_PATH("media/Fonts/arial.ttf"), 32));
  font.Paint(Context());
  const auto unmasked = ReadPixels();
  ASSERT_TRUE(m_window->ClearBuffers(0));
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  const GLState beforeText;
  font.Paint(Context());
  EXPECT_EQ(GLState(), beforeText);
  const auto masked = ReadPixels();
  Context().RestoreStencilLayer();

  unsigned int visible = 0;
  unsigned int clipped = 0;
  for (int y = 0; y < SIZE; ++y)
  {
    for (int x = 0; x < SIZE; ++x)
    {
      const bool inside = InsideMask(x, y);
      const size_t i = y * SIZE + x;
      ASSERT_EQ(masked[i], inside ? unmasked[i] : Pixels::value_type{});
      visible += inside && masked[i][0] != 0;
      clipped += !inside && unmasked[i][0] != 0;
    }
  }
  EXPECT_GT(visible, 0u);
  EXPECT_GT(clipped, 0u);
}

TEST_P(TestGUIControlGroupMaskGLES, StencilDrawsRestoreState)
{
  glEnable(GL_STENCIL_TEST);
  glStencilFuncSeparate(GL_FRONT, GL_NEVER, 3, 0x12);
  glStencilFuncSeparate(GL_BACK, GL_GREATER, 5, 0x24);
  glStencilOpSeparate(GL_FRONT, GL_ZERO, GL_INCR, GL_DECR);
  glStencilOpSeparate(GL_BACK, GL_INVERT, GL_REPLACE, GL_KEEP);
  glStencilMaskSeparate(GL_FRONT, 0x56);
  glStencilMaskSeparate(GL_BACK, 0x78);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_NEVER);
  glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
  glDepthMask(GL_FALSE);
  const GLState beforeMask;
  Mask();
  EXPECT_EQ(GLState(), beforeMask);
  EXPECT_EQ(ReadPixels(), Pixels{});

  glDisable(GL_DEPTH_TEST);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  const GLState beforeChildren;
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  EXPECT_EQ(GLState(), beforeChildren);
  ExpectMask(ReadPixels());
  CTestTexture texture;
  texture.Paint();
  EXPECT_EQ(GLState(), beforeChildren);
  Context().RestoreStencilLayer();
  glDisable(GL_STENCIL_TEST);
}

TEST_P(TestGUIControlGroupMaskGLES, ClearsStencilWithRestrictedWriteMasksAndDirtyRegion)
{
  for (const int clear : {0, 1, 2})
  {
    SCOPED_TRACE(clear);
    Context().SetScissors(SCREEN);
    glStencilMask(0xff);
    glClearStencil(0xff);
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glClearStencil(7);
    glStencilMaskSeparate(GL_FRONT, 0);
    glStencilMaskSeparate(GL_BACK, 0x20);
    glDepthMask(GL_FALSE);
    glClearDepthf(0.75f);
    Context().SetScissors(CRect(32, 32, 64, 96));
    const GLState beforeClear;
    if (clear == 0)
      ASSERT_TRUE(m_window->ClearBuffers(0));
    else
    {
      m_window->clearFunction =
          clear == 1 ? ClearFunction::FIXED_FUNCTION : ClearFunction::GEOMETRY;
      m_window->InvalidateColorBuffer();
    }
    EXPECT_EQ(GLState(), beforeClear);
    Context().SetScissors(SCREEN);
    Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
    Paint();
    Context().RestoreStencilLayer();
    const auto pixels = ReadPixels();
    for (int y = 0; y < SIZE; ++y)
      for (int x = 0; x < SIZE; ++x)
        ASSERT_EQ(pixels[y * SIZE + x][0], x >= 32 && x < 64 && y >= 32 && y < 96 ? 0 : 255);
  }
}

TEST_P(TestGUIControlGroupMaskGLES, FramebufferStencilAndHDRComposite)
{
  CFrameBufferObject fbo;
  ASSERT_TRUE(fbo.Initialize());
  ASSERT_TRUE(fbo.CreateAndBindToTexture(GL_TEXTURE_2D, SIZE, SIZE, GL_RGBA));
  ASSERT_TRUE(fbo.AttachDepthBuffer(SIZE, SIZE));
  ASSERT_TRUE(fbo.BeginRender());
  GLint stencilBits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
  ASSERT_GE(stencilBits, 8);
  ASSERT_TRUE(m_window->ClearBuffers(0));
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  Context().RestoreStencilLayer();
  ExpectMask(ReadPixels());
  fbo.EndRender();

  CompositeGui(fbo.Texture());
  const auto pixels = ReadPixels();
  EXPECT_NEAR(pixels[64 * SIZE + 64][0], 148, 2);
  EXPECT_EQ(pixels[0], Pixels::value_type{});
  fbo.Cleanup();
}

TEST_P(TestGUIControlGroupMaskGLES, SeparateStencilAttachmentAndMissingStencil)
{
  m_window->UseSeparateStencil();
  CFrameBufferObject fbo;
  ASSERT_TRUE(fbo.Initialize());
  ASSERT_TRUE(fbo.CreateAndBindToTexture(GL_TEXTURE_2D, SIZE, SIZE, GL_RGBA));
  ASSERT_TRUE(fbo.BeginRender());
  ASSERT_TRUE(m_window->ClearBuffers(0));
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  EXPECT_EQ(ReadPixels(), Pixels{});
  CTestTexture texture;
  texture.Paint();
  EXPECT_EQ(ReadPixels(), Pixels{});
  CTestFont font;
  ASSERT_TRUE(font.Load(XBMC_REF_FILE_PATH("media/Fonts/arial.ttf"), 32));
  font.Paint(Context());
  EXPECT_EQ(ReadPixels(), Pixels{});
  Context().RestoreStencilLayer();
  fbo.EndRender();
  const GLState beforeAttachment;
  ASSERT_TRUE(fbo.AttachDepthBuffer(SIZE, SIZE));
  EXPECT_EQ(GLState(), beforeAttachment);
  GLint binding = -1;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &binding);
  EXPECT_EQ(binding, 0);
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &binding);
  EXPECT_EQ(binding, 0);
  ASSERT_TRUE(fbo.BeginRender());
  ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
  GLint stencilBits = 0, depthBits = 0, depthBuffer = 0, framebuffer = 0;
  glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
  glGetIntegerv(GL_DEPTH_BITS, &depthBits);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthBuffer);
  ASSERT_GE(depthBits, 16);
  ASSERT_TRUE(stencilBits == 0 || stencilBits >= 8);
  RecordProperty("depth_only_fallback", stencilBits == 0);
  ASSERT_TRUE(m_window->ClearBuffers(0));
  Mask();
  const GLState beforeChildren;
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  if (stencilBits >= 8)
    ExpectMask(ReadPixels());
  else
  {
    GLint attachmentType = -1;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
    EXPECT_EQ(attachmentType, GL_NONE);
    texture.Paint();
    font.Paint(Context());
    EXPECT_EQ(ReadPixels(), Pixels{});
  }
  Context().RestoreStencilLayer();
  EXPECT_EQ(GLState(), beforeChildren);
  ASSERT_TRUE(m_window->ClearBuffers(0));
  Paint();
  for (const auto& pixel : ReadPixels())
    ASSERT_EQ(pixel[0], 255);
  fbo.EndRender();

  CompositeGui(fbo.Texture());
  const auto pixels = ReadPixels();
  EXPECT_NEAR(pixels[64 * SIZE + 64][0], 148, 2);
  EXPECT_NEAR(pixels[0][0], 148, 2);
  EXPECT_EQ(pixels[0][3], 255);
  const GLuint colorTexture = fbo.Texture();
  const GLState beforeCleanup;
  fbo.Cleanup();
  EXPECT_EQ(GLState(), beforeCleanup);
  EXPECT_FALSE(glIsFramebuffer(framebuffer));
  EXPECT_FALSE(glIsRenderbuffer(depthBuffer));
  EXPECT_FALSE(glIsTexture(colorTexture));
  EXPECT_FALSE(fbo.BeginRender());
  EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_P(TestGUIControlGroupMaskGLES, FailedDepthOnlyFallbackCleansAttachments)
{
  m_window->UseSeparateStencil();
  CFrameBufferObject fbo;
  ASSERT_TRUE(fbo.Initialize());
  ASSERT_TRUE(fbo.CreateAndBindToTexture(GL_TEXTURE_2D, SIZE, SIZE, GL_RGBA));
  for (int width : {0, -1})
  {
    SCOPED_TRACE(width);
    const GLState beforeAttachment;
    EXPECT_FALSE(fbo.AttachDepthBuffer(width, SIZE));
    EXPECT_EQ(GLState(), beforeAttachment);
    ASSERT_TRUE(fbo.BeginRender());
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    for (GLenum attachment : {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT})
    {
      GLint type = -1;
      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
      EXPECT_EQ(type, GL_NONE);
    }
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    ASSERT_TRUE(m_window->ClearBuffers(0));
    Paint();
    for (const auto& pixel : ReadPixels())
      ASSERT_EQ(pixel[0], 255);
    fbo.EndRender();
  }
  fbo.Cleanup();
}

TEST_P(TestGUIControlGroupMaskGLES, InvalidLayersDoNotDraw)
{
  const GLState beforeMask;
  for (auto layer : {STENCIL_LAYER::NONE, STENCIL_LAYER::INVALID})
    CGUITexture::DrawStencilQuad(SCREEN, layer);
  EXPECT_EQ(GLState(), beforeMask);
  EXPECT_EQ(ReadPixels(), Pixels{});
  Context().SetStencilLayer(STENCIL_LAYER::INVALID);
  Paint();
  CTestTexture texture;
  texture.Paint();
  EXPECT_EQ(ReadPixels(), Pixels{});
  Context().RestoreStencilLayer();
}

INSTANTIATE_TEST_SUITE_P(RenderPasses, TestGUIControlGroupMaskGLES, testing::Bool());

#endif
