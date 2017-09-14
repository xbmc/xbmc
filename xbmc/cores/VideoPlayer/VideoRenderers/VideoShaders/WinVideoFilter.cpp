/*
 *      Copyright (C) 2007-2013 Team XBMC
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

#include "ConvolutionKernels.h"
#include "dither.h"
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "platform/win32/WIN32Util.h"
#include "Util.h"
#include "utils/log.h"
#include "VideoRenderers/WinRenderBuffer.h"
#include "windowing/WindowingFactory.h"
#include "WinVideoFilter.h"
#include "YUVMatrix.h"

#include <DirectXPackedVector.h>
#include <map>

using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

CYUV2RGBMatrix::CYUV2RGBMatrix()
{
  m_NeedRecalc = true;
  m_blacklevel = 0.0f;
  m_contrast = 0.0f;
  m_flags = 0;
  m_limitedRange = false;
  m_format = BUFFER_FMT_NONE;
}

void CYUV2RGBMatrix::SetParameters(float contrast, float blacklevel, unsigned int flags, EBufferFormat format)
{
  if (m_contrast != contrast)
  {
    m_NeedRecalc = true;
    m_contrast = contrast;
  }
  if (m_blacklevel != blacklevel)
  {
    m_NeedRecalc = true;
    m_blacklevel = blacklevel;
  }
  if (m_flags != flags)
  {
    m_NeedRecalc = true;
    m_flags = flags;
  }
  if (m_format != format)
  {
    m_NeedRecalc = true;
    m_format = format;
  }
  if (m_limitedRange != g_Windowing.UseLimitedColor())
  {
    m_NeedRecalc = true;
    m_limitedRange = g_Windowing.UseLimitedColor();
  }
}

XMFLOAT4X4* CYUV2RGBMatrix::Matrix()
{
  if (m_NeedRecalc)
  {
    TransformMatrix matrix;
    EShaderFormat fmt = SHADER_NONE;
    if (m_format == BUFFER_FMT_YUV420P10)
      fmt = SHADER_YV12_10;
    CalculateYUVMatrix(matrix, m_flags, fmt, m_blacklevel, m_contrast, m_limitedRange);

    m_mat._11 = matrix.m[0][0];
    m_mat._12 = matrix.m[1][0];
    m_mat._13 = matrix.m[2][0];
    m_mat._14 = 0.0f;
    m_mat._21 = matrix.m[0][1];
    m_mat._22 = matrix.m[1][1];
    m_mat._23 = matrix.m[2][1];
    m_mat._24 = 0.0f;
    m_mat._31 = matrix.m[0][2];
    m_mat._32 = matrix.m[1][2];
    m_mat._33 = matrix.m[2][2];
    m_mat._34 = 0.0f;
    m_mat._41 = matrix.m[0][3];
    m_mat._42 = matrix.m[1][3];
    m_mat._43 = matrix.m[2][3];
    m_mat._44 = 1.0f;

    m_NeedRecalc = false;
  }
  return &m_mat;
}

//===================================================================

CWinShader::~CWinShader()
{
  if (m_effect.Get())
    m_effect.Release();
  if (m_vb.Get())
    m_vb.Release();
  if (m_ib.Get())
    m_ib.Release();
}

bool CWinShader::CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize)
{
  if (!m_vb.Create(D3D11_BIND_VERTEX_BUFFER, vertCount, vertSize, DXGI_FORMAT_UNKNOWN, D3D11_USAGE_DYNAMIC))
    return false;

  uint16_t id[4] = { 3, 0, 2, 1 };
  if (!m_ib.Create(D3D11_BIND_INDEX_BUFFER, ARRAYSIZE(id), sizeof(uint16_t), DXGI_FORMAT_R16_UINT, D3D11_USAGE_IMMUTABLE, id))
    return false;

  m_vbsize = vertCount * vertSize;
  m_vertsize = vertSize;

  return true;
}

bool CWinShader::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC *layout, unsigned numElements)
{
  D3DX11_PASS_DESC desc = {};
  if (FAILED(m_effect.Get()->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&desc)))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to get first pass description.");
    return false;
  }

  return S_OK == g_Windowing.Get3D11Device()->CreateInputLayout(layout, numElements, desc.pIAInputSignature, desc.IAInputSignatureSize, &m_inputLayout);
}

void CWinShader::SetTarget(CD3DTexture* target)
{
  m_target = target;
  if (m_target)
    g_Windowing.Get3D11Context()->OMSetRenderTargets(1, target->GetAddressOfRTV(), nullptr);
}

bool CWinShader::LockVertexBuffer(void **data)
{
  if (!m_vb.Map(data))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock vertex buffer");
    return false;
  }
  return true;
}

bool CWinShader::UnlockVertexBuffer()
{
  if (!m_vb.Unmap())
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock vertex buffer");
    return false;
  }
  return true;
}

bool CWinShader::LoadEffect(const std::string& filename, DefinesMap* defines)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - loading shader %s", filename.c_str());

  XFILE::CFileStream file;
  if(!file.Open(filename))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to open file %s", filename.c_str());
    return false;
  }

  std::string pStrEffect;
  getline(file, pStrEffect, '\0');

  if (!m_effect.Create(pStrEffect, defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__" %s failed", pStrEffect.c_str());
    return false;
  }

  return true;
}

bool CWinShader::Execute(const std::vector<CD3DTexture*> &targets, unsigned int vertexIndexStep)
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ComPtr<ID3D11RenderTargetView> oldRT;

  // The render target will be overridden: save the caller's original RT
  if (!targets.empty())
    pContext->OMGetRenderTargets(1, &oldRT, nullptr);

  UINT cPasses;
  if (!m_effect.Begin(&cPasses, 0))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to begin d3d effect");
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

  for (UINT iPass = 0; iPass < cPasses; iPass++)
  {
    SetTarget(targets.size() > iPass ? targets.at(iPass) : nullptr);
    SetStepParams(iPass);

    if (!m_effect.BeginPass(iPass))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - failed to begin d3d effect pass");
      break;
    }

    pContext->DrawIndexed(4, 0, iPass * vertexIndexStep);

    if (!m_effect.EndPass())
      CLog::Log(LOGERROR, __FUNCTION__" - failed to end d3d effect pass");

    CD3DHelper::PSClearShaderResources(pContext);
  }
  if (!m_effect.End())
    CLog::Log(LOGERROR, __FUNCTION__" - failed to end d3d effect");

  if (oldRT)
    pContext->OMSetRenderTargets(1, oldRT.GetAddressOf(), nullptr);

  return true;
}

//==================================================================================

COutputShader::~COutputShader()
{
  m_clutSize = 0;
}

void COutputShader::ApplyEffectParameters(CD3DEffect &effect, unsigned sourceWidth, unsigned sourceHeight)
{
  if (m_useCLUT)
  {
    float clut_params[2] = { 0.0f, 0.0f };
    if (HasCLUT())
    {
      clut_params[0] = (m_clutSize - 1) / static_cast<float>(m_clutSize);
      clut_params[1] = 0.5f / static_cast<float>(m_clutSize);
    };
    effect.SetResources("m_CLUT", &m_pCLUTView, 1);
    effect.SetFloatArray("m_CLUTParams", clut_params, 2);
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
}

void COutputShader::GetDefines(DefinesMap& map) const
{
  if (m_useCLUT)
  {
    map["KODI_3DLUT"] = "";
  }
  if (m_useDithering)
  {
    map["KODI_DITHER"] = "";
  }
}

bool COutputShader::Create(bool useCLUT, bool useDithering, int ditherDepth)
{
  m_useCLUT = useCLUT;
  m_ditherDepth = ditherDepth;

  CWinShader::CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

  if (useDithering)
    CreateDitherView();

  DefinesMap defines;
  defines["KODI_OUTPUT_T"] = "";
  GetDefines(defines);

  std::string effectString("special://xbmc/system/shaders/output_d3d.fx");

  if (!LoadEffect(effectString, &defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
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

void COutputShader::Render(CD3DTexture& sourceTexture, unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4]
                         , CD3DTexture *target, unsigned range, float contrast, float brightness)
{
  PrepareParameters(sourceWidth, sourceHeight, sourceRect, points);
  SetShaderParameters(sourceTexture, range, contrast, brightness);
  Execute({ target }, 4);
}

void COutputShader::Render(CD3DTexture &sourceTexture, unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, CRect destRect
                         , CD3DTexture *target, unsigned range, float contrast, float brightness)
{
  CPoint points[] =
  {
    { destRect.x1, destRect.y1 },
    { destRect.x2, destRect.y1 },
    { destRect.x2, destRect.y2 },
    { destRect.x1, destRect.y2 },
  };
  Render(sourceTexture, sourceWidth, sourceHeight, sourceRect, points, target, range, contrast, brightness);
}

void COutputShader::SetCLUT(int clutSize, ID3D11ShaderResourceView* pCLUTView)
{
  m_clutSize = clutSize;
  m_pCLUTView = pCLUTView;
}

bool COutputShader::CreateCLUTView(int clutSize, uint16_t* clutData, bool isRGB, ID3D11ShaderResourceView** ppCLUTView)
{
  if (!clutSize || !clutData)
    return false;

  uint16_t* cData;
  if (isRGB)
  {
    // repack data to RGBA
    unsigned lutsamples = clutSize * clutSize * clutSize;
    cData = reinterpret_cast<uint16_t*>(_aligned_malloc(lutsamples * sizeof(uint16_t) * 4, 16));
    uint16_t* rgba = static_cast<uint16_t*>(cData);
    for (unsigned i = 0; i < lutsamples - 1; ++i, rgba += 4, clutData += 3)
    {
      *(uint64_t*)rgba = *(uint64_t*)clutData;
    }
    // and last one
    rgba[0] = clutData[0]; rgba[1] = clutData[1]; rgba[2] = clutData[2]; rgba[3] = 0xFFFF;
  }
  else
    cData = clutData;

  ID3D11Device* pDevice = g_Windowing.Get3D11Device();
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();
  CD3D11_TEXTURE3D_DESC txDesc(DXGI_FORMAT_R16G16B16A16_UNORM, clutSize, clutSize, clutSize, 1,
                               D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

  ComPtr<ID3D11Texture3D> pCLUTTex;
  D3D11_SUBRESOURCE_DATA texData;
  texData.pSysMem = cData;
  texData.SysMemPitch = clutSize * sizeof(uint16_t) * 4;
  texData.SysMemSlicePitch = texData.SysMemPitch * clutSize;

  HRESULT hr = pDevice->CreateTexture3D(&txDesc, &texData, &pCLUTTex);
  if (isRGB)
    _aligned_free(cData);

  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s: unable to create 3dlut texture cube.");
    return false;
  }

  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE3D, DXGI_FORMAT_R16G16B16A16_UNORM, 0, 1);
  hr = pDevice->CreateShaderResourceView(pCLUTTex.Get(), &srvDesc, ppCLUTView);
  pContext->Flush();

  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s: unable to create view for 3dlut texture cube.");
    return false;
  }

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

    CUSTOMVERTEX* v;
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

  UINT numPorts = 1;
  D3D11_VIEWPORT viewPort;
  g_Windowing.Get3D11Context()->RSGetViewports(&numPorts, &viewPort);
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);

  float params[3] = { static_cast<float>(range), contrast, brightness };
  m_effect.SetFloatArray("m_params", params, 3);

  ApplyEffectParameters(m_effect, sourceTexture.GetWidth(), sourceTexture.GetHeight());
}

void COutputShader::CreateDitherView()
{
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();

  CD3D11_TEXTURE2D_DESC txDesc(DXGI_FORMAT_R16_UNORM, dither_size, dither_size, 1, 1);
  D3D11_SUBRESOURCE_DATA resData;
  resData.pSysMem = dither_matrix;
  resData.SysMemPitch = dither_size * sizeof(uint16_t);
  resData.SysMemSlicePitch = resData.SysMemPitch * dither_size;

  ComPtr<ID3D11Texture2D> pDitherTex;
  HRESULT hr = pDevice->CreateTexture2D(&txDesc, &resData, &pDitherTex);

  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s: unable to create 3dlut texture cube.");
    m_useDithering = false;
    return;
  }

  CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R16_UNORM, 0, 1);
  hr = pDevice->CreateShaderResourceView(pDitherTex.Get(), &srvDesc, &m_pDitherView);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s: unable to create view for 3dlut texture cube.");
    m_useDithering = false;
    return;
  }
  m_useDithering = true;
}

//==================================================================================

bool CYUV2RGBShader::Create(EBufferFormat fmt, COutputShader *pCLUT)
{
  m_format = fmt;
  m_pOutShader = pCLUT;

  DefinesMap defines;

  switch (fmt)
  {
  case BUFFER_FMT_YUV420P:
  case BUFFER_FMT_YUV420P10:
  case BUFFER_FMT_YUV420P16:
    defines["XBMC_YV12"] = "";
    break;
  case BUFFER_FMT_D3D11_BYPASS:
  case BUFFER_FMT_D3D11_NV12:
  case BUFFER_FMT_D3D11_P010:
  case BUFFER_FMT_D3D11_P016:
  case BUFFER_FMT_NV12:
    defines["XBMC_NV12"] = "";
    // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
    if (!g_Windowing.IsFormatSupport(DXGI_FORMAT_R8G8_UNORM, D3D11_FORMAT_SUPPORT_TEXTURE2D))
      defines["NV12_SNORM_UV"] = "";
    break;
  case BUFFER_FMT_UYVY422:
    defines["XBMC_UYVY"] = "";
    break;
  case BUFFER_FMT_YUYV422:
    defines["XBMC_YUY2"] = "";
    break;
  default:
    return false;
  }

  if (m_pOutShader)
    m_pOutShader->GetDefines(defines);

  std::string effectString = "special://xbmc/system/shaders/yuv2rgb_d3d.fx";

  if(!LoadEffect(effectString, &defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
    return false;
  }

  CWinShader::CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));
  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  if (!CWinShader::CreateInputLayout(layout, ARRAYSIZE(layout)))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create input layout for Input Assembler.");
    return false;
  }
  return true;
}

void CYUV2RGBShader::Render(CRect sourceRect, CPoint dest[], float contrast, float brightness, CRenderBuffer* videoBuffer, CD3DTexture *target)
{
  PrepareParameters(videoBuffer, sourceRect, dest, contrast, brightness);
  SetShaderParameters(videoBuffer);
  Execute({ target }, 4);
}

CYUV2RGBShader::CYUV2RGBShader()
  : m_sourceWidth(0)
  , m_sourceHeight(0)
  , m_format(BUFFER_FMT_NONE)
  , m_pOutShader(nullptr)
{
  memset(&m_texSteps, 0, sizeof(m_texSteps));
}

CYUV2RGBShader::~CYUV2RGBShader()
{
}

void CYUV2RGBShader::PrepareParameters(CRenderBuffer* videoBuffer, CRect sourceRect, CPoint dest[],
                                       float contrast, float brightness)
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

    CUSTOMVERTEX* v;
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
  if ( m_format == BUFFER_FMT_UYVY422
    || m_format == BUFFER_FMT_YUYV422)
    texWidth = texWidth >> 1;

  m_texSteps[0] = 1.0f / texWidth;
  m_texSteps[1] = 1.0f / m_sourceHeight;

  m_matrix.SetParameters(contrast * 0.02f,
                         brightness * 0.01f - 0.5f,
                         videoBuffer->flags,
                         m_format);
}

void CYUV2RGBShader::SetShaderParameters(CRenderBuffer* videoBuffer)
{
  m_effect.SetTechnique("YUV2RGB_T");
  ID3D11ShaderResourceView* ppSRView[3] = {};
  for (unsigned i = 0, max_i = videoBuffer->GetActivePlanes(); i < max_i; i++)
    ppSRView[i] = reinterpret_cast<ID3D11ShaderResourceView*>(videoBuffer->GetView(i));
  m_effect.SetResources("g_Texture", ppSRView, videoBuffer->GetActivePlanes());
  m_effect.SetMatrix("g_ColorMatrix", m_matrix.Matrix());
  m_effect.SetFloatArray("g_StepXY", m_texSteps, ARRAY_SIZE(m_texSteps));

  UINT numPorts = 1;
  D3D11_VIEWPORT viewPort;
  g_Windowing.Get3D11Context()->RSGetViewports(&numPorts, &viewPort);
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);
  if (m_pOutShader)
    m_pOutShader->ApplyEffectParameters(m_effect, m_sourceWidth, m_sourceHeight);
}

//==================================================================================

CConvolutionShader::CConvolutionShader() : CWinShader()
  , m_KernelFormat(DXGI_FORMAT_UNKNOWN)
  , m_floattex(false)
  , m_rgba(false)
  , m_pOutShader(nullptr)
{
}

CConvolutionShader::~CConvolutionShader()
{
  if(m_HQKernelTexture.Get())
    m_HQKernelTexture.Release();
}

bool CConvolutionShader::ChooseKernelD3DFormat()
{
  if (g_Windowing.IsFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
  {
    m_KernelFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    m_floattex = true;
    m_rgba = true;
  }
  else if (g_Windowing.IsFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
  {
    m_KernelFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_floattex = false;
    m_rgba = true;
  }
  else if (g_Windowing.IsFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
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
  void *kernelVals;
  int kernelValsSize;

  if (m_floattex)
  {
    float *rawVals = kern.GetFloatPixels();
    HALF* float16Vals = new HALF[kern.GetSize() * 4];

    XMConvertFloatToHalfStream(float16Vals, sizeof(HALF), rawVals, sizeof(float), kern.GetSize()*4);

    kernelVals = float16Vals;
    kernelValsSize = sizeof(HALF)*kern.GetSize() * 4;
  }
  else
  {
    kernelVals = kern.GetUint8Pixels();
    kernelValsSize = sizeof(uint8_t)*kern.GetSize() * 4;
  }

  if (!m_HQKernelTexture.Create(kern.GetSize(), 1, 1, D3D11_USAGE_IMMUTABLE, m_KernelFormat, kernelVals, kernelValsSize))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create kernel texture.");
    return false;
  }

  if (m_floattex)
    delete[] (HALF*)kernelVals;

  return true;
}
//==================================================================================
bool CConvolutionShader1Pass::Create(ESCALINGMETHOD method, COutputShader *pCLUT)
{
  m_pOutShader = pCLUT;

  std::string effectString;
  switch(method)
  {
    case VS_SCALINGMETHOD_CUBIC:
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
      CLog::Log(LOGERROR, __FUNCTION__": scaling method %d not supported.", method);
      return false;
  }

  if (!ChooseKernelD3DFormat())
  {
    CLog::Log(LOGERROR, __FUNCTION__": failed to find a compatible texture format for the kernel.");
    return false;
  }

  CWinShader::CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

  DefinesMap defines;
  if (m_floattex)
    defines["HAS_FLOAT_TEXTURE"] = "";
  if (m_rgba)
    defines["HAS_RGBA"] = "";

  if (m_pOutShader)
    m_pOutShader->GetDefines(defines);

  if(!LoadEffect(effectString, &defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
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

void CConvolutionShader1Pass::Render(CD3DTexture &sourceTexture,
                                     unsigned int sourceWidth, unsigned int sourceHeight,
                                     unsigned int destWidth, unsigned int destHeight,
                                     CRect sourceRect, CRect destRect, bool useLimitRange, 
                                     CD3DTexture *target)
{
  PrepareParameters(sourceWidth, sourceHeight, sourceRect, destRect);
  float texSteps[] = { 1.0f/(float)sourceWidth, 1.0f/(float)sourceHeight};
  SetShaderParameters(sourceTexture, &texSteps[0], ARRAY_SIZE(texSteps), useLimitRange);
  Execute({ target }, 4);
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

    CUSTOMVERTEX* v;
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
  UINT numVP = 1;
  D3D11_VIEWPORT viewPort = {};
  g_Windowing.Get3D11Context()->RSGetViewports(&numVP, &viewPort);
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

CConvolutionShaderSeparable::CConvolutionShaderSeparable() : CConvolutionShader()
  , m_IntermediateFormat(DXGI_FORMAT_UNKNOWN)
  , m_sourceWidth(-1)
  , m_sourceHeight(-1)
  , m_destWidth(-1)
  , m_destHeight(-1)
{
}

bool CConvolutionShaderSeparable::Create(ESCALINGMETHOD method, COutputShader *pCLUT)
{
  m_pOutShader = pCLUT;

  std::string effectString;
  switch(method)
  {
    case VS_SCALINGMETHOD_CUBIC:
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
      CLog::Log(LOGERROR, __FUNCTION__": scaling method %d not supported.", method);
      return false;
  }

  if (!ChooseIntermediateD3DFormat())
  {
    CLog::Log(LOGERROR, __FUNCTION__": failed to find a compatible texture format for the intermediate render target.");
    return false;
  }

  if (!ChooseKernelD3DFormat())
  {
    CLog::Log(LOGERROR, __FUNCTION__": failed to find a compatible texture format for the kernel.");
    return false;
  }

  CWinShader::CreateVertexBuffer(8, sizeof(CUSTOMVERTEX));

  DefinesMap defines;
  if (m_floattex)
    defines["HAS_FLOAT_TEXTURE"] = "";
  if (m_rgba)
    defines["HAS_RGBA"] = "";

  if (m_pOutShader)
    m_pOutShader->GetDefines(defines);

  if(!LoadEffect(effectString, &defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
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

void CConvolutionShaderSeparable::Render(CD3DTexture &sourceTexture,
                                         unsigned int sourceWidth, unsigned int sourceHeight,
                                         unsigned int destWidth, unsigned int destHeight,
                                         CRect sourceRect, CRect destRect, bool useLimitRange,
                                         CD3DTexture *target)
{
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

  Execute({ &m_IntermediateTarget, target }, 4);

  // we changed view port, so we need to restore our real viewport.
  g_Windowing.RestoreViewPort();
}

CConvolutionShaderSeparable::~CConvolutionShaderSeparable()
{
  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
}

bool CConvolutionShaderSeparable::ChooseIntermediateD3DFormat()
{
  D3D11_FORMAT_SUPPORT usage = D3D11_FORMAT_SUPPORT_RENDER_TARGET;

  // Need a float texture, as the output of the first pass can contain negative values.
  if      (g_Windowing.IsFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, usage)) m_IntermediateFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
  else if (g_Windowing.IsFormatSupport(DXGI_FORMAT_R32G32B32A32_FLOAT, usage)) m_IntermediateFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
  else
  {
    CLog::Log(LOGNOTICE, __FUNCTION__": no float format available for the intermediate render target");
    return false;
  }

  CLog::Log(LOGDEBUG, __FUNCTION__": format %i", m_IntermediateFormat);

  return true;
}

bool CConvolutionShaderSeparable::CreateIntermediateRenderTarget(unsigned int width, unsigned int height)
{
  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  if (!m_IntermediateTarget.Create(width, height, 1, D3D11_USAGE_DEFAULT, m_IntermediateFormat))
  {
    CLog::Log(LOGERROR, __FUNCTION__": render target creation failed.");
    return false;
  }
  return true;
}

bool CConvolutionShaderSeparable::ClearIntermediateRenderTarget()
{
  float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  g_Windowing.Get3D11Context()->ClearRenderTargetView(m_IntermediateTarget.GetRenderTarget(), color);
  return true;
}

void CConvolutionShaderSeparable::PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                                           unsigned int destWidth, unsigned int destHeight,
                                           CRect sourceRect,
                                           CRect destRect)
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

    CUSTOMVERTEX* v;
    CWinShader::LockVertexBuffer((void**)&v);

    // Alter rectangles the destination rectangle exceeds the intermediate target width when zooming and causes artifacts.
    // Work on the parameters rather than the members to avoid disturbing the parameter change detection the next time the function is called
    CRect tgtRect(0, 0, static_cast<float>(destWidth), static_cast<float>(destHeight));
    CWIN32Util::CropSource(sourceRect, destRect, tgtRect);

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

void CConvolutionShaderSeparable::SetStepParams(UINT iPass)
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();

  CD3D11_VIEWPORT viewPort = CD3D11_VIEWPORT(
    0.0f, 
    0.0f,
    static_cast<float>(m_target->GetWidth()),
    static_cast<float>(m_target->GetHeight()));

  if (iPass == 0)
  {
    // reset scissor
    g_Windowing.ResetScissors();
  }
  else if (iPass == 1)
  {
    // at the second pass m_IntermediateTarget is a source of data
    m_effect.SetTexture("g_Texture", m_IntermediateTarget);
    // restore scissor
    g_Windowing.SetScissors(g_graphicsContext.StereoCorrection(g_graphicsContext.GetScissors()));
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
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create test shader: %s", strShader.c_str());
    return false;
  }
  return true;
}
