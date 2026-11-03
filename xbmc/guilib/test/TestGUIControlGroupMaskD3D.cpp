/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_WINDOWS_DESKTOP) && defined(HAS_DX)

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupMask.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIFontTTFDX.h"
#include "guilib/GUIShaderDX.h"
#include "guilib/GUITextureD3D.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureDX.h"
#include "guilib/TextureManager.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/dx/DeviceResources.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "windowing/windows/WinSystemWin32DX.h"

#include "platform/win32/CharsetConverter.h"

#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
constexpr int TARGET_SIZE = 128;
const float ANGLE = std::atan(0.75f);
const CRect SCREEN{0, 0, TARGET_SIZE, TARGET_SIZE};
const CRect MASK{32, 32, 96, 96};
using Pixels = std::array<std::array<unsigned char, 4>, TARGET_SIZE * TARGET_SIZE>;

bool InsideMask(int x, int y, const CRect& rect = MASK, float angle = ANGLE)
{
  const float dx = x + 0.5f - 64;
  const float dy = y + 0.5f - 64;
  const float localX = std::cos(angle) * dx + std::sin(angle) * dy + 64;
  const float localY = -std::sin(angle) * dx + std::cos(angle) * dy + 64;
  return localX > rect.x1 && localX < rect.x2 && localY > rect.y1 && localY < rect.y2;
}

class CTestWinSystem : public CWinSystemWin32DX
{
public:
  CTestWinSystem() { m_deviceResources = DX::DeviceResources::Get(); }
  bool GetEnabledFrontToBackRendering() override { return frontToBack; }
  ClearFunction GetClearFunction() override { return clearFunction; }
  bool UseLimitedColor() override { return false; }
  int GetBufferAge() override { return bufferAge; }
  int bufferAge{0};
  bool frontToBack{false};
  ClearFunction clearFunction{ClearFunction::FIXED_FUNCTION};
};

class CTestTexture : public CGUITextureD3D
{
public:
  CTestTexture() : CGUITextureD3D(0, 0, TARGET_SIZE, TARGET_SIZE, {})
  {
    auto texture = std::make_unique<CDXTexture>();
    const unsigned char white[]{255, 255, 255, 255};
    texture->LoadFromMemory(1, 1, 4, XB_FMT_A8R8G8B8, false, white);
    m_texture.Add(std::move(texture), 0);
  }
  void Paint()
  {
    float x[]{0, TARGET_SIZE, TARGET_SIZE, 0};
    float y[]{0, 0, TARGET_SIZE, TARGET_SIZE};
    float z[]{0, 0, 0, 0};
    Begin(0xffffffff);
    Draw(x, y, z, CRect(0, 0, 1, 1), {}, 0);
    End();
  }
};

class CTestFont : public CGUIFontTTFDX
{
public:
  CTestFont() : CGUIFontTTFDX("groupmask-test") {}
  ~CTestFont() override { OnDestroyDevice(false); }
  void Paint(CGraphicContext& context, float y = 48)
  {
    const std::array<KODI::UTILS::COLOR::Color, 1> colors{0xffffffff};
    const vecText text{'M', 'A', 'S', 'K', 'M', 'A', 'S', 'K'};
    DrawTextInternal(context, 0, y, colors, text, 0, 0, false, false);
  }
};

class CPaintControl : public CGUIControl
{
public:
  explicit CPaintControl(float size = TARGET_SIZE) : CGUIControl(0, 0, 0, 0, size, size) {}
  CPaintControl* Clone() const override { return new CPaintControl(*this); }
  void Render() override
  {
    layers.push_back(CServiceBroker::GetWinSystem()->GetGfxContext().GetStencilLayer());
    // Fill beyond the logical bounds so the mask determines the visible pixels.
    CGUITexture::DrawQuad(SCREEN, color);
  }
  KODI::UTILS::COLOR::Color color{0xffffffff};
  std::vector<STENCIL_LAYER> layers;
};

class CRotatingMask : public CGUIControlGroupMask
{
public:
  CRotatingMask() : CGUIControlGroupMask(0, 0, 32, 32, 64, 64) {}
  bool Animate(unsigned int) override
  {
    m_transform.SetZRotation(angle, 64, 64);
    return true;
  }
  float angle{ANGLE};
};

template<class Window>
class CMaskWindow : public Window
{
public:
  explicit CMaskWindow(int id) : Window(id, "")
  {
    this->m_active = true;
    this->m_bAllocated = true;
    this->m_needsScaling = false;
  }
  bool OnMessage(CGUIMessage&) override { return true; }
  void ClearBackground() override
  {
    scissors.push_back(CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors());
    Window::ClearBackground();
  }
  void MakeClean() { this->m_controlDirtyState = 0; }
  void SetActive(bool active) { this->m_active = active; }
  void Process(unsigned int time, CDirtyRegionList& dirty) override
  {
    ++processCount;
    Window::Process(time, dirty);
  }
  unsigned int processCount{0};
  std::vector<CRect> scissors;
};

class CRotatedTextControl : public CGUIControl
{
public:
  explicit CRotatedTextControl(CTestFont& font)
    : CGUIControl(0, 0, 0, 0, TARGET_SIZE, TARGET_SIZE),
      m_font(font)
  {
  }
  CRotatedTextControl* Clone() const override { return new CRotatedTextControl(*this); }
  bool Animate(unsigned int) override
  {
    m_transform.SetZRotation(ANGLE, 64, 64);
    return true;
  }
  void Render() override
  {
    auto& context = CServiceBroker::GetWinSystem()->GetGfxContext();
    if (context.GetRenderOrder() == RENDER_ORDER_FRONT_TO_BACK)
      return;
    EXPECT_EQ(context.GetStencilLayer(), STENCIL_LAYER::NONE);
    auto* d3d = DX::DeviceResources::Get()->GetD3DContext();
    ComPtr<ID3D11RasterizerState> before, after;
    D3D11_RECT beforeRect{}, afterRect{};
    UINT count = 1;
    d3d->RSGetState(&before);
    d3d->RSGetScissorRects(&count, &beforeRect);
    m_font.Paint(context);
    EXPECT_FALSE(CServiceBroker::GetRenderSystem()->ScissorsCanEffectClipping());
    d3d->RSGetState(&after);
    count = 1;
    d3d->RSGetScissorRects(&count, &afterRect);
    EXPECT_EQ(before.Get(), after.Get());
    EXPECT_EQ(beforeRect.left, afterRect.left);
    EXPECT_EQ(beforeRect.top, afterRect.top);
    EXPECT_EQ(beforeRect.right, afterRect.right);
    EXPECT_EQ(beforeRect.bottom, afterRect.bottom);
  }

private:
  CTestFont& m_font;
};

class TestGUIControlGroupMaskD3D : public testing::TestWithParam<bool>
{
protected:
  void SetUp() override
  {
    m_previousWinSystem = CServiceBroker::GetWinSystem();
    m_previousGUI = CServiceBroker::GetGUI();
    m_window = std::make_unique<CTestWinSystem>();
    CServiceBroker::RegisterWinSystem(m_window.get());
    m_window->frontToBack = GetParam();
    m_hwnd = CreateWindowExW(0, L"STATIC", L"Kodi mask test", WS_POPUP, 0, 0, TARGET_SIZE,
                             TARGET_SIZE, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!m_hwnd)
    {
      GTEST_SKIP() << "D3D11 masking tests require a window: CreateWindowExW failed with error "
                   << GetLastError();
    }
    m_device = DX::DeviceResources::Get();
    m_device->SetWindow(m_hwnd);
    if (!m_device->HasValidDevice())
    {
      GTEST_SKIP() << "D3D11 masking tests require a usable D3D11 device";
    }
    m_device->SetLogicalSize(TARGET_SIZE, TARGET_SIZE);
    if (!m_device->GetSwapChain())
      m_device->ResizeBuffers();
    if (!m_device->GetSwapChain())
    {
      GTEST_SKIP() << "D3D11 masking tests require a windowed DXGI swap chain; "
                      "none is available in this session";
    }
    m_device->CreateBackBuffer();
    if (!m_device->GetBackBuffer().Get() || !m_device->GetBackBuffer().GetRenderTarget())
    {
      GTEST_SKIP() << "D3D11 masking tests require a usable backbuffer render target";
    }
    if (!m_device->GetDSV())
    {
      GTEST_SKIP() << "D3D11 masking tests require a depth/stencil attachment";
    }
    ASSERT_TRUE(m_window->InitRenderSystem());
    ASSERT_TRUE(m_window->BeginRender());
    m_gui = std::make_unique<CGUIComponent>();
    CServiceBroker::RegisterGUI(m_gui.get());
    SetProjection();
    Context().SetScissors(SCREEN);
    Context().SetClipRegion(0, 0, TARGET_SIZE, TARGET_SIZE);
    Context().SetRenderOrder(GetParam() ? RENDER_ORDER_BACK_TO_FRONT
                                        : RENDER_ORDER_ALL_BACK_TO_FRONT);
    ASSERT_TRUE(m_window->ClearBuffers(0));
    RecordProperty("adapter",
                   KODI::PLATFORM::WINDOWS::FromW(m_device->GetAdapterDesc().Description));
    RecordProperty("feature_level", m_device->GetDeviceFeatureLevel());
  }

  void TearDown() override
  {
    m_gui.reset();
    if (m_previousGUI)
      CServiceBroker::RegisterGUI(m_previousGUI);
    if (m_device)
      m_device->RegisterDeviceNotify(nullptr);
    m_window->DestroyRenderSystem();
    CGUITexture::Register({}, {}, {});
    m_window.reset();
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
    if (m_hwnd)
      DestroyWindow(m_hwnd);
  }

  void SetProjection()
  {
    m_window->SetViewPort(SCREEN);
    m_window->GetGUIShader()->SetWVP(
        XMMatrixIdentity(), XMMatrixIdentity(),
        XMMatrixOrthographicOffCenterLH(0, TARGET_SIZE, TARGET_SIZE, 0, 0, 1));
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
  void Paint() { CGUITexture::DrawQuad(SCREEN, 0xffffffff); }
  Pixels ReadPixels()
  {
    Pixels pixels{};
    m_device->FinishCommandList();
    D3D11_TEXTURE2D_DESC desc;
    m_device->GetBackBuffer().Get()->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_B8G8R8A8_UNORM);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    EXPECT_HRESULT_SUCCEEDED(m_device->GetD3DDevice()->CreateTexture2D(&desc, nullptr, &staging));
    auto* immediate = m_device->GetImmediateContext();
    immediate->CopyResource(staging.Get(), m_device->GetBackBuffer().Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(immediate->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
      for (int y = 0; y < TARGET_SIZE; ++y)
        std::memcpy(&pixels[y * TARGET_SIZE],
                    static_cast<unsigned char*>(mapped.pData) + y * mapped.RowPitch,
                    TARGET_SIZE * 4);
      immediate->Unmap(staging.Get(), 0);
    }
    else
      ADD_FAILURE() << "D3D11 pixel readback failed";
    return pixels;
  }
  void ExpectMask(const Pixels& pixels, const CRect& rect = MASK, float angle = ANGLE)
  {
    unsigned int wrong = 0;
    for (int y = 0; y < TARGET_SIZE; ++y)
      for (int x = 0; x < TARGET_SIZE; ++x)
        wrong += pixels[y * TARGET_SIZE + x][0] != (InsideMask(x, y, rect, angle) ? 255 : 0);
    EXPECT_EQ(wrong, 0u);
    RecordProperty("pixel_mismatches", wrong);
  }

  std::unique_ptr<CTestWinSystem> m_window;
  std::shared_ptr<DX::DeviceResources> m_device;
  std::unique_ptr<CGUIComponent> m_gui;
  CGUIComponent* m_previousGUI{nullptr};
  CWinSystemBase* m_previousWinSystem{nullptr};
  HWND m_hwnd{nullptr};
};

class TestGUIControlGroupMaskD3DWindows : public TestGUIControlGroupMaskD3D
{
protected:
  void SetUp() override
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    m_previousSmartRedraw = settings->m_guiSmartRedraw;
    m_previousFrontToBack = settings->m_guiFrontToBackRendering;
    m_previousDirtyRegions = settings->m_guiAlgorithmDirtyRegions;
    m_previousVisualize = settings->m_guiVisualizeDirtyRegions;
    settings->m_guiFrontToBackRendering = GetParam();
    settings->m_guiAlgorithmDirtyRegions = DIRTYREGION_SOLVER_COST_REDUCTION;
    settings->m_guiVisualizeDirtyRegions = false;
    m_previousMessenger = CServiceBroker::GetAppMessenger();
    auto messenger = std::make_shared<KODI::MESSAGING::CApplicationMessenger>();
    messenger->SetProcessThread(std::this_thread::get_id());
    CServiceBroker::RegisterAppMessenger(messenger);
    TestGUIControlGroupMaskD3D::SetUp();
    if (!HasFatalFailure() && !IsSkipped())
      m_gui->GetWindowManager().Initialize();
  }
  void TearDown() override
  {
    TestGUIControlGroupMaskD3D::TearDown();
    auto settings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    settings->m_guiSmartRedraw = m_previousSmartRedraw;
    settings->m_guiFrontToBackRendering = m_previousFrontToBack;
    settings->m_guiAlgorithmDirtyRegions = m_previousDirtyRegions;
    settings->m_guiVisualizeDirtyRegions = m_previousVisualize;
    CServiceBroker::RegisterAppMessenger(m_previousMessenger);
  }

private:
  bool m_previousSmartRedraw{false};
  bool m_previousFrontToBack{false};
  int m_previousDirtyRegions{0};
  bool m_previousVisualize{false};
  std::shared_ptr<KODI::MESSAGING::CApplicationMessenger> m_previousMessenger;
};
} // namespace

TEST_P(TestGUIControlGroupMaskD3DWindows, CleanWindowAndOverlappingDialog)
{
  auto& manager = m_gui->GetWindowManager();
  const auto expectPixels = [](const Pixels& pixels, bool dialogVisible, float dialogAngle)
  {
    unsigned int wrong = 0;
    for (int y = 0; y < TARGET_SIZE; ++y)
      for (int x = 0; x < TARGET_SIZE; ++x)
      {
        const auto& pixel = pixels[y * TARGET_SIZE + x];
        const bool inWindow = InsideMask(x, y);
        const bool inDialog = dialogVisible && InsideMask(x, y, MASK, dialogAngle);
        wrong += pixel[0] != (inWindow && !inDialog ? 255 : 0) || pixel[1] != 0 ||
                 pixel[2] != (inDialog ? 255 : 0);
      }
    EXPECT_EQ(wrong, 0u);
  };
  for (const auto [smartRedraw, bufferAge] :
       {std::pair{false, 0}, {true, 0}, {false, 1}, {true, 1}})
  {
    SCOPED_TRACE(smartRedraw);
    SCOPED_TRACE(bufferAge);
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiSmartRedraw = smartRedraw;
    m_window->bufferAge = 0;
    Context().SetScissors(SCREEN);
    auto* window = new CMaskWindow<CGUIWindow>(5000);
    auto* windowMask = new CRotatingMask;
    auto* windowPaint = new CPaintControl(64);
    windowPaint->color = 0xff0000ff;
    windowMask->AddControl(windowPaint);
    window->AddControl(windowMask);
    ASSERT_TRUE(manager.Add(window));
    manager.ActivateWindow(window->GetID());
    ASSERT_EQ(manager.GetActiveWindow(), window->GetID());
    manager.Process(0);
    // Age the initial damage out of the buffer-age history without processing the window again.
    for (int frame = 0; frame < 3; ++frame)
      ASSERT_TRUE(manager.Render());
    expectPixels(ReadPixels(), false, 0);

    auto* dialog = new CMaskWindow<CGUIDialog>(5001);
    auto* dialogMask = new CRotatingMask;
    dialogMask->angle = -ANGLE;
    auto* dialogPaint = new CPaintControl(64);
    dialogPaint->color = 0xffff0000;
    dialogMask->AddControl(dialogPaint);
    dialog->AddControl(dialogMask);
    ASSERT_TRUE(manager.Add(dialog));
    manager.RegisterDialog(dialog);
    window->MakeClean();
    ASSERT_FALSE(window->IsControlDirty());
    const auto processed = window->processCount;
    windowPaint->layers.clear();
    window->scissors.clear();
    manager.Process(1);
    EXPECT_EQ(window->processCount, processed + (smartRedraw ? 0 : 1));
    EXPECT_EQ(dialog->processCount, 1u);

    m_window->bufferAge = bufferAge;
    ASSERT_TRUE(manager.Render());
    ASSERT_EQ(window->scissors.size(), 1u);
    if (bufferAge)
      EXPECT_LT(window->scissors[0].Area(), SCREEN.Area());
    else
      EXPECT_EQ(window->scissors[0], SCREEN);
    const auto windowLayer = GetParam() ? STENCIL_LAYER::LAYER1 : STENCIL_LAYER::LAYER0;
    const auto dialogLayer = GetParam() ? STENCIL_LAYER::LAYER0 : STENCIL_LAYER::LAYER1;
    EXPECT_EQ(windowPaint->layers, std::vector<STENCIL_LAYER>(GetParam() ? 2 : 1, windowLayer));
    EXPECT_EQ(dialogPaint->layers, std::vector<STENCIL_LAYER>(GetParam() ? 2 : 1, dialogLayer));
    EXPECT_EQ(Context().AddStencilRectToBucket(SCREEN), STENCIL_LAYER::LAYER2);
    const auto before = ReadPixels();
    expectPixels(before, true, -ANGLE);

    m_window->bufferAge = 0;
    for (int frame = 0; frame < 3; ++frame)
      ASSERT_TRUE(manager.Render());
    m_window->bufferAge = 1;
    const std::vector<CRect> damage{{16, 32, 40, 96}, {88, 32, 112, 96}};
    for (const auto& rect : damage)
      manager.MarkDirty(rect);
    window->MakeClean();
    dialogPaint->color = 0xff00ff00;
    window->scissors.clear();
    windowPaint->layers.clear();
    dialogPaint->layers.clear();
    ASSERT_TRUE(manager.Render());
    EXPECT_EQ(window->scissors, damage);
    EXPECT_EQ(windowPaint->layers, std::vector<STENCIL_LAYER>(GetParam() ? 4 : 2, windowLayer));
    EXPECT_EQ(dialogPaint->layers, std::vector<STENCIL_LAYER>(GetParam() ? 4 : 2, dialogLayer));
    EXPECT_EQ(Context().AddStencilRectToBucket(SCREEN), STENCIL_LAYER::LAYER2);
    EXPECT_EQ(Context().GetStencilLayer(), STENCIL_LAYER::NONE);
    auto expected = before;
    for (int y = 0; y < TARGET_SIZE; ++y)
      for (int x = 0; x < TARGET_SIZE; ++x)
        if (damage[0].PtInRect(CPoint(x + 0.5f, y + 0.5f)) ||
            damage[1].PtInRect(CPoint(x + 0.5f, y + 0.5f)))
        {
          const bool inDialog = InsideMask(x, y, MASK, -ANGLE);
          auto& pixel = expected[y * TARGET_SIZE + x];
          pixel[0] = InsideMask(x, y) && !inDialog ? 255 : 0;
          pixel[1] = inDialog ? 255 : 0;
          pixel[2] = 0;
        }
    EXPECT_EQ(ReadPixels(), expected);

    m_window->bufferAge = 0;
    dialogPaint->color = 0xffff0000;
    for (const auto [visible, angle] : {std::pair{false, -ANGLE}, {true, 0.0f}, {true, -ANGLE}})
    {
      SCOPED_TRACE(visible);
      SCOPED_TRACE(angle);
      dialog->SetActive(visible);
      dialogMask->angle = angle;
      dialog->MarkDirtyRegion();
      window->MakeClean();
      dialogPaint->layers.clear();
      manager.Process(2);
      ASSERT_TRUE(manager.Render());
      expectPixels(ReadPixels(), visible, angle);
      if (visible)
        EXPECT_EQ(dialogPaint->layers,
                  std::vector<STENCIL_LAYER>(GetParam() ? 2 : 1,
                                             angle == 0 ? STENCIL_LAYER::NONE : dialogLayer));
      else
        EXPECT_TRUE(dialogPaint->layers.empty());
      EXPECT_EQ(Context().GetStencilLayer(), STENCIL_LAYER::NONE);
    }

    manager.Remove(dialog->GetID());
    manager.Remove(window->GetID());
  }
}

TEST_P(TestGUIControlGroupMaskD3DWindows, DisjointNestedMasksRecoverFromExhaustion)
{
  auto& manager = m_gui->GetWindowManager();
  auto* window = new CMaskWindow<CGUIWindow>(5000);
  ASSERT_TRUE(manager.Add(window));
  manager.ActivateWindow(window->GetID());
  for (int depth : {2, 9, 2})
  {
    SCOPED_TRACE(depth);
    window->ClearAll();
    std::array<CPaintControl*, 2> paints{};
    for (int side = 0; side < 2; ++side)
    {
      auto* outer = new CRotatingMask;
      const float position = side == 0 ? 24 : 88;
      outer->SetPosition(position, position);
      outer->SetWidth(16);
      outer->SetHeight(16);
      window->AddControl(outer);
      CGUIControlGroupMask* inner = outer;
      for (int level = 1; side == 0 && level < depth; ++level)
      {
        auto* child = new CGUIControlGroupMask(0, 0, 0, 0, 16, 16);
        inner->AddControl(child);
        inner = child;
      }
      paints[side] = new CPaintControl(16);
      inner->AddControl(paints[side]);
    }
    window->MarkDirtyRegion();
    manager.Process(0);
    ASSERT_TRUE(manager.Render());
    if (depth == 9)
      EXPECT_TRUE(paints[0]->layers.empty());
    else
      EXPECT_EQ(paints[0]->layers,
                std::vector<STENCIL_LAYER>(GetParam() ? 2 : 1, STENCIL_LAYER::LAYER1));
    EXPECT_EQ(paints[1]->layers,
              std::vector<STENCIL_LAYER>(GetParam() ? 2 : 1, STENCIL_LAYER::LAYER0));
    EXPECT_EQ(Context().GetStencilLayer(), STENCIL_LAYER::NONE);
    EXPECT_EQ(Context().AddStencilRectToBucket(SCREEN),
              depth == 9 ? STENCIL_LAYER::INVALID : STENCIL_LAYER::LAYER2);
    const auto pixels = ReadPixels();
    unsigned int wrong = 0;
    for (int y = 0; y < TARGET_SIZE; ++y)
      for (int x = 0; x < TARGET_SIZE; ++x)
      {
        const bool inside = (depth != 9 && InsideMask(x, y, CRect(24, 24, 40, 40))) ||
                            InsideMask(x, y, CRect(88, 88, 104, 104));
        for (int channel = 0; channel < 3; ++channel)
          wrong += pixels[y * TARGET_SIZE + x][channel] != (inside ? 255 : 0);
      }
    EXPECT_EQ(wrong, 0u);
  }
}

TEST_P(TestGUIControlGroupMaskD3D, UnmaskedQuad)
{
  Paint();
  ExpectMask(ReadPixels(), SCREEN, 0);
}

TEST_P(TestGUIControlGroupMaskD3D, RotatedQuad)
{
  ComPtr<ID3D11DepthStencilState> before;
  UINT beforeReference;
  m_device->GetD3DContext()->OMGetDepthStencilState(&before, &beforeReference);
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  Context().RestoreStencilLayer();
  ExpectMask(ReadPixels());
  ComPtr<ID3D11DepthStencilState> after;
  UINT afterReference;
  m_device->GetD3DContext()->OMGetDepthStencilState(&after, &afterReference);
  EXPECT_EQ(before.Get(), after.Get());
  EXPECT_EQ(beforeReference, afterReference);
}

TEST_P(TestGUIControlGroupMaskD3D, RotatedTexture)
{
  CTestTexture texture;
  if (GetParam())
    Context().SetRenderOrder(RENDER_ORDER_FRONT_TO_BACK);
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  texture.Paint();
  Context().RestoreStencilLayer();
  ExpectMask(ReadPixels());
  if (GetParam())
  {
    Context().SetRenderOrder(RENDER_ORDER_BACK_TO_FRONT);
    CGUITextureD3D::DrawQuad(SCREEN, 0xffff0000, nullptr, nullptr, -0.5f);
    const auto pixels = ReadPixels();
    ExpectMask(pixels);
    for (const auto& pixel : pixels)
      ASSERT_EQ(pixel[2], 255);
  }
}

TEST_P(TestGUIControlGroupMaskD3D, NestedMasksPreserveOtherBits)
{
  const CRect inner{64, 32, 112, 96};
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Mask(inner, STENCIL_LAYER::LAYER7);
  Context().RestoreStencilLayer();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER7);
  Paint();
  Context().RestoreStencilLayer();
  ExpectMask(ReadPixels(), CRect(64, 32, 96, 96));
  const float black[]{0, 0, 0, 0};
  m_device->GetD3DContext()->ClearRenderTargetView(m_device->GetBackBuffer().GetRenderTarget(),
                                                   black);
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  Context().RestoreStencilLayer();
  ExpectMask(ReadPixels());
}

TEST_P(TestGUIControlGroupMaskD3D, TextUsesTheSameStencilMask)
{
  CTestFont font;
  ASSERT_TRUE(font.Load(XBMC_REF_FILE_PATH("media/Fonts/arial.ttf"), 32));
  font.Paint(Context());
  const auto unmasked = ReadPixels();
  ASSERT_TRUE(m_window->ClearBuffers(0));
  Mask();
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  font.Paint(Context());
  Context().RestoreStencilLayer();
  const auto masked = ReadPixels();
  unsigned int wrong = 0;
  unsigned int visible = 0;
  unsigned int clipped = 0;
  for (int y = 0; y < TARGET_SIZE; ++y)
    for (int x = 0; x < TARGET_SIZE; ++x)
    {
      const size_t i = y * TARGET_SIZE + x;
      const bool inside = InsideMask(x, y);
      wrong += masked[i] != (inside ? unmasked[i] : Pixels::value_type{});
      visible += inside && masked[i][0] != 0;
      clipped += !inside && unmasked[i][0] != 0;
    }
  EXPECT_EQ(wrong, 0u);
  EXPECT_GT(visible, 0u);
  EXPECT_GT(clipped, 0u);
}

TEST_P(TestGUIControlGroupMaskD3D, RotatedTextPreservesUnrotatedMask)
{
  m_window->SetCameraPosition(CPoint(64, 64), TARGET_SIZE, TARGET_SIZE, 0);
  CTestFont font;
  ASSERT_TRUE(font.Load(XBMC_REF_FILE_PATH("media/Fonts/arial.ttf"), 32));
  auto* text = new CRotatedTextControl(font);
  CGUIControlGroupMask group(0, 0, 0, 0, 64, 96);
  group.AddControl(text);
  CDirtyRegionList dirty;
  group.DoProcess(0, dirty);
  text->DoRender();
  const auto unmasked = ReadPixels();
  ASSERT_TRUE(m_window->ClearBuffers(0));
  if (GetParam())
  {
    Context().SetRenderOrder(RENDER_ORDER_FRONT_TO_BACK);
    group.DoRender();
    Context().SetRenderOrder(RENDER_ORDER_BACK_TO_FRONT);
  }
  group.DoRender();
  EXPECT_EQ(Context().GetScissors(), SCREEN);
  const auto masked = ReadPixels();
  unsigned int outside = 0;
  unsigned int visible = 0;
  unsigned int clipped = 0;
  unsigned int changedInside = 0;
  for (int y = 0; y < TARGET_SIZE; ++y)
    for (int x = 0; x < TARGET_SIZE; ++x)
    {
      const size_t i = y * TARGET_SIZE + x;
      const bool inside = x < 64 && y < 96;
      outside += !inside && masked[i] != Pixels::value_type{};
      visible += inside && masked[i][0] != 0;
      clipped += !inside && unmasked[i][0] != 0;
      changedInside += inside && masked[i] != unmasked[i];
    }
  EXPECT_EQ(outside, 0u);
  EXPECT_EQ(changedInside, 0u);
  EXPECT_GT(visible, 0u);
  EXPECT_GT(clipped, 0u);
  RecordProperty("outside_mask_pixels", outside);
  RecordProperty("visible_text_pixels", visible);
  RecordProperty("inside_mask_mismatches", changedInside);
}

TEST_P(TestGUIControlGroupMaskD3D, TextClipRegionWithAndWithoutRotation)
{
  m_window->SetCameraPosition(CPoint(64, 64), TARGET_SIZE, TARGET_SIZE, 0);
  CTestFont font;
  ASSERT_TRUE(font.Load(XBMC_REF_FILE_PATH("media/Fonts/arial.ttf"), 32));
  const CRect clip{32, 0, 96, TARGET_SIZE};
  for (float angle : {0.0f, ANGLE})
  {
    SCOPED_TRACE(angle);
    Context().SetScissors(SCREEN);
    ASSERT_TRUE(m_window->ClearBuffers(0));
    TransformMatrix transform;
    transform.SetZRotation(angle, 64, 64);
    Context().AddTransform(transform);
    // Keep the rotated text origin on the pixel grid.
    font.Paint(Context(), 47);
    EXPECT_EQ(m_window->ScissorsCanEffectClipping(), angle == 0);
    const auto unmasked = ReadPixels();
    ASSERT_TRUE(m_window->ClearBuffers(0));
    ASSERT_TRUE(Context().SetClipRegion(clip.x1, clip.y1, clip.Width(), clip.Height()));
    font.Paint(Context(), 47);
    Context().RestoreClipRegion();
    Context().RemoveTransform();
    const auto clipped = ReadPixels();
    unsigned int wrong = 0;
    unsigned int visible = 0;
    unsigned int removed = 0;
    for (int y = 0; y < TARGET_SIZE; ++y)
      for (int x = 0; x < TARGET_SIZE; ++x)
      {
        const size_t i = y * TARGET_SIZE + x;
        const bool inside = InsideMask(x, y, clip, angle);
        bool mismatch = false;
        for (size_t channel = 0; channel < 4; ++channel)
        {
          // Clipping glyph vertices can change interpolation rounding by one channel value.
          const int expected = inside ? unmasked[i][channel] : 0;
          mismatch |= std::abs(clipped[i][channel] - expected) > (inside ? 1 : 0);
        }
        wrong += mismatch;
        visible += inside && clipped[i][0] != 0;
        removed += !inside && unmasked[i][0] != 0;
      }
    EXPECT_EQ(wrong, 0u);
    EXPECT_GT(visible, 0u);
    EXPECT_GT(removed, 0u);
    RecordProperty(angle == 0 ? "scissor_clip_mismatches" : "shader_clip_mismatches", wrong);
  }
}

TEST_P(TestGUIControlGroupMaskD3D, RotationStartsAndStopsAcrossFrames)
{
  CRotatingMask group;
  group.AddControl(new CPaintControl);
  CDirtyRegionList dirty;
  for (float angle : {0.0f, ANGLE, 0.0f, -ANGLE})
  {
    SCOPED_TRACE(angle);
    ASSERT_TRUE(m_window->ClearBuffers(0));
    Context().ClearStencilBuckets();
    group.angle = angle;
    group.DoProcess(0, dirty);
    Context().SetRenderOrder(GetParam() ? RENDER_ORDER_FRONT_TO_BACK
                                        : RENDER_ORDER_ALL_BACK_TO_FRONT);
    group.DoRender();
    if (GetParam())
    {
      Context().SetRenderOrder(RENDER_ORDER_BACK_TO_FRONT);
      group.DoRender();
    }
    ExpectMask(ReadPixels(), MASK, angle);
    EXPECT_EQ(Context().GetStencilLayer(), STENCIL_LAYER::NONE);
  }
}

TEST_P(TestGUIControlGroupMaskD3D, ClearStencilWithoutChangingSinglePassDepth)
{
  for (int clear : {0, 1, 2})
  {
    SCOPED_TRACE(clear);
    m_device->GetD3DContext()->ClearDepthStencilView(
        m_device->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.75f, 0xff);
    const float black[]{0, 0, 0, 0};
    m_device->GetD3DContext()->ClearRenderTargetView(m_device->GetBackBuffer().GetRenderTarget(),
                                                     black);
    if (clear == 0)
      ASSERT_TRUE(m_window->ClearBuffers(0));
    else
    {
      m_window->clearFunction =
          clear == 1 ? ClearFunction::FIXED_FUNCTION : ClearFunction::GEOMETRY;
      m_window->InvalidateColorBuffer();
    }
    Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
    Paint();
    Context().RestoreStencilLayer();
    EXPECT_EQ(ReadPixels(), Pixels{});
    m_window->SetDepthCulling(DepthCulling::BACK_TO_FRONT);
    CGUITextureD3D::DrawQuad(SCREEN, 0xffffffff, nullptr, nullptr, 0);
    const auto pixels = ReadPixels();
    EXPECT_EQ(pixels[64 * TARGET_SIZE + 64][0], GetParam() ? 255 : 0);
  }
}

TEST_P(TestGUIControlGroupMaskD3D, RestoresPipelineStateAndDoesNotWriteColorOrDepth)
{
  auto* context = m_device->GetD3DContext();
  context->ClearDepthStencilView(m_device->GetDSV(), D3D11_CLEAR_DEPTH, 0, 0);
  CD3D11_DEPTH_STENCIL_DESC desc(D3D11_DEFAULT);
  desc.DepthFunc = D3D11_COMPARISON_NEVER;
  desc.StencilEnable = true;
  desc.StencilReadMask = 0x12;
  desc.StencilWriteMask = 0x24;
  desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
  ComPtr<ID3D11DepthStencilState> depthState;
  ASSERT_HRESULT_SUCCEEDED(m_device->GetD3DDevice()->CreateDepthStencilState(&desc, &depthState));
  context->OMSetDepthStencilState(depthState.Get(), 7);
  m_window->SetStereoMode(RenderStereoMode::ANAGLYPH_RED_CYAN, RenderStereoView::LEFT);
  ComPtr<ID3D11BlendState> blendState;
  context->OMGetBlendState(&blendState, nullptr, nullptr);
  const float factors[]{0.1f, 0.2f, 0.3f, 0.4f};
  context->OMSetBlendState(blendState.Get(), factors, 0x12345678);
  auto* shader = m_window->GetGUIShader();
  shader->Begin(SHADER_METHOD_RENDER_TEXTURE_BLEND);
  const XMMATRIX world = XMMatrixTranslation(2, 3, 0);
  shader->SetWorld(world);
  ComPtr<ID3D11PixelShader> pixelShader;
  context->PSGetShader(&pixelShader, nullptr, nullptr);
  Mask();
  ComPtr<ID3D11DepthStencilState> restoredDepth;
  UINT reference;
  context->OMGetDepthStencilState(&restoredDepth, &reference);
  EXPECT_EQ(restoredDepth.Get(), depthState.Get());
  EXPECT_EQ(reference, 7u);
  ComPtr<ID3D11BlendState> restoredBlend;
  float restoredFactors[4];
  UINT sampleMask;
  context->OMGetBlendState(&restoredBlend, restoredFactors, &sampleMask);
  EXPECT_EQ(restoredBlend.Get(), blendState.Get());
  EXPECT_EQ(sampleMask, 0x12345678u);
  for (size_t i = 0; i < 4; ++i)
    EXPECT_EQ(restoredFactors[i], factors[i]);
  ComPtr<ID3D11PixelShader> restoredShader;
  context->PSGetShader(&restoredShader, nullptr, nullptr);
  EXPECT_EQ(restoredShader.Get(), pixelShader.Get());
  for (size_t i = 0; i < 4; ++i)
    EXPECT_TRUE(XMVector4Equal(shader->GetWorld().r[i], world.r[i]));
  EXPECT_EQ(ReadPixels(), Pixels{});

  m_window->SetStereoMode(RenderStereoMode::OFF, RenderStereoView::OFF);
  shader->SetWorld(XMMatrixIdentity());
  m_window->SetDepthCulling(DepthCulling::FRONT_TO_BACK);
  CGUITextureD3D::DrawQuad(SCREEN, 0xffffffff, nullptr, nullptr, -0.5f);
  ExpectMask(ReadPixels(), SCREEN, 0);
}

TEST_P(TestGUIControlGroupMaskD3D, InterlacedRightEyeClearsStencilOnly)
{
  Paint();
  const auto before = ReadPixels();
  m_device->GetD3DContext()->ClearDepthStencilView(
      m_device->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.75f, 0xff);
  m_window->CRenderSystemBase::SetStereoMode(RenderStereoMode::INTERLACED, RenderStereoView::RIGHT);
  m_window->InvalidateColorBuffer();
  EXPECT_EQ(ReadPixels(), before);
  const float black[]{0, 0, 0, 0};
  m_device->GetD3DContext()->ClearRenderTargetView(m_device->GetBackBuffer().GetRenderTarget(),
                                                   black);
  m_window->SetDepthCulling(DepthCulling::OFF);
  Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
  Paint();
  Context().RestoreStencilLayer();
  EXPECT_EQ(ReadPixels(), Pixels{});
  m_window->SetDepthCulling(DepthCulling::BACK_TO_FRONT);
  CGUITextureD3D::DrawQuad(SCREEN, 0xffffffff, nullptr, nullptr, 0);
  EXPECT_EQ(ReadPixels(), Pixels{});
}

TEST_P(TestGUIControlGroupMaskD3D, DeviceRecreation)
{
  for (int iteration = 0; iteration < 2; ++iteration)
  {
    if (iteration != 0)
    {
      m_device->HandleDeviceLost(false);
      ASSERT_TRUE(m_window->BeginRender());
      SetProjection();
      Context().SetRenderOrder(GetParam() ? RENDER_ORDER_BACK_TO_FRONT
                                          : RENDER_ORDER_ALL_BACK_TO_FRONT);
    }
    ASSERT_TRUE(m_window->ClearBuffers(0));
    Mask();
    Context().SetStencilLayer(STENCIL_LAYER::LAYER0);
    Paint();
    Context().RestoreStencilLayer();
    ExpectMask(ReadPixels());
  }
}

INSTANTIATE_TEST_SUITE_P(RenderOrders, TestGUIControlGroupMaskD3D, testing::Bool());
INSTANTIATE_TEST_SUITE_P(RenderOrders, TestGUIControlGroupMaskD3DWindows, testing::Bool());

#endif
