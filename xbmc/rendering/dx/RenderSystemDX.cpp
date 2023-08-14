/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "RenderSystemDX.h"

#include "application/Application.h"

#include <mutex>
#if defined(TARGET_WINDOWS_DESKTOP)
#include "cores/RetroPlayer/process/windows/RPProcessInfoWin.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPWinRenderer.h"
#endif
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DXVA.h"
#if defined(TARGET_WINDOWS_STORE)
#include "cores/VideoPlayer/Process/windows/ProcessInfoWin10.h"
#else
#include "cores/VideoPlayer/Process/windows/ProcessInfoWin.h"
#endif
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/WinRenderer.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "guilib/D3DResource.h"
#include "guilib/GUIShaderDX.h"
#include "guilib/GUITextureD3D.h"
#include "guilib/GUIWindowManager.h"
#include "utils/MathUtils.h"
#include "utils/log.h"

#include <DirectXPackedVector.h>

extern "C" {
#include <libavutil/pixfmt.h>
}

using namespace KODI;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;
using namespace std::chrono_literals;

CRenderSystemDX::CRenderSystemDX() : CRenderSystemBase()
{
  m_bVSync = true;

  memset(&m_viewPort, 0, sizeof m_viewPort);
  memset(&m_scissor, 0, sizeof m_scissor);
}

CRenderSystemDX::~CRenderSystemDX() = default;

bool CRenderSystemDX::InitRenderSystem()
{
  m_bVSync = true;

  // check various device capabilities
  CheckDeviceCaps();

  if (!CreateStates() || !InitGUIShader())
    return false;

  m_bRenderCreated = true;
  m_deviceResources->RegisterDeviceNotify(this);

  // register platform dependent objects
#if defined(TARGET_WINDOWS_STORE)
  VIDEOPLAYER::CProcessInfoWin10::Register();
#else
  VIDEOPLAYER::CProcessInfoWin::Register();
#endif
  CDVDFactoryCodec::ClearHWAccels();
  DXVA::CDecoder::Register();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CWinRenderer::Register();
#if defined(TARGET_WINDOWS_DESKTOP)
  RETRO::CRPProcessInfoWin::Register();
  RETRO::CRPProcessInfoWin::RegisterRendererFactory(new RETRO::CWinRendererFactory);
#endif
  m_viewPort = m_deviceResources->GetScreenViewport();
  RestoreViewPort();

  auto outputSize = m_deviceResources->GetOutputSize();
  // set camera to center of screen
  CPoint camPoint = { outputSize.Width * 0.5f, outputSize.Height * 0.5f };
  SetCameraPosition(camPoint, outputSize.Width, outputSize.Height);

  const DXGI_ADAPTER_DESC AIdentifier = m_deviceResources->GetAdapterDesc();
  m_RenderRenderer = KODI::PLATFORM::WINDOWS::FromW(AIdentifier.Description);
  uint32_t version = 0;
  for (uint32_t decimal = m_deviceResources->GetDeviceFeatureLevel() >> 8, round = 0; decimal > 0; decimal >>= 4, ++round)
    version += (decimal % 16) * std::pow(10, round);
  m_RenderVersion = StringUtils::Format("{:.1f}", static_cast<float>(version) / 10.0f);

  CGUITextureD3D::Register();

  return true;
}

void CRenderSystemDX::OnResize()
{
  if (!m_bRenderCreated)
    return;

  auto outputSize = m_deviceResources->GetOutputSize();

  // set camera to center of screen
  CPoint camPoint = { outputSize.Width * 0.5f, outputSize.Height * 0.5f };
  SetCameraPosition(camPoint, outputSize.Width, outputSize.Height);

  CheckInterlacedStereoView();
}

bool CRenderSystemDX::IsFormatSupport(DXGI_FORMAT format, unsigned int usage) const
{
  ComPtr<ID3D11Device1> pD3DDev = m_deviceResources->GetD3DDevice();
  UINT supported;
  pD3DDev->CheckFormatSupport(format, &supported);
  return (supported & usage) != 0;
}

bool CRenderSystemDX::DestroyRenderSystem()
{
  std::unique_lock<CCriticalSection> lock(m_resourceSection);

  if (m_pGUIShader)
  {
    m_pGUIShader->End();
    delete m_pGUIShader;
    m_pGUIShader = nullptr;
  }
  m_rightEyeTex.Release();
  m_BlendEnableState = nullptr;
  m_BlendDisableState = nullptr;
  m_RSScissorDisable = nullptr;
  m_RSScissorEnable = nullptr;
  m_depthStencilState = nullptr;

  return true;
}

void CRenderSystemDX::CheckInterlacedStereoView()
{
  RENDER_STEREO_MODE stereoMode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();

  if ( m_rightEyeTex.Get()
    && RENDER_STEREO_MODE_INTERLACED    != stereoMode
    && RENDER_STEREO_MODE_CHECKERBOARD  != stereoMode)
  {
    m_rightEyeTex.Release();
  }

  if ( !m_rightEyeTex.Get()
    && ( RENDER_STEREO_MODE_INTERLACED   == stereoMode
      || RENDER_STEREO_MODE_CHECKERBOARD == stereoMode))
  {
    const auto outputSize = m_deviceResources->GetOutputSize();
    DXGI_FORMAT texFormat = m_deviceResources->GetBackBuffer().GetFormat();
    if (!m_rightEyeTex.Create(outputSize.Width, outputSize.Height, 1, D3D11_USAGE_DEFAULT, texFormat))
    {
      CLog::Log(LOGERROR, "{} - Failed to create right eye buffer.", __FUNCTION__);
      CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoMode(RENDER_STEREO_MODE_SPLIT_HORIZONTAL); // try fallback to split horizontal
    }
    else
      m_deviceResources->Unregister(&m_rightEyeTex); // we will handle its health
  }
}

bool CRenderSystemDX::CreateStates()
{
  auto m_pD3DDev = m_deviceResources->GetD3DDevice();
  auto m_pContext = m_deviceResources->GetD3DContext();

  if (!m_pD3DDev)
    return false;

  m_BlendEnableState = nullptr;
  m_BlendDisableState = nullptr;

  // Initialize the description of the stencil state.
  D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};

  // Set up the description of the stencil state.
  depthStencilDesc.DepthEnable = false;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
  depthStencilDesc.StencilEnable = false;
  depthStencilDesc.StencilReadMask = 0xFF;
  depthStencilDesc.StencilWriteMask = 0xFF;

  // Stencil operations if pixel is front-facing.
  depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Stencil operations if pixel is back-facing.
  depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Create the depth stencil state.
  HRESULT hr = m_pD3DDev->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
  if(FAILED(hr))
    return false;

  // Set the depth stencil state.
  m_pContext->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

  D3D11_RASTERIZER_DESC rasterizerState;
  rasterizerState.CullMode = D3D11_CULL_NONE;
  rasterizerState.FillMode = D3D11_FILL_SOLID;// DEBUG - D3D11_FILL_WIREFRAME
  rasterizerState.FrontCounterClockwise = false;
  rasterizerState.DepthBias = 0;
  rasterizerState.DepthBiasClamp = 0.0f;
  rasterizerState.DepthClipEnable = true;
  rasterizerState.SlopeScaledDepthBias = 0.0f;
  rasterizerState.ScissorEnable = false;
  rasterizerState.MultisampleEnable = false;
  rasterizerState.AntialiasedLineEnable = false;

  if (FAILED(m_pD3DDev->CreateRasterizerState(&rasterizerState, &m_RSScissorDisable)))
    return false;

  rasterizerState.ScissorEnable = true;
  if (FAILED(m_pD3DDev->CreateRasterizerState(&rasterizerState, &m_RSScissorEnable)))
    return false;

  m_pContext->RSSetState(m_RSScissorDisable.Get()); // by default

  D3D11_BLEND_DESC blendState = {};
  blendState.RenderTarget[0].BlendEnable = true;
  blendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // D3D11_BLEND_SRC_ALPHA;
  blendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // D3D11_BLEND_INV_SRC_ALPHA;
  blendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  blendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  m_pD3DDev->CreateBlendState(&blendState, &m_BlendEnableState);

  blendState.RenderTarget[0].BlendEnable = false;
  m_pD3DDev->CreateBlendState(&blendState, &m_BlendDisableState);

  // by default
  m_pContext->OMSetBlendState(m_BlendEnableState.Get(), nullptr, 0xFFFFFFFF);
  m_BlendEnabled = true;

  return true;
}

void CRenderSystemDX::PresentRender(bool rendered, bool videoLayer)
{
  if (!m_bRenderCreated)
    return;

  if ( rendered
    && ( m_stereoMode == RENDER_STEREO_MODE_INTERLACED
      || m_stereoMode == RENDER_STEREO_MODE_CHECKERBOARD))
  {
    auto m_pContext = m_deviceResources->GetD3DContext();

    // all views prepared, let's merge them before present
    m_pContext->OMSetRenderTargets(1, m_deviceResources->GetBackBuffer().GetAddressOfRTV(), m_deviceResources->GetDSV());

    auto outputSize = m_deviceResources->GetOutputSize();
    CRect destRect = { 0.0f, 0.0f, float(outputSize.Width), float(outputSize.Height) };

    SHADER_METHOD method = RENDER_STEREO_MODE_INTERLACED == m_stereoMode
                           ? SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT
                           : SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT;
    SetAlphaBlendEnable(true);
    CD3DTexture::DrawQuad(destRect, 0, &m_rightEyeTex, nullptr, method);
    CD3DHelper::PSClearShaderResources(m_pContext);
  }

  // time for decoder that may require the context
  {
    std::unique_lock<CCriticalSection> lock(m_decoderSection);
    XbmcThreads::EndTime<> timer;
    timer.Set(5ms);
    while (!m_decodingTimer.IsTimePast() && !timer.IsTimePast())
    {
      m_decodingEvent.wait(lock, 1ms);
    }
  }

  PresentRenderImpl(rendered);
}

void CRenderSystemDX::RequestDecodingTime()
{
  std::unique_lock<CCriticalSection> lock(m_decoderSection);
  m_decodingTimer.Set(3ms);
}

void CRenderSystemDX::ReleaseDecodingTime()
{
  std::unique_lock<CCriticalSection> lock(m_decoderSection);
  m_decodingTimer.SetExpired();
  m_decodingEvent.notify();
}

bool CRenderSystemDX::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  m_limitedColorRange = CServiceBroker::GetWinSystem()->UseLimitedColor();
  m_inScene = m_deviceResources->Begin();
  return m_inScene;
}

bool CRenderSystemDX::EndRender()
{
  m_inScene = false;

  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemDX::ClearBuffers(UTILS::COLOR::Color color)
{
  if (!m_bRenderCreated)
    return false;

  float fColor[4];
  CD3DHelper::XMStoreColor(fColor, color);
  ID3D11RenderTargetView* pRTView = m_deviceResources->GetBackBuffer().GetRenderTarget();

  if ( m_stereoMode != RENDER_STEREO_MODE_OFF
    && m_stereoMode != RENDER_STEREO_MODE_MONO)
  {
    // if stereo anaglyph/tab/sbs, data was cleared when left view was rendered
    if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    {
      // execute command's queue
      m_deviceResources->FinishCommandList();

      // do not clear RT for anaglyph modes
      if ( m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA
        || m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN
        || m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
      {
        pRTView = nullptr;
      }
      // for interlaced/checkerboard clear view for right texture
      else if (m_stereoMode == RENDER_STEREO_MODE_INTERLACED
            || m_stereoMode == RENDER_STEREO_MODE_CHECKERBOARD)
      {
        pRTView = m_rightEyeTex.GetRenderTarget();
      }
    }
  }

  if (pRTView == nullptr)
    return true;

  auto outputSize = m_deviceResources->GetOutputSize();
  CRect clRect(0.0f, 0.0f,
    static_cast<float>(outputSize.Width),
    static_cast<float>(outputSize.Height));

  // Unlike Direct3D 9, D3D11 ClearRenderTargetView always clears full extent of the resource view.
  // Viewport and scissor settings are not applied. So clear RT by drawing full sized rect with clear color
  if (m_ScissorsEnabled && m_scissor != clRect)
  {
    bool alphaEnabled = m_BlendEnabled;
    if (alphaEnabled)
      SetAlphaBlendEnable(false);

    CGUITextureD3D::DrawQuad(clRect, color);

    if (alphaEnabled)
      SetAlphaBlendEnable(true);
  }
  else
    m_deviceResources->ClearRenderTarget(pRTView, fColor);

  m_deviceResources->ClearDepthStencil();
  return true;
}

void CRenderSystemDX::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  auto m_pContext = m_deviceResources->GetD3DContext();

  m_pContext->RSSetState(m_ScissorsEnabled ? m_RSScissorEnable.Get() : m_RSScissorDisable.Get());
  m_pContext->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
  float factors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  m_pContext->OMSetBlendState(m_BlendEnabled ? m_BlendEnableState.Get() : m_BlendDisableState.Get(), factors, 0xFFFFFFFF);

  m_pGUIShader->ApplyStateBlock();
}

void CRenderSystemDX::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor)
{
  if (!m_bRenderCreated)
    return;

  // grab the viewport dimensions and location
  float w = m_viewPort.Width * 0.5f;
  float h = m_viewPort.Height * 0.5f;

  XMFLOAT2 offset = XMFLOAT2(camera.x - screenWidth*0.5f, camera.y - screenHeight*0.5f);

  // world view.  Until this is moved onto the GPU (via a vertex shader for instance), we set it to the identity here.
  m_pGUIShader->SetWorld(XMMatrixIdentity());

  // Initialize the view matrix camera view.
  // Multiply the Y coord by -1 then translate so that everything is relative to the camera position.
  XMMATRIX flipY = XMMatrixScaling(1.0, -1.0f, 1.0f);
  XMMATRIX translate = XMMatrixTranslation(-(w + offset.x - stereoFactor), -(h + offset.y), 2 * h);
  m_pGUIShader->SetView(XMMatrixMultiply(translate, flipY));

  // projection onto screen space
  m_pGUIShader->SetProjection(XMMatrixPerspectiveOffCenterLH((-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100 * h));
}

void CRenderSystemDX::Project(float &x, float &y, float &z)
{
  if (!m_bRenderCreated)
    return;

  m_pGUIShader->Project(x, y, z);
}

CRect CRenderSystemDX::GetBackBufferRect()
{
  auto outputSize = m_deviceResources->GetOutputSize();
  return CRect(0.f, 0.f, static_cast<float>(outputSize.Width), static_cast<float>(outputSize.Height));
}

void CRenderSystemDX::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  viewPort.x1 = m_viewPort.TopLeftX;
  viewPort.y1 = m_viewPort.TopLeftY;
  viewPort.x2 = m_viewPort.TopLeftX + m_viewPort.Width;
  viewPort.y2 = m_viewPort.TopLeftY + m_viewPort.Height;
}

void CRenderSystemDX::SetViewPort(const CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  m_viewPort.MinDepth   = 0.0f;
  m_viewPort.MaxDepth   = 1.0f;
  m_viewPort.TopLeftX   = viewPort.x1;
  m_viewPort.TopLeftY   = viewPort.y1;
  m_viewPort.Width      = viewPort.x2 - viewPort.x1;
  m_viewPort.Height     = viewPort.y2 - viewPort.y1;

  m_deviceResources->SetViewPort(m_viewPort);
  m_pGUIShader->SetViewPort(m_viewPort);
}

void CRenderSystemDX::RestoreViewPort()
{
  if (!m_bRenderCreated)
    return;

  m_deviceResources->SetViewPort(m_viewPort);
  m_pGUIShader->SetViewPort(m_viewPort);
}

bool CRenderSystemDX::ScissorsCanEffectClipping()
{
  if (!m_bRenderCreated)
    return false;

  return m_pGUIShader != nullptr && m_pGUIShader->HardwareClipIsPossible();
}

CRect CRenderSystemDX::ClipRectToScissorRect(const CRect &rect)
{
  if (!m_bRenderCreated)
    return CRect();

  float xFactor = m_pGUIShader->GetClipXFactor();
  float xOffset = m_pGUIShader->GetClipXOffset();
  float yFactor = m_pGUIShader->GetClipYFactor();
  float yOffset = m_pGUIShader->GetClipYOffset();

  return CRect(rect.x1 * xFactor + xOffset,
               rect.y1 * yFactor + yOffset,
               rect.x2 * xFactor + xOffset,
               rect.y2 * yFactor + yOffset);
}

void CRenderSystemDX::SetScissors(const CRect& rect)
{
  if (!m_bRenderCreated)
    return;

  auto m_pContext = m_deviceResources->GetD3DContext();

  m_scissor = rect;
  CD3D11_RECT scissor(MathUtils::round_int(rect.x1)
                    , MathUtils::round_int(rect.y1)
                    , MathUtils::round_int(rect.x2)
                    , MathUtils::round_int(rect.y2));

  m_pContext->RSSetScissorRects(1, &scissor);
  m_pContext->RSSetState(m_RSScissorEnable.Get());
  m_ScissorsEnabled = true;
}

void CRenderSystemDX::ResetScissors()
{
  if (!m_bRenderCreated)
    return;

  auto m_pContext = m_deviceResources->GetD3DContext();
  auto outputSize = m_deviceResources->GetOutputSize();

  m_scissor.SetRect(0.0f, 0.0f,
    static_cast<float>(outputSize.Width),
    static_cast<float>(outputSize.Height));

  m_pContext->RSSetState(m_RSScissorDisable.Get());
  m_ScissorsEnabled = false;
}

void CRenderSystemDX::OnDXDeviceLost()
{
  CRenderSystemDX::DestroyRenderSystem();
}

void CRenderSystemDX::OnDXDeviceRestored()
{
  CRenderSystemDX::InitRenderSystem();
}

void CRenderSystemDX::SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view)
{
  CRenderSystemBase::SetStereoMode(mode, view);

  if (!m_bRenderCreated)
    return;

  auto m_pContext = m_deviceResources->GetD3DContext();

  UINT writeMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_RED;
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_GREEN;
  }
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_GREEN;
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_RED;
  }
  if (m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
  {
    if (m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN;
    else if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
  }
  if ( RENDER_STEREO_MODE_INTERLACED    == m_stereoMode
    || RENDER_STEREO_MODE_CHECKERBOARD  == m_stereoMode)
  {
    if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    {
      m_pContext->OMSetRenderTargets(1, m_rightEyeTex.GetAddressOfRTV(), m_deviceResources->GetDSV());
    }
  }
  else if (RENDER_STEREO_MODE_HARDWAREBASED == m_stereoMode)
  {
    m_deviceResources->SetStereoIdx(m_stereoView == RENDER_STEREO_VIEW_RIGHT ? 1 : 0);

    m_pContext->OMSetRenderTargets(1, m_deviceResources->GetBackBuffer().GetAddressOfRTV(), m_deviceResources->GetDSV());
  }

  auto m_pD3DDev = m_deviceResources->GetD3DDevice();

  D3D11_BLEND_DESC desc;
  m_BlendEnableState->GetDesc(&desc);
  // update blend state
  if (desc.RenderTarget[0].RenderTargetWriteMask != writeMask)
  {
    m_BlendDisableState = nullptr;
    m_BlendEnableState = nullptr;

    desc.RenderTarget[0].RenderTargetWriteMask = writeMask;
    m_pD3DDev->CreateBlendState(&desc, &m_BlendEnableState);

    desc.RenderTarget[0].BlendEnable = false;
    m_pD3DDev->CreateBlendState(&desc, &m_BlendDisableState);

    float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pContext->OMSetBlendState(m_BlendEnabled ? m_BlendEnableState.Get() : m_BlendDisableState.Get(), blendFactors, 0xFFFFFFFF);
  }
}

bool CRenderSystemDX::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch (mode)
  {
    case RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN:
    case RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA:
    case RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE:
    case RENDER_STEREO_MODE_INTERLACED:
    case RENDER_STEREO_MODE_CHECKERBOARD:
      return true;
    case RENDER_STEREO_MODE_HARDWAREBASED:
      return m_deviceResources->IsStereoAvailable();
    default:
      return CRenderSystemBase::SupportsStereo(mode);
  }
}

void CRenderSystemDX::FlushGPU() const
{
  if (!m_bRenderCreated)
    return;

  m_deviceResources->FinishCommandList();
  m_deviceResources->GetImmediateContext()->Flush();
}

bool CRenderSystemDX::InitGUIShader()
{
  delete m_pGUIShader;
  m_pGUIShader = nullptr;

  m_pGUIShader = new CGUIShaderDX();
  if (!m_pGUIShader->Initialize())
  {
    CLog::LogF(LOGERROR, "Failed to initialize GUI shader.");
    return false;
  }

  m_pGUIShader->ApplyStateBlock();
  return true;
}

void CRenderSystemDX::SetAlphaBlendEnable(bool enable)
{
  if (!m_bRenderCreated)
    return;

  m_deviceResources->GetD3DContext()->OMSetBlendState(enable ? m_BlendEnableState.Get() : m_BlendDisableState.Get(), nullptr, 0xFFFFFFFF);
  m_BlendEnabled = enable;
}

CD3DTexture& CRenderSystemDX::GetBackBuffer()
{
  if (m_stereoView == RENDER_STEREO_VIEW_RIGHT && m_rightEyeTex.Get())
    return m_rightEyeTex;

  return m_deviceResources->GetBackBuffer();
}

void CRenderSystemDX::CheckDeviceCaps()
{
  const auto feature_level = m_deviceResources->GetDeviceFeatureLevel();
  if (feature_level < D3D_FEATURE_LEVEL_9_3)
    m_maxTextureSize = D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else if (feature_level < D3D_FEATURE_LEVEL_10_0)
    m_maxTextureSize = D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else if (feature_level < D3D_FEATURE_LEVEL_11_0)
    m_maxTextureSize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else
    // 11_x and greater feature level. Limit this size to avoid memory overheads
    m_maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION >> 1;
}

bool CRenderSystemDX::SupportsNPOT(bool dxt) const
{
  // MSDN says:
  // At feature levels 9_1, 9_2 and 9_3, the display device supports the use
  // of 2D textures with dimensions that are not powers of two under two conditions:
  // 1) only one MIP-map level for each texture can be created - we are using both 1 and 0 mipmap levels
  // 2) no wrap sampler modes for textures are allowed - we are using clamp everywhere
  // At feature levels 10_0, 10_1 and 11_0, the display device unconditionally supports the use of 2D textures with dimensions that are not powers of two.
  // taking in account first condition we setup caps NPOT for FE > 9.x only
  return m_deviceResources->GetDeviceFeatureLevel() > D3D_FEATURE_LEVEL_9_3 ? true : false;
}
