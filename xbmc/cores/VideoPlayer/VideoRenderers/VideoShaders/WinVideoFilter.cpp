/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinVideoFilter.h"

#include "ConvolutionKernels.h"
#include "ToneMappers.h"
#include "Util.h"
#include "VideoRenderers/windows/RendererBase.h"
#include "cores/VideoPlayer/VideoRenderers/VideoShaders/dither.h"
#include "filesystem/File.h"
#include "rendering/dx/RenderContext.h"
#include "utils/MemUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "platform/win32/WIN32Util.h"

#include <map>

#include <DirectXPackedVector.h>

using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

//===================================================================

bool CWinShader::CreateVertexBuffer(unsigned int count, unsigned int size)
{
  if (!m_vb.Create(D3D11_BIND_VERTEX_BUFFER, count, size, DXGI_FORMAT_UNKNOWN, D3D11_USAGE_DYNAMIC))
    return false;

  uint16_t id[4] = { 3, 0, 2, 1 };
  if (!m_ib.Create(D3D11_BIND_INDEX_BUFFER, ARRAYSIZE(id), sizeof(uint16_t), DXGI_FORMAT_R16_UINT, D3D11_USAGE_IMMUTABLE, id))
    return false;

  m_vbsize = count * size;
  m_vertsize = size;

  return true;
}

bool CWinShader::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC *layout, unsigned numElements)
{
  D3DX11_PASS_DESC desc = {};
  if (FAILED(m_effect.Get()->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&desc)))
  {
    CLog::LogF(LOGERROR, "Failed to get first pass description.");
    return false;
  }

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  return SUCCEEDED(pDevice->CreateInputLayout(layout, numElements, desc.pIAInputSignature, desc.IAInputSignatureSize, &m_inputLayout));
}

void CWinShader::SetTarget(CD3DTexture* target)
{
  m_target = target;
  if (m_target)
    DX::DeviceResources::Get()->GetD3DContext()->OMSetRenderTargets(1, target->GetAddressOfRTV(), nullptr);
}

bool CWinShader::LockVertexBuffer(void **data)
{
  if (!m_vb.Map(data))
  {
    CLog::LogF(LOGERROR, "failed to lock vertex buffer");
    return false;
  }
  return true;
}

bool CWinShader::UnlockVertexBuffer()
{
  if (!m_vb.Unmap())
  {
    CLog::LogF(LOGERROR, "failed to unlock vertex buffer");
    return false;
  }
  return true;
}

bool CWinShader::LoadEffect(const std::string& filename, DefinesMap* defines)
{
  CLog::LogF(LOGDEBUG, "loading shader {}", filename);

  XFILE::CFileStream file;
  if(!file.Open(filename))
  {
    CLog::LogF(LOGERROR, "failed to open file {}", filename);
    return false;
  }

  std::string pStrEffect;
  getline(file, pStrEffect, '\0');

  if (!m_effect.Create(pStrEffect, defines))
  {
    CLog::LogF(LOGERROR, "{} failed", pStrEffect);
    return false;
  }

  return true;
}

bool CWinShader::Execute(const std::vector<CD3DTexture*> &targets, unsigned int vertexIndexStep)
{
  ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();
  ComPtr<ID3D11RenderTargetView> oldRT;

  // The render target will be overridden: save the caller's original RT
  if (!targets.empty())
    pContext->OMGetRenderTargets(1, &oldRT, nullptr);

  unsigned cPasses;
  if (!m_effect.Begin(&cPasses, 0))
  {
    CLog::LogF(LOGERROR, "failed to begin d3d effect");
    return false;
  }

  ID3D11Buffer* vertexBuffer = m_vb.Get();
  ID3D11Buffer* indexBuffer = m_ib.Get();
  unsigned int stride = m_vb.GetStride();
  unsigned int offset = 0;
  pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  pContext->IASetIndexBuffer(indexBuffer, m_ib.GetFormat(), 0);
  pContext->IASetInputLayout(m_inputLayout.Get());
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  for (unsigned iPass = 0; iPass < cPasses; iPass++)
  {
    SetTarget(targets.size() > iPass ? targets.at(iPass) : nullptr);
    SetStepParams(iPass);

    if (!m_effect.BeginPass(iPass))
    {
      CLog::LogF(LOGERROR, "failed to begin d3d effect pass");
      break;
    }

    pContext->DrawIndexed(4, 0, iPass * vertexIndexStep);

    if (!m_effect.EndPass())
      CLog::LogF(LOGERROR, "failed to end d3d effect pass");

    CD3DHelper::PSClearShaderResources(pContext);
  }
  if (!m_effect.End())
    CLog::LogF(LOGERROR, "failed to end d3d effect");

  if (oldRT)
    pContext->OMSetRenderTargets(1, oldRT.GetAddressOf(), nullptr);

  return true;
}

//==================================================================================

void COutputShader::ApplyEffectParameters(CD3DEffect &effect, unsigned sourceWidth, unsigned sourceHeight)
{
  if (m_useLut && HasLUT())
  {
    float lut_params[2] = {(m_lutSize - 1) / static_cast<float>(m_lutSize),
                           0.5f / static_cast<float>(m_lutSize)};
    effect.SetResources("m_LUT", m_pLUTView.GetAddressOf(), 1);
    effect.SetFloatArray("m_LUTParams", lut_params, 2);
  }
  if (m_useDithering)
  {
    float ditherParams[3] =
    {
      static_cast<float>(sourceWidth) / dither_size,
      static_cast<float>(sourceHeight) / dither_size,
      static_cast<float>(1 << m_ditherDepth) - 1.0f
    };
    effect.SetResources("m_ditherMatrix", m_pDitherView.GetAddressOf(), 1);
    effect.SetFloatArray("m_ditherParams", ditherParams, 3);
  }
  if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_REINHARD)
  {
    const float def_param = log10(100.0f) / log10(600.0f); // 600 nits --> 0.72
    float param = def_param;

    if (m_hasLightMetadata)
      param = static_cast<float>(log10(100) / log10(m_lightMetadata.MaxCLL));
    else if (m_hasDisplayMetadata && m_displayMetadata.has_luminance)
      param = static_cast<float>(log10(100) / log10(m_displayMetadata.max_luminance.num /
                                                    m_displayMetadata.max_luminance.den));

    // Sanity check
    if (param < 0.1f || param > 5.0f)
      param = def_param;

    param *= m_toneMappingParam;

    Matrix3x1 coefs = CConvertMatrix::GetRGBYuvCoefs(AVCOL_SPC_BT709);

    effect.SetScalar("g_toneP1", param);
    effect.SetFloatArray("g_coefsDst", coefs.data(), coefs.size());
    m_toneMappingDebug = param;
  }
  else if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_ACES)
  {
    const float lumin = CToneMappers::GetLuminanceValue(m_hasDisplayMetadata, m_displayMetadata,
                                                        m_hasLightMetadata, m_lightMetadata);
    effect.SetScalar("g_luminance", lumin);
    effect.SetScalar("g_toneP1", m_toneMappingParam);
    m_toneMappingDebug = lumin;
  }
  else if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_HABLE)
  {
    const float lumin = CToneMappers::GetLuminanceValue(m_hasDisplayMetadata, m_displayMetadata,
                                                        m_hasLightMetadata, m_lightMetadata);
    const float lumin_factor = (10000.0f / lumin) * (2.0f / m_toneMappingParam);
    const float lumin_div100 = lumin / (100.0f * m_toneMappingParam);
    effect.SetScalar("g_toneP1", lumin_factor);
    effect.SetScalar("g_toneP2", lumin_div100);
    m_toneMappingDebug = lumin;
  }
}

void COutputShader::GetDefines(DefinesMap& map) const
{
  if (m_useLut)
  {
    map["KODI_3DLUT"] = "";
  }
  if (m_useDithering)
  {
    map["KODI_DITHER"] = "";
  }
  if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_REINHARD)
  {
    map["KODI_TONE_MAPPING_REINHARD"] = "";
  }
  else if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_ACES)
  {
    map["KODI_TONE_MAPPING_ACES"] = "";
  }
  else if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_HABLE)
  {
    map["KODI_TONE_MAPPING_HABLE"] = "";
  }
  if (m_useHLGtoPQ)
  {
    map["KODI_HLG_TO_PQ"] = "";
  }
}

bool COutputShader::Create(bool useLUT,
                           bool useDithering,
                           int ditherDepth,
                           bool toneMapping,
                           ETONEMAPMETHOD toneMethod,
                           bool HLGtoPQ)
{
  m_useLut = useLUT;
  m_ditherDepth = ditherDepth;
  m_toneMapping = toneMapping;
  m_useHLGtoPQ = HLGtoPQ;
  m_toneMappingMethod = toneMethod;

  CWinShader::CreateVertexBuffer(4, sizeof(Vertex));

  if (useDithering)
    CreateDitherView();

  DefinesMap defines;
  defines["KODI_OUTPUT_T"] = "";
  GetDefines(defines);

  std::string effectString("special://xbmc/system/shaders/output_d3d.fx");

  if (!LoadEffect(effectString, &defines))
  {
    CLog::LogF(LOGERROR, "Failed to load shader {}.", effectString);
    return false;
  }

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  return CWinShader::CreateInputLayout(layout, ARRAYSIZE(layout));
}

void COutputShader::Render(CD3DTexture& sourceTexture, CRect sourceRect, const CPoint points[4]
                         , CD3DTexture& target, unsigned range, float contrast, float brightness)
{
  PrepareParameters(sourceTexture.GetWidth(), sourceTexture.GetHeight(), sourceRect, points);
  SetShaderParameters(sourceTexture, range, contrast, brightness);
  Execute({ &target }, 4);
}

void COutputShader::Render(CD3DTexture& sourceTexture, CRect sourceRect, CRect destRect
                         , CD3DTexture& target, unsigned range, float contrast, float brightness)
{
  CPoint points[] =
  {
    { destRect.x1, destRect.y1 },
    { destRect.x2, destRect.y1 },
    { destRect.x2, destRect.y2 },
    { destRect.x1, destRect.y2 },
  };
  Render(sourceTexture, sourceRect, points, target, range, contrast, brightness);
}

void COutputShader::SetLUT(int lutSize, ID3D11ShaderResourceView* pLUTView)
{
  m_lutSize = lutSize;
  m_pLUTView = pLUTView;
}

void COutputShader::SetDisplayMetadata(bool hasDisplayMetadata, AVMasteringDisplayMetadata displayMetadata, bool hasLightMetadata, AVContentLightMetadata lightMetadata)
{
  m_hasDisplayMetadata = hasDisplayMetadata;
  m_displayMetadata = displayMetadata;
  m_hasLightMetadata = hasLightMetadata;
  m_lightMetadata = lightMetadata;
}

void COutputShader::SetToneMapParam(ETONEMAPMETHOD method, float param)
{
  m_toneMappingMethod = method;
  m_toneMappingParam = param;
}

bool COutputShader::CreateLUTView(int lutSize, uint16_t* lutData, bool isRGB, ID3D11ShaderResourceView** ppLUTView)
{
  if (!lutSize || !lutData)
    return false;

  uint16_t* cData;
  if (isRGB)
  {
    // repack data to RGBA
    const unsigned samples = lutSize * lutSize * lutSize;
    cData = reinterpret_cast<uint16_t*>(KODI::MEMORY::AlignedMalloc(samples * sizeof(uint16_t) * 4, 16));
    auto rgba = static_cast<uint16_t*>(cData);
    for (unsigned i = 0; i < samples - 1; ++i, rgba += 4, lutData += 3)
    {
      *reinterpret_cast<uint64_t*>(rgba) = *reinterpret_cast<uint64_t*>(lutData);
    }
    // and last one
    rgba[0] = lutData[0]; rgba[1] = lutData[1]; rgba[2] = lutData[2]; rgba[3] = 0xFFFF;
  }
  else
    cData = lutData;

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
  CD3D11_TEXTURE3D_DESC txDesc(DXGI_FORMAT_R16G16B16A16_UNORM, lutSize, lutSize, lutSize, 1,
                               D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

  ComPtr<ID3D11Texture3D> pLUTTex;
  D3D11_SUBRESOURCE_DATA texData;
  texData.pSysMem = cData;
  texData.SysMemPitch = lutSize * sizeof(uint16_t) * 4;
  texData.SysMemSlicePitch = texData.SysMemPitch * lutSize;

  HRESULT hr = pDevice->CreateTexture3D(&txDesc, &texData, pLUTTex.GetAddressOf());
  if (isRGB)
    KODI::MEMORY::AlignedFree(cData);

  if (FAILED(hr))
  {
    CLog::LogF(LOGDEBUG, "unable to create 3dlut texture cube.");
    return false;
  }

  ComPtr<ID3D11ShaderResourceView> lutView;
  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE3D, DXGI_FORMAT_R16G16B16A16_UNORM, 0, 1);
  hr = pDevice->CreateShaderResourceView(pLUTTex.Get(), &srvDesc, &lutView);
  pContext->Flush();

  if (FAILED(hr))
  {
    CLog::LogF(LOGDEBUG, "unable to create view for 3dlut texture cube.");
    return false;
  }

  *ppLUTView = lutView.Detach();
  return true;
}

void COutputShader::PrepareParameters(unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4])
{
  bool changed = false;
  for(int i= 0; i < 4 && !changed; ++i)
    changed = points[i] != m_destPoints[i];

  if (m_sourceWidth != sourceWidth || m_sourceHeight != sourceHeight
    || m_sourceRect != sourceRect || changed)
  {
    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceRect = sourceRect;
    for (int i = 0; i < 4; ++i)
      m_destPoints[i] = points[i];

    Vertex* v;
    CWinShader::LockVertexBuffer(reinterpret_cast<void**>(&v));

    v[0].x = m_destPoints[0].x;
    v[0].y = m_destPoints[0].y;
    v[0].z = 0;
    v[0].tu = m_sourceRect.x1 / m_sourceWidth;
    v[0].tv = m_sourceRect.y1 / m_sourceHeight;

    v[1].x = m_destPoints[1].x;
    v[1].y = m_destPoints[1].y;
    v[1].z = 0;
    v[1].tu = m_sourceRect.x2 / m_sourceWidth;
    v[1].tv = m_sourceRect.y1 / m_sourceHeight;

    v[2].x = m_destPoints[2].x;
    v[2].y = m_destPoints[2].y;
    v[2].z = 0;
    v[2].tu = m_sourceRect.x2 / m_sourceWidth;
    v[2].tv = m_sourceRect.y2 / m_sourceHeight;

    v[3].x = m_destPoints[3].x;
    v[3].y = m_destPoints[3].y;
    v[3].z = 0;
    v[3].tu = m_sourceRect.x1 / m_sourceWidth;
    v[3].tv = m_sourceRect.y2 / m_sourceHeight;

    CWinShader::UnlockVertexBuffer();
  }
}

void COutputShader::SetShaderParameters(CD3DTexture& sourceTexture, unsigned range, float contrast, float brightness)
{
  m_effect.SetTechnique("OUTPUT_T");
  m_effect.SetResources("g_Texture", sourceTexture.GetAddressOfSRV(), 1);

  unsigned numPorts = 1;
  D3D11_VIEWPORT viewPort;
  DX::DeviceResources::Get()->GetD3DContext()->RSGetViewports(&numPorts, &viewPort);
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);

  float params[3] = { static_cast<float>(range), contrast, brightness };
  m_effect.SetFloatArray("m_params", params, 3);

  ApplyEffectParameters(m_effect, sourceTexture.GetWidth(), sourceTexture.GetHeight());
}

void COutputShader::CreateDitherView()
{
  ID3D11Device* pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  CD3D11_TEXTURE2D_DESC txDesc(DXGI_FORMAT_R16_UNORM, dither_size, dither_size, 1, 1);
  D3D11_SUBRESOURCE_DATA resData;
  resData.pSysMem = dither_matrix;
  resData.SysMemPitch = dither_size * sizeof(uint16_t);
  resData.SysMemSlicePitch = resData.SysMemPitch * dither_size;

  ComPtr<ID3D11Texture2D> pDitherTex;
  HRESULT hr = pDevice->CreateTexture2D(&txDesc, &resData, &pDitherTex);

  if (FAILED(hr))
  {
    CLog::LogF(LOGDEBUG, "unable to create dither texture cube.");
    m_useDithering = false;
    return;
  }

  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R16_UNORM, 0, 1);
  hr = pDevice->CreateShaderResourceView(pDitherTex.Get(), &srvDesc, &m_pDitherView);
  if (FAILED(hr))
  {
    CLog::LogF(LOGDEBUG, "unable to create view for dither texture cube.");
    m_useDithering = false;
    return;
  }
  m_useDithering = true;
}

std::string COutputShader::GetDebugInfo()
{
  std::string tone = "OFF";
  std::string hlg = "OFF";
  std::string lut = "OFF";
  std::string dither = "OFF";

  if (m_toneMapping)
  {
    std::string method;
    switch (m_toneMappingMethod)
    {
      case VS_TONEMAPMETHOD_REINHARD:
        method = "Reinhard";
        break;
      case VS_TONEMAPMETHOD_ACES:
        method = "ACES";
        break;
      case VS_TONEMAPMETHOD_HABLE:
        method = "Hable";
        break;
    }
    tone = StringUtils::Format("ON ({}, {:.2f}, {:.2f}{})", method, m_toneMappingParam,
                               m_toneMappingDebug, (m_toneMappingMethod == 1) ? "" : " nits");
  }

  if (m_useHLGtoPQ)
    hlg = "ON (peak 1000 nits)";

  if (m_useLut)
    lut = StringUtils::Format("ON (size {})", m_lutSize);

  if (m_useDithering)
    dither = StringUtils::Format("ON (depth {})", m_ditherDepth);

  return StringUtils::Format("Tone mapping: {} | HLG to PQ: {} | 3D LUT: {} | Dithering: {}", tone,
                             hlg, lut, dither);
}

//==================================================================================

bool CYUV2RGBShader::Create(AVPixelFormat fmt, AVColorPrimaries dstPrimaries,
                            AVColorPrimaries srcPrimaries, const std::shared_ptr<COutputShader>& pOutputShader)
{
  m_format = fmt;
  m_pOutShader = pOutputShader;

  DefinesMap defines;

  switch (fmt)
  {
  case AV_PIX_FMT_YUV420P:
  case AV_PIX_FMT_YUV420P10:
  case AV_PIX_FMT_YUV420P16:
    defines["XBMC_YV12"] = "";
    break;
  case AV_PIX_FMT_D3D11VA_VLD:
  case AV_PIX_FMT_NV12:
  case AV_PIX_FMT_P010:
  case AV_PIX_FMT_P016:
    defines["XBMC_NV12"] = "";
    // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
    if (!DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R8G8_UNORM, D3D11_FORMAT_SUPPORT_TEXTURE2D))
      defines["NV12_SNORM_UV"] = "";
    break;
  case AV_PIX_FMT_UYVY422:
    defines["XBMC_UYVY"] = "";
    break;
  case AV_PIX_FMT_YUYV422:
    defines["XBMC_YUY2"] = "";
    break;
  default:
    return false;
  }

  if (srcPrimaries != dstPrimaries)
  {
    m_colorConversion = true;
    defines["XBMC_COL_CONVERSION"] = "";
  }

  if (m_pOutShader)
    m_pOutShader->GetDefines(defines);

  std::string effectString = "special://xbmc/system/shaders/yuv2rgb_d3d.fx";

  if(!LoadEffect(effectString, &defines))
  {
    CLog::LogF(LOGERROR, "Failed to load shader {}.", effectString);
    return false;
  }

  CWinShader::CreateVertexBuffer(4, sizeof(Vertex));
  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  if (!CWinShader::CreateInputLayout(layout, ARRAYSIZE(layout)))
  {
    CLog::LogF(LOGERROR, "Failed to create input layout for Input Assembler.");
    return false;
  }

  m_convMatrix.SetDestinationColorPrimaries(dstPrimaries).SetSourceColorPrimaries(srcPrimaries);

  return true;
}

void CYUV2RGBShader::Render(CRect sourceRect, CPoint dest[], CRenderBuffer* videoBuffer, CD3DTexture& target)
{
  PrepareParameters(videoBuffer, sourceRect, dest);
  SetShaderParameters(videoBuffer);
  Execute({ &target }, 4);
}

void CYUV2RGBShader::SetParams(float contrast, float black, bool limited)
{
  m_convMatrix.SetDestinationContrast(contrast * 0.02f)
      .SetDestinationBlack(black * 0.01f - 0.5f)
      .SetDestinationLimitedRange(limited);
}

void CYUV2RGBShader::SetColParams(AVColorSpace colSpace, int bits, bool limited, int texBits)
{
  if (colSpace == AVCOL_SPC_UNSPECIFIED)
  {
    if (m_sourceWidth > 1024 || m_sourceHeight >= 600)
      colSpace = AVCOL_SPC_BT709;
    else
      colSpace = AVCOL_SPC_BT470BG;
  }

  m_convMatrix.SetSourceColorSpace(colSpace)
      .SetSourceBitDepth(bits)
      .SetSourceLimitedRange(limited)
      .SetSourceTextureBitDepth(texBits);
}

void CYUV2RGBShader::PrepareParameters(CRenderBuffer* videoBuffer, CRect sourceRect, CPoint dest[])
{
  if (m_sourceRect != sourceRect
    || m_dest[0] != dest[0] || m_dest[1] != dest[1]
    || m_dest[2] != dest[2] || m_dest[3] != dest[3]
    || videoBuffer->GetWidth() != m_sourceWidth
    || videoBuffer->GetHeight() != m_sourceHeight)
  {
    m_sourceRect = sourceRect;
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];
    m_sourceWidth = videoBuffer->GetWidth();
    m_sourceHeight = videoBuffer->GetHeight();

    Vertex* v;
    CWinShader::LockVertexBuffer(reinterpret_cast<void**>(&v));

    v[0].x = m_dest[0].x;
    v[0].y = m_dest[0].y;
    v[0].z = 0.0f;
    v[0].tu = sourceRect.x1 / m_sourceWidth;
    v[0].tv = sourceRect.y1 / m_sourceHeight;
    v[0].tu2 = (sourceRect.x1 / 2.0f) / (m_sourceWidth>>1);
    v[0].tv2 = (sourceRect.y1 / 2.0f) / (m_sourceHeight>>1);

    v[1].x = m_dest[1].x;
    v[1].y = m_dest[1].y;
    v[1].z = 0.0f;
    v[1].tu = sourceRect.x2 / m_sourceWidth;
    v[1].tv = sourceRect.y1 / m_sourceHeight;
    v[1].tu2 = (sourceRect.x2 / 2.0f) / (m_sourceWidth>>1);
    v[1].tv2 = (sourceRect.y1 / 2.0f) / (m_sourceHeight>>1);

    v[2].x = m_dest[2].x;
    v[2].y = m_dest[2].y;
    v[2].z = 0.0f;
    v[2].tu = sourceRect.x2 / m_sourceWidth;
    v[2].tv = sourceRect.y2 / m_sourceHeight;
    v[2].tu2 = (sourceRect.x2 / 2.0f) / (m_sourceWidth>>1);
    v[2].tv2 = (sourceRect.y2 / 2.0f) / (m_sourceHeight>>1);

    v[3].x = m_dest[3].x;
    v[3].y = m_dest[3].y;
    v[3].z = 0.0f;
    v[3].tu = sourceRect.x1 / m_sourceWidth;
    v[3].tv = sourceRect.y2 / m_sourceHeight;
    v[3].tu2 = (sourceRect.x1 / 2.0f) / (m_sourceWidth>>1);
    v[3].tv2 = (sourceRect.y2 / 2.0f) / (m_sourceHeight>>1);

    CWinShader::UnlockVertexBuffer();
  }

  unsigned int texWidth = m_sourceWidth;
  if ( m_format == AV_PIX_FMT_UYVY422
    || m_format == AV_PIX_FMT_YUYV422)
    texWidth = texWidth >> 1;

  m_texSteps[0] = 1.0f / texWidth;
  m_texSteps[1] = 1.0f / m_sourceHeight;
}

void CYUV2RGBShader::SetShaderParameters(CRenderBuffer* videoBuffer)
{
  m_effect.SetTechnique("YUV2RGB_T");
  ID3D11ShaderResourceView* ppSRView[3] = {};
  for (unsigned i = 0, max_i = videoBuffer->GetViewCount(); i < max_i; i++)
    ppSRView[i] = reinterpret_cast<ID3D11ShaderResourceView*>(videoBuffer->GetView(i));
  m_effect.SetResources("g_Texture", ppSRView, videoBuffer->GetViewCount());
  m_effect.SetFloatArray("g_StepXY", m_texSteps.data(), m_texSteps.size());

  Matrix4 yuvMat = m_convMatrix.GetYuvMat();
  m_effect.SetMatrix("g_ColorMatrix", yuvMat.ToRaw());

  if (m_colorConversion)
  {
    Matrix4 primMat(m_convMatrix.GetPrimMat());

    // looks like FX11 doesn't like 3x3 matrix, so used 4x4 for its happiness
    m_effect.SetMatrix("g_primMat", primMat.ToRaw());
    m_effect.SetScalar("g_gammaSrc", m_convMatrix.GetGammaSrc());
    m_effect.SetScalar("g_gammaDstInv", 1 / m_convMatrix.GetGammaDst());
  }

  unsigned numPorts = 1;
  D3D11_VIEWPORT viewPort;
  DX::DeviceResources::Get()->GetD3DContext()->RSGetViewports(&numPorts, &viewPort);
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);
  if (m_pOutShader)
    m_pOutShader->ApplyEffectParameters(m_effect, m_sourceWidth, m_sourceHeight);
}

//==================================================================================

bool CConvolutionShader::ChooseKernelD3DFormat()
{
  if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
  {
    m_KernelFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    m_floattex = true;
    m_rgba = true;
  }
  else if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
  {
    m_KernelFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_floattex = false;
    m_rgba = true;
  }
  else if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
  {
    m_KernelFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    m_floattex = false;
    m_rgba = false;
  }
  else
    return false;

  return true;
}

bool CConvolutionShader::CreateHQKernel(ESCALINGMETHOD method)
{
  CConvolutionKernel kern(method, 256);
  void *kernel;
  int kernelSize;

  if (m_floattex)
  {
    float *data = kern.GetFloatPixels();
    const auto float16 = new uint16_t[kern.GetSize() * 4];

    XMConvertFloatToHalfStream(float16, sizeof(uint16_t), data, sizeof(float), kern.GetSize()*4);

    kernel = float16;
    kernelSize = sizeof(uint16_t)*kern.GetSize() * 4;
  }
  else
  {
    kernel = kern.GetUint8Pixels();
    kernelSize = sizeof(uint8_t)*kern.GetSize() * 4;
  }

  if (!m_HQKernelTexture.Create(kern.GetSize(), 1, 1, D3D11_USAGE_IMMUTABLE, m_KernelFormat, kernel, kernelSize))
  {
    CLog::LogF(LOGERROR, "Failed to create kernel texture.");
    return false;
  }

  if (m_floattex)
    delete[] static_cast<uint16_t*>(kernel);

  return true;
}
//==================================================================================
bool CConvolutionShader1Pass::Create(ESCALINGMETHOD method, const std::shared_ptr<COutputShader>& pOutputShader)
{
  m_pOutShader = pOutputShader;

  std::string effectString;
  switch(method)
  {
    case VS_SCALINGMETHOD_CUBIC_MITCHELL:
    case VS_SCALINGMETHOD_LANCZOS2:
    case VS_SCALINGMETHOD_SPLINE36_FAST:
    case VS_SCALINGMETHOD_LANCZOS3_FAST:
      effectString = "special://xbmc/system/shaders/convolution-4x4_d3d.fx";
      break;
    case VS_SCALINGMETHOD_SPLINE36:
    case VS_SCALINGMETHOD_LANCZOS3:
      effectString = "special://xbmc/system/shaders/convolution-6x6_d3d.fx";
      break;
    default:
      CLog::LogF(LOGERROR, "scaling method {} not supported.", method);
      return false;
  }

  if (!ChooseKernelD3DFormat())
  {
    CLog::LogF(LOGERROR, "failed to find a compatible texture format for the kernel.");
    return false;
  }

  CWinShader::CreateVertexBuffer(4, sizeof(Vertex));

  DefinesMap defines;
  if (m_floattex)
    defines["HAS_FLOAT_TEXTURE"] = "";
  if (m_rgba)
    defines["HAS_RGBA"] = "";

  if (m_pOutShader)
    m_pOutShader->GetDefines(defines);

  if(!LoadEffect(effectString, &defines))
  {
    CLog::LogF(LOGERROR, "Failed to load shader {}.", effectString);
    return false;
  }

  if (!CreateHQKernel(method))
    return false;

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  return CWinShader::CreateInputLayout(layout, ARRAYSIZE(layout));
}

void CConvolutionShader1Pass::Render(CD3DTexture& sourceTexture, CD3DTexture& target,
                                     CRect sourceRect, CRect destRect, bool useLimitRange)
{
  const unsigned int sourceWidth = sourceTexture.GetWidth();
  const unsigned int sourceHeight = sourceTexture.GetHeight();

  PrepareParameters(sourceWidth, sourceHeight, sourceRect, destRect);
  std::array<float, 2> texSteps = {1.0f / static_cast<float>(sourceWidth),
                                   1.0f / static_cast<float>(sourceHeight)};
  SetShaderParameters(sourceTexture, texSteps.data(), texSteps.size(), useLimitRange);
  Execute({ &target }, 4);
}

void CConvolutionShader1Pass::PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                                                CRect sourceRect, CRect destRect)
{
  if(m_sourceWidth != sourceWidth || m_sourceHeight != sourceHeight
  || m_sourceRect != sourceRect || m_destRect != destRect)
  {
    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceRect = sourceRect;
    m_destRect = destRect;

    Vertex* v;
    CWinShader::LockVertexBuffer(reinterpret_cast<void**>(&v));

    v[0].x = destRect.x1;
    v[0].y = destRect.y1;
    v[0].z = 0;
    v[0].tu = sourceRect.x1 / sourceWidth;
    v[0].tv = sourceRect.y1 / sourceHeight;

    v[1].x = destRect.x2;
    v[1].y = destRect.y1;
    v[1].z = 0;
    v[1].tu = sourceRect.x2 / sourceWidth;
    v[1].tv = sourceRect.y1 / sourceHeight;

    v[2].x = destRect.x2;
    v[2].y = destRect.y2;
    v[2].z = 0;
    v[2].tu = sourceRect.x2 / sourceWidth;
    v[2].tv = sourceRect.y2 / sourceHeight;

    v[3].x = destRect.x1;
    v[3].y = destRect.y2;
    v[3].z = 0;
    v[3].tu = sourceRect.x1 / sourceWidth;
    v[3].tv = sourceRect.y2 / sourceHeight;

    CWinShader::UnlockVertexBuffer();
  }
}

void CConvolutionShader1Pass::SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount, bool useLimitRange)
{
  m_effect.SetTechnique( "SCALER_T" );
  m_effect.SetTexture( "g_Texture",  sourceTexture ) ;
  m_effect.SetTexture( "g_KernelTexture", m_HQKernelTexture );
  m_effect.SetFloatArray("g_StepXY", texSteps, texStepsCount);
  unsigned numVP = 1;
  D3D11_VIEWPORT viewPort = {};
  DX::DeviceResources::Get()->GetD3DContext()->RSGetViewports(&numVP, &viewPort);
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);
  float colorRange[2] =
  {
    (useLimitRange ?  16.f / 255.f : 0.f),
    (useLimitRange ? 219.f / 255.f : 1.f),
  };
  m_effect.SetFloatArray("g_colorRange", colorRange, _countof(colorRange));
  if (m_pOutShader)
    m_pOutShader->ApplyEffectParameters(m_effect, sourceTexture.GetWidth(), sourceTexture.GetHeight());
}

//==================================================================================

bool CConvolutionShaderSeparable::Create(ESCALINGMETHOD method, const std::shared_ptr<COutputShader>& pOutputShader)
{
  m_pOutShader = pOutputShader;

  std::string effectString;
  switch(method)
  {
    case VS_SCALINGMETHOD_CUBIC_MITCHELL:
    case VS_SCALINGMETHOD_LANCZOS2:
    case VS_SCALINGMETHOD_SPLINE36_FAST:
    case VS_SCALINGMETHOD_LANCZOS3_FAST:
      effectString = "special://xbmc/system/shaders/convolutionsep-4x4_d3d.fx";
      break;
    case VS_SCALINGMETHOD_SPLINE36:
    case VS_SCALINGMETHOD_LANCZOS3:
      effectString = "special://xbmc/system/shaders/convolutionsep-6x6_d3d.fx";
      break;
    default:
      CLog::LogF(LOGERROR, "scaling method {} not supported.", method);
      return false;
  }

  if (!ChooseIntermediateD3DFormat())
  {
    CLog::LogF(LOGERROR, "failed to find a compatible texture format for the intermediate render target.");
    return false;
  }

  if (!ChooseKernelD3DFormat())
  {
    CLog::LogF(LOGERROR, "failed to find a compatible texture format for the kernel.");
    return false;
  }

  CWinShader::CreateVertexBuffer(8, sizeof(Vertex));

  DefinesMap defines;
  if (m_floattex)
    defines["HAS_FLOAT_TEXTURE"] = "";
  if (m_rgba)
    defines["HAS_RGBA"] = "";

  if (m_pOutShader)
    m_pOutShader->GetDefines(defines);

  if(!LoadEffect(effectString, &defines))
  {
    CLog::LogF(LOGERROR, "Failed to load shader {}.", effectString);
    return false;
  }

  if (!CreateHQKernel(method))
    return false;

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  return CWinShader::CreateInputLayout(layout, ARRAYSIZE(layout));
}

void CConvolutionShaderSeparable::Render(CD3DTexture& sourceTexture, CD3DTexture& target,
                                         CRect sourceRect, CRect destRect, bool useLimitRange)
{
  const unsigned int sourceWidth = sourceTexture.GetWidth();
  const unsigned int sourceHeight = sourceTexture.GetHeight();

  const unsigned int destWidth = target.GetWidth();
  const unsigned int destHeight = target.GetHeight();

    if(m_destWidth != destWidth || m_sourceHeight != sourceHeight)
    CreateIntermediateRenderTarget(destWidth, sourceHeight);

  PrepareParameters(sourceWidth, sourceHeight, destWidth, destHeight, sourceRect, destRect);
  float texSteps[] =
  {
    1.0f / static_cast<float>(sourceWidth),
    1.0f / static_cast<float>(sourceHeight),
    1.0f / static_cast<float>(destWidth),
    1.0f / static_cast<float>(sourceHeight)
  };
  SetShaderParameters(sourceTexture, texSteps, 4, useLimitRange);

  Execute({ &m_IntermediateTarget, &target }, 4);
}

bool CConvolutionShaderSeparable::ChooseIntermediateD3DFormat()
{
  const D3D11_FORMAT_SUPPORT usage = D3D11_FORMAT_SUPPORT_RENDER_TARGET;

  // Need a float texture, as the output of the first pass can contain negative values.
  if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, usage))
    m_IntermediateFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
  else if (DX::Windowing()->IsFormatSupport(DXGI_FORMAT_R32G32B32A32_FLOAT, usage))
    m_IntermediateFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
  else
  {
    CLog::LogF(LOGINFO, "no float format available for the intermediate render target");
    return false;
  }

  CLog::LogF(LOGDEBUG, "format {}", DX::DXGIFormatToString(m_IntermediateFormat));
  return true;
}

bool CConvolutionShaderSeparable::CreateIntermediateRenderTarget(unsigned int width, unsigned int height)
{
  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  if (!m_IntermediateTarget.Create(width, height, 1, D3D11_USAGE_DEFAULT, m_IntermediateFormat))
  {
    CLog::LogF(LOGERROR, "render target creation failed.");
    return false;
  }
  return true;
}

bool CConvolutionShaderSeparable::ClearIntermediateRenderTarget()
{
  float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  DX::DeviceResources::Get()->GetD3DContext()->ClearRenderTargetView(m_IntermediateTarget.GetRenderTarget(), color);
  return true;
}

void CConvolutionShaderSeparable::PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                                           unsigned int destWidth, unsigned int destHeight,
                                           CRect sourceRect, CRect destRect)
{
  if(m_sourceWidth != sourceWidth || m_sourceHeight != sourceHeight
  || m_destWidth != destWidth || m_destHeight != destHeight
  || m_sourceRect != sourceRect || m_destRect != destRect)
  {
    ClearIntermediateRenderTarget();

    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_destWidth = destWidth;
    m_destHeight = destHeight;
    m_sourceRect = sourceRect;
    m_destRect = destRect;

    Vertex* v;
    CWinShader::LockVertexBuffer(reinterpret_cast<void**>(&v));

    // Alter rectangles the destination rectangle exceeds the intermediate target width when zooming and causes artifacts.
    // Work on the parameters rather than the members to avoid disturbing the parameter change detection the next time the function is called
    const CRect target(0, 0, static_cast<float>(destWidth), static_cast<float>(destHeight));
    CWIN32Util::CropSource(sourceRect, destRect, target);

    // Manipulate the coordinates to work only on the active parts of the textures,
    // and therefore avoid the need to clear surfaces/render targets

    // Pass 1:
    // Horizontal dimension: crop/zoom, so that it is completely done with the convolution shader. Scaling to display width in pass1 and
    // cropping/zooming in pass 2 would use bilinear in pass2, which we don't want.
    // Vertical dimension: crop using sourceRect to save memory bandwidth for high zoom values, but don't stretch/shrink in any way in this pass.

    v[0].x = 0;
    v[0].y = 0;
    v[0].z = 0;
    v[0].tu = sourceRect.x1 / sourceWidth;
    v[0].tv = sourceRect.y1 / sourceHeight;

    v[1].x = destRect.x2 - destRect.x1;
    v[1].y = 0;
    v[1].z = 0;
    v[1].tu = sourceRect.x2 / sourceWidth;
    v[1].tv = sourceRect.y1 / sourceHeight;

    v[2].x = destRect.x2 - destRect.x1;
    v[2].y = sourceRect.y2 - sourceRect.y1;
    v[2].z = 0;
    v[2].tu = sourceRect.x2 / sourceWidth;
    v[2].tv = sourceRect.y2 / sourceHeight;

    v[3].x = 0;
    v[3].y = sourceRect.y2 - sourceRect.y1;
    v[3].z = 0;
    v[3].tu = sourceRect.x1 / sourceWidth;
    v[3].tv = sourceRect.y2 / sourceHeight;

    // Pass 2: pass the horizontal data untouched, resize vertical dimension for final result.

    v[4].x = destRect.x1;
    v[4].y = destRect.y1;
    v[4].z = 0;
    v[4].tu = 0;
    v[4].tv = 0;

    v[5].x = destRect.x2;
    v[5].y = destRect.y1;
    v[5].z = 0;
    v[5].tu = (destRect.x2 - destRect.x1) / destWidth;
    v[5].tv = 0;

    v[6].x = destRect.x2;
    v[6].y = destRect.y2;
    v[6].z = 0;
    v[6].tu = (destRect.x2 - destRect.x1) / destWidth;
    v[6].tv = (sourceRect.y2 - sourceRect.y1) / sourceHeight;

    v[7].x = destRect.x1;
    v[7].y = destRect.y2;
    v[7].z = 0;
    v[7].tu = 0;
    v[7].tv = (sourceRect.y2 - sourceRect.y1) / sourceHeight;

    CWinShader::UnlockVertexBuffer();
  }
}

void CConvolutionShaderSeparable::SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount, bool useLimitRange)
{
  m_effect.SetTechnique( "SCALER_T" );
  m_effect.SetTexture( "g_Texture",  sourceTexture );
  m_effect.SetTexture( "g_KernelTexture", m_HQKernelTexture );
  m_effect.SetFloatArray("g_StepXY", texSteps, texStepsCount);
  float colorRange[2] =
  {
    (useLimitRange ?  16.f / 255.f : 0.f),
    (useLimitRange ? 219.f / 255.f : 1.f)
  };
  m_effect.SetFloatArray("g_colorRange", colorRange, _countof(colorRange));
  if (m_pOutShader)
    m_pOutShader->ApplyEffectParameters(m_effect, sourceTexture.GetWidth(), sourceTexture.GetHeight());
}

void CConvolutionShaderSeparable::SetStepParams(unsigned iPass)
{
  ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();

  CD3D11_VIEWPORT viewPort = CD3D11_VIEWPORT(
    0.0f,
    0.0f,
    static_cast<float>(m_target->GetWidth()),
    static_cast<float>(m_target->GetHeight()));

  if (iPass == 0)
  {
    // reset scissor
    DX::Windowing()->ResetScissors();
  }
  else if (iPass == 1)
  {
    // at the second pass m_IntermediateTarget is a source of data
    m_effect.SetTexture("g_Texture", m_IntermediateTarget);
    // restore scissor
    DX::Windowing()->SetScissors(CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors()));
  }
  // setting view port
  pContext->RSSetViewports(1, &viewPort);
  // pass viewport dimension to the shaders
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);
}

//==========================================================
#define SHADER_SOURCE(...) #__VA_ARGS__

bool CTestShader::Create()
{
  std::string strShader = SHADER_SOURCE(
    float4 TEST() : SV_TARGET
    {
      return float4(0.0, 0.0, 0.0, 0.0);
    }

    technique11 TEST_T
    {
      pass P0
      {
        SetPixelShader(CompileShader(ps_4_0_level_9_1, TEST()));
      }
    };
  );

  if (!m_effect.Create(strShader, nullptr))
  {
    CLog::LogF(LOGERROR, "Failed to create test shader: {}", strShader);
    return false;
  }
  return true;
}
