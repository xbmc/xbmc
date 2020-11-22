/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererBase.h"

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Video/DXVA.h"
#include "ServiceBroker.h"
#include "VideoRenderers/BaseRenderer.h"
#include "VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "rendering/dx/RenderContext.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/MemUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

using namespace Microsoft::WRL;

void CRenderBuffer::AppendPicture(const VideoPicture& picture)
{
  videoBuffer = picture.videoBuffer;
  videoBuffer->Acquire();

  pictureFlags = picture.iFlags;
  primaries = static_cast<AVColorPrimaries>(picture.color_primaries);
  color_space = static_cast<AVColorSpace>(picture.color_space);
  color_transfer = static_cast<AVColorTransferCharacteristic>(picture.color_transfer);
  full_range = picture.color_range == 1;
  bits = picture.colorBits;
  stereoMode = picture.stereoMode;

  hasDisplayMetadata = picture.hasDisplayMetadata;
  displayMetadata = picture.displayMetadata;
  lightMetadata = picture.lightMetadata;
  hasLightMetadata = picture.hasLightMetadata && picture.lightMetadata.MaxCLL;
  if (hasDisplayMetadata && displayMetadata.has_luminance && !displayMetadata.max_luminance.num)
    displayMetadata.has_luminance = 0;
}

void CRenderBuffer::ReleasePicture()
{
  if (videoBuffer)
    videoBuffer->Release();
  videoBuffer = nullptr;
  m_bLoaded = false;
}

CRenderBuffer::CRenderBuffer(AVPixelFormat av_pix_format, unsigned width, unsigned height)
  : av_format(av_pix_format) , m_width(width) , m_height(height), m_widthTex(width), m_heightTex(height)
{
}

HRESULT CRenderBuffer::GetResource(ID3D11Resource** ppResource, unsigned* index) const
{
  if (!ppResource)
    return E_POINTER;
  if (!index)
    return E_POINTER;

  auto dxva = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer);
  if (!dxva)
    return E_NOT_SET;

  ComPtr<ID3D11Resource> pResource;
  const HRESULT hr = dxva->GetResource(&pResource);
  if (SUCCEEDED(hr))
  {
    *ppResource = pResource.Detach();
    *index = dxva->GetIdx();
  }

  return hr;
}

void CRenderBuffer::QueueCopyFromGPU()
{
  if (!videoBuffer)
    return;

  unsigned index;
  ComPtr<ID3D11Resource> pResource;
  const HRESULT hr = GetResource(&pResource, &index);

  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "unable to open d3d11va resource.");
    return;
  }

  if (!m_staging)
  {
    // create staging texture
    ComPtr<ID3D11Texture2D> surface;
    if (SUCCEEDED(pResource.As(&surface)))
    {
      D3D11_TEXTURE2D_DESC tDesc;
      surface->GetDesc(&tDesc);

      CD3D11_TEXTURE2D_DESC sDesc(tDesc);
      sDesc.ArraySize = 1;
      sDesc.Usage = D3D11_USAGE_STAGING;
      sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      sDesc.BindFlags = 0;
      sDesc.MiscFlags = 0;

      ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
      if (SUCCEEDED(pDevice->CreateTexture2D(&sDesc, nullptr, &m_staging)))
        m_sDesc = sDesc;
    }
  }

  if (m_staging)
  {
    ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
    // queue copying content from decoder texture to temporary texture.
    // actual data copying will be performed before rendering
    pContext->CopySubresourceRegion(m_staging.Get(), D3D11CalcSubresource(0, 0, 1), 0, 0, 0,
                                    pResource.Get(), D3D11CalcSubresource(0, index, 1), nullptr);
    m_bPending = true;
  }
}


CRendererBase::CRendererBase(CVideoSettings& videoSettings)
  : m_videoSettings(videoSettings)
{
  m_colorManager.reset(new CColorManager());
}

CRendererBase::~CRendererBase()
{
  if (DX::Windowing()->IsHDROutput())
  {
    CLog::LogF(LOGDEBUG, "Restoring SDR rendering");
    DX::Windowing()->SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
    if (m_AutoSwitchHDR)
      DX::Windowing()->ToggleHDR(); // Toggle display HDR OFF
  }
  Flush(false);
}

CRenderInfo CRendererBase::GetRenderInfo()
{
  CRenderInfo info;
  info.formats =
  {
    AV_PIX_FMT_D3D11VA_VLD,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_P010,
    AV_PIX_FMT_P016,
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_YUV420P16
  };
  info.max_buffer_size = NUM_BUFFERS;
  info.optimal_buffer_size = 4;

  return info;
}

bool CRendererBase::Configure(const VideoPicture& picture, float fps, unsigned orientation)
{
  m_iNumBuffers = 0;
  m_iBufferIndex = 0;

  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_fps = fps;
  m_renderOrientation = orientation;

  m_lastHdr10 = {};
  m_HdrType = HDR_TYPE::HDR_NONE_SDR;
  m_useHLGtoPQ = false;
  m_AutoSwitchHDR = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                        DX::Windowing()->SETTING_WINSYSTEM_IS_HDR_DISPLAY) &&
                    DX::Windowing()->IsHDRDisplay();

  // Auto switch HDR only if supported and "Settings/Player/Use HDR display capabilities" = ON
  if (m_AutoSwitchHDR)
  {
    bool streamIsHDR = (picture.color_primaries == AVCOL_PRI_BT2020) &&
                       (picture.color_transfer == AVCOL_TRC_SMPTE2084 ||
                        picture.color_transfer == AVCOL_TRC_ARIB_STD_B67);

    if (streamIsHDR != DX::Windowing()->IsHDROutput())
      DX::Windowing()->ToggleHDR();
  }

  return true;
}

void CRendererBase::AddVideoPicture(const VideoPicture& picture, int index)
{
  if (m_renderBuffers[index])
  {
    m_renderBuffers[index]->AppendPicture(picture);
    m_renderBuffers[index]->frameIdx = m_frameIdx;
    m_frameIdx += 2;
  }
}

void CRendererBase::Render(int index, int index2, CD3DTexture& target, const CRect& sourceRect, 
                           const CRect& destRect, const CRect& viewRect, unsigned flags)
{
  m_iBufferIndex = index;
  ManageTextures();
  Render(target, sourceRect, destRect, viewRect, flags);
}

void CRendererBase::Render(CD3DTexture& target, const CRect& sourceRect, const CRect& destRect, const CRect& viewRect, unsigned flags)
{
  if (m_iNumBuffers == 0)
    return;

  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  if (!buf->IsLoaded())
  {
    if (!buf->UploadBuffer())
      return;
  }

  ProcessHDR(buf);

  if (m_viewWidth != static_cast<unsigned>(viewRect.Width()) ||
    m_viewHeight != static_cast<unsigned>(viewRect.Height()))
  {
    m_viewWidth = static_cast<unsigned>(viewRect.Width());
    m_viewHeight = static_cast<unsigned>(viewRect.Height());

    OnViewSizeChanged();
  }

  CheckVideoParameters();
  UpdateVideoFilters();

  CPoint dest[4];
  CRect source = sourceRect;     // can be changed
  CRect(destRect).GetQuad(dest); // can be changed

  RenderImpl(m_IntermediateTarget, source, dest, flags);

  if (m_toneMapping)
  {
    m_outputShader->SetDisplayMetadata(buf->hasDisplayMetadata, buf->displayMetadata, buf->hasLightMetadata, buf->lightMetadata);
    m_outputShader->SetToneMapParam(m_toneMapMethod, m_videoSettings.m_ToneMapParam);
  }

  FinalOutput(m_IntermediateTarget, target, source, dest);

  // Restore our view port.
  DX::Windowing()->RestoreViewPort();
  DX::Windowing()->ApplyStateBlock();
}

void CRendererBase::FinalOutput(CD3DTexture& source, CD3DTexture& target, const CRect& src, const CPoint(&destPoints)[4])
{
  m_outputShader->Render(source, src, destPoints, target);
}

void CRendererBase::ManageTextures()
{
  if (m_iNumBuffers < m_iBuffersRequired)
  {
    for (int i = m_iNumBuffers; i < m_iBuffersRequired; i++)
      CreateRenderBuffer(i);

    m_iNumBuffers = m_iBuffersRequired;
  }
  else if (m_iNumBuffers > m_iBuffersRequired)
  {
    for (int i = m_iNumBuffers - 1; i >= m_iBuffersRequired; i--)
      DeleteRenderBuffer(i);

    m_iNumBuffers = m_iBuffersRequired;
    m_iBufferIndex = m_iBufferIndex % m_iNumBuffers;
  }
}

int CRendererBase::NextBuffer() const
{
  if (m_iNumBuffers)
    return (m_iBufferIndex + 1) % m_iNumBuffers;
  return -1;
}

void CRendererBase::ReleaseBuffer(int idx)
{
  if (m_renderBuffers[idx])
    m_renderBuffers[idx]->ReleasePicture();
}

bool CRendererBase::Flush(bool saveBuffers)
{
  if (!saveBuffers)
  {
    for (int i = 0; i < NUM_BUFFERS; i++)
      DeleteRenderBuffer(i);

    m_iBufferIndex = 0;
    m_iNumBuffers = 0;
  }

  return true;
}

bool CRendererBase::CreateRenderBuffer(int index)
{
  m_renderBuffers.insert(std::make_pair(index, CreateBuffer()));
  return true;
}

void CRendererBase::DeleteRenderBuffer(int index)
{
  if (m_renderBuffers[index])
  {
    delete m_renderBuffers[index];
    m_renderBuffers.erase(index);
  }
}

bool CRendererBase::CreateIntermediateTarget(unsigned width, unsigned height, bool dynamic)
{
  DXGI_FORMAT format = DX::Windowing()->GetBackBuffer().GetFormat();

  // don't create new one if it exists with requested size and format
  if (m_IntermediateTarget.Get() && m_IntermediateTarget.GetFormat() == format
    && m_IntermediateTarget.GetWidth() == width && m_IntermediateTarget.GetHeight() == height)
    return true;

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  CLog::LogF(LOGDEBUG, "intermediate target format %i.", format);

  if (!m_IntermediateTarget.Create(width, height, 1, dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT, format))
  {
    CLog::LogF(LOGERROR, "intermediate target creation failed.");
    return false;
  }
  return true;
}

void CRendererBase::OnCMSConfigChanged(AVColorPrimaries srcPrimaries)
{
  m_lutSize = 0;
  m_clutLoaded = false;

  auto loadLutTask = Concurrency::create_task([this, srcPrimaries] {
    // load 3DLUT data
    int lutSize, dataSize;
    if (!CColorManager::Get3dLutSize(CMS_DATA_FMT_RGBA, &lutSize, &dataSize))
      return 0;

    const auto lutData = static_cast<uint16_t*>(KODI::MEMORY::AlignedMalloc(dataSize, 16));
    bool success = m_colorManager->GetVideo3dLut(srcPrimaries, &m_cmsToken, CMS_DATA_FMT_RGBA,
                                                 lutSize, lutData);
    if (success)
    {
      success = COutputShader::CreateLUTView(lutSize, lutData, false, m_pLUTView.ReleaseAndGetAddressOf());
    }
    else
      CLog::LogFunction(LOGERROR, "CRendererBase::OnCMSConfigChanged", "unable to loading the 3dlut data.");

    KODI::MEMORY::AlignedFree(lutData);
    if (!success)
      return 0;

    return lutSize;
  });

  loadLutTask.then([&](const int lutSize) {
    m_lutSize = lutSize;
    if (m_outputShader)
      m_outputShader->SetLUT(m_lutSize, m_pLUTView.Get());
    m_clutLoaded = true;
  });
}

// this is copy from CBaseRenderer::ReorderDrawPoints()
void CRendererBase::ReorderDrawPoints(const CRect& destRect, CPoint(&rotatedPoints)[4]) const
{
  // 0 - top left, 1 - top right, 2 - bottom right, 3 - bottom left
  float origMat[4][2] = {{destRect.x1, destRect.y1},
                         {destRect.x2, destRect.y1},
                         {destRect.x2, destRect.y2},
                         {destRect.x1, destRect.y2}};

  const int pointOffset = m_renderOrientation / 90;

  for (int destIdx = 0, srcIdx = pointOffset; destIdx < 4; destIdx++)
  {
    rotatedPoints[destIdx].x = origMat[srcIdx][0];
    rotatedPoints[destIdx].y = origMat[srcIdx][1];

    srcIdx++;
    srcIdx = srcIdx % 4;
  }
}

void CRendererBase::UpdateVideoFilters()
{
  if (!m_outputShader)
  {
    m_outputShader = std::make_shared<COutputShader>();
    if (!m_outputShader->Create(m_cmsOn, m_useDithering, m_ditherDepth, m_toneMapping,
                                m_toneMapMethod, m_useHLGtoPQ))
    {
      CLog::LogF(LOGDEBUG, "unable to create output shader.");
      m_outputShader.reset();
    }
    else if (m_pLUTView && m_lutSize)
    {
      m_outputShader->SetLUT(m_lutSize, m_pLUTView.Get());
    }
  }
}

void CRendererBase::CheckVideoParameters()
{
  CRenderBuffer* buf = m_renderBuffers[m_iBufferIndex];
  int method = m_videoSettings.m_ToneMapMethod;

  bool isHDRPQ = (buf->color_transfer == AVCOL_TRC_SMPTE2084 && buf->primaries == AVCOL_PRI_BT2020);

  bool toneMap = (isHDRPQ && m_HdrType == HDR_TYPE::HDR_NONE_SDR && method != VS_TONEMAPMETHOD_OFF);

  bool hlg = (m_HdrType == HDR_TYPE::HDR_HLG);

  if (toneMap != m_toneMapping || m_cmsOn != m_colorManager->IsEnabled() || hlg != m_useHLGtoPQ ||
      method != m_toneMapMethod)
  {
    m_toneMapping = toneMap;
    m_cmsOn = m_colorManager->IsEnabled();
    m_useHLGtoPQ = hlg;
    m_toneMapMethod = method;

    m_outputShader.reset();
    OnOutputReset();
  }

  const AVColorPrimaries color_primaries = static_cast<AVColorPrimaries>(buf->primaries);
  if (m_cmsOn && !m_colorManager->CheckConfiguration(m_cmsToken, color_primaries))
  {
    OnCMSConfigChanged(color_primaries);
  }
}

DXGI_FORMAT CRendererBase::GetDXGIFormat(const VideoPicture& picture)
{
  if (picture.videoBuffer && picture.videoBuffer->GetFormat() == AV_PIX_FMT_D3D11VA_VLD)
    return GetDXGIFormat(picture.videoBuffer);

  return DXGI_FORMAT_UNKNOWN;
}

DXGI_FORMAT CRendererBase::GetDXGIFormat(CVideoBuffer* videoBuffer)
{
  const auto dxva_buf = dynamic_cast<DXVA::CVideoBuffer*>(videoBuffer);
  if (dxva_buf)
    return dxva_buf->format;

  return DXGI_FORMAT_UNKNOWN;
}

AVPixelFormat CRendererBase::GetAVFormat(DXGI_FORMAT dxgi_format)
{
  switch (dxgi_format)
  {
  case DXGI_FORMAT_NV12:
    return AV_PIX_FMT_NV12;
  case DXGI_FORMAT_P010:
    return AV_PIX_FMT_P010;
  case DXGI_FORMAT_P016:
    return AV_PIX_FMT_P016;
  default:
    return AV_PIX_FMT_NONE;
  }
}

DXGI_HDR_METADATA_HDR10 CRendererBase::GetDXGIHDR10MetaData(CRenderBuffer* rb)
{
  DXGI_HDR_METADATA_HDR10 hdr10 = {};

  if (rb->hasDisplayMetadata && rb->displayMetadata.has_primaries)
  {
    hdr10.RedPrimary[0] = static_cast<uint16_t>(rb->displayMetadata.display_primaries[0][0].num);
    hdr10.RedPrimary[1] = static_cast<uint16_t>(rb->displayMetadata.display_primaries[0][1].num);
    hdr10.GreenPrimary[0] = static_cast<uint16_t>(rb->displayMetadata.display_primaries[1][0].num);
    hdr10.GreenPrimary[1] = static_cast<uint16_t>(rb->displayMetadata.display_primaries[1][1].num);
    hdr10.BluePrimary[0] = static_cast<uint16_t>(rb->displayMetadata.display_primaries[2][0].num);
    hdr10.BluePrimary[1] = static_cast<uint16_t>(rb->displayMetadata.display_primaries[2][1].num);
    hdr10.WhitePoint[0] = static_cast<uint16_t>(rb->displayMetadata.white_point[0].num);
    hdr10.WhitePoint[1] = static_cast<uint16_t>(rb->displayMetadata.white_point[1].num);
  }
  if (rb->hasDisplayMetadata && rb->displayMetadata.has_luminance)
  {
    hdr10.MaxMasteringLuminance = static_cast<uint32_t>(rb->displayMetadata.max_luminance.num);
    hdr10.MinMasteringLuminance = static_cast<uint32_t>(rb->displayMetadata.min_luminance.num);
  }
  if (rb->hasLightMetadata)
  {
    hdr10.MaxContentLightLevel = static_cast<uint16_t>(rb->lightMetadata.MaxCLL);
    hdr10.MaxFrameAverageLightLevel = static_cast<uint16_t>(rb->lightMetadata.MaxFALL);
  }

  return hdr10;
}

void CRendererBase::ProcessHDR(CRenderBuffer* rb)
{
  if (m_AutoSwitchHDR && rb->primaries == AVCOL_PRI_BT2020 &&
      (rb->color_transfer == AVCOL_TRC_SMPTE2084 || rb->color_transfer == AVCOL_TRC_ARIB_STD_B67) &&
      !DX::Windowing()->IsHDROutput())
  {
    DX::Windowing()->ToggleHDR(); // Toggle display HDR ON
  }

  if (!DX::Windowing()->IsHDROutput())
  {
    if (m_HdrType != HDR_TYPE::HDR_NONE_SDR)
    {
      m_HdrType = HDR_TYPE::HDR_NONE_SDR;
      m_lastHdr10 = {};
    }
    return;
  }

  // HDR10
  if (rb->color_transfer == AVCOL_TRC_SMPTE2084 && rb->primaries == AVCOL_PRI_BT2020)
  {
    DXGI_HDR_METADATA_HDR10 hdr10 = GetDXGIHDR10MetaData(rb);
    if (m_HdrType == HDR_TYPE::HDR_HDR10)
    {
      // Sets HDR10 metadata only if it differs from previous
      if (0 != std::memcmp(&hdr10, &m_lastHdr10, sizeof(hdr10)))
      {
        DX::Windowing()->SetHdrMetaData(hdr10);
        m_lastHdr10 = hdr10;
      }
    }
    else
    {
      // Sets HDR10 metadata and enables HDR10 color space (switch to HDR rendering)
      DX::Windowing()->SetHdrMetaData(hdr10);
      CLog::LogF(LOGINFO, "Switching to HDR rendering");
      DX::Windowing()->SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
      m_HdrType = HDR_TYPE::HDR_HDR10;
      m_lastHdr10 = hdr10;
    }
  }
  // HLG
  else if (rb->color_transfer == AVCOL_TRC_ARIB_STD_B67 && rb->primaries == AVCOL_PRI_BT2020)
  {
    if (m_HdrType != HDR_TYPE::HDR_HLG)
    {
      // Windows 10 doesn't support HLG HDR passthrough
      // It's used HDR10 with dummy metadata and shaders to convert HLG transfer to PQ transfer
      DXGI_HDR_METADATA_HDR10 hdr10 = {};
      hdr10.RedPrimary[0] = 34000; // Display P3 primaries
      hdr10.RedPrimary[1] = 16000;
      hdr10.GreenPrimary[0] = 13250;
      hdr10.GreenPrimary[1] = 34500;
      hdr10.BluePrimary[0] = 7500;
      hdr10.BluePrimary[1] = 3000;
      hdr10.WhitePoint[0] = 15635;
      hdr10.WhitePoint[1] = 16450;
      hdr10.MaxMasteringLuminance = 1000 * 10000; // 1000 nits
      hdr10.MinMasteringLuminance = 100; // 0.01 nits
      DX::Windowing()->SetHdrMetaData(hdr10);
      CLog::LogF(LOGINFO, "Switching to HDR rendering");
      DX::Windowing()->SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
      m_HdrType = HDR_TYPE::HDR_HLG;
    }
  }
  // SDR
  else
  {
    if (m_HdrType != HDR_TYPE::HDR_NONE_SDR)
    {
      // Switch to SDR rendering
      CLog::LogF(LOGINFO, "Switching to SDR rendering");
      DX::Windowing()->SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
      m_HdrType = HDR_TYPE::HDR_NONE_SDR;
      m_lastHdr10 = {};
      if (m_AutoSwitchHDR)
        DX::Windowing()->ToggleHDR(); // Toggle display HDR OFF
    }
  }
}
