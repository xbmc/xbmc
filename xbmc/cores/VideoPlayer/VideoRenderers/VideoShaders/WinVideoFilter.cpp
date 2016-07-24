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

#ifdef HAS_DX

#include "WinVideoFilter.h"
#include "windowing/WindowingFactory.h"
#include "../../../../utils/log.h"
#include "../../../../FileSystem/File.h"
#include <map>
#include "ConvolutionKernels.h"
#include <DirectXPackedVector.h>
#include "FileSystem/File.h"
#include "guilib/GraphicContext.h"
#include "Util.h"
#include "utils/log.h"
#include "platform/win32/WIN32Util.h"
#include "windowing/WindowingFactory.h"
#include "YUV2RGBShader.h"
#include <map>

using namespace DirectX::PackedVector;

CYUV2RGBMatrix::CYUV2RGBMatrix()
{
  m_NeedRecalc = true;
  m_blacklevel = 0.0f;
  m_contrast = 0.0f;
  m_flags = 0;
  m_limitedRange = false;
  m_format = RENDER_FMT_NONE;
}

void CYUV2RGBMatrix::SetParameters(float contrast, float blacklevel, unsigned int flags, ERenderFormat format)
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
    CalculateYUVMatrix(matrix, m_flags, m_format, m_blacklevel, m_contrast, m_limitedRange);

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
  SAFE_RELEASE(m_inputLayout);
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

bool CWinShader::Execute(std::vector<ID3D11RenderTargetView*> *vecRT, unsigned int vertexIndexStep)
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11RenderTargetView* oldRT = nullptr;

  // The render target will be overriden: save the caller's original RT
  if (vecRT != nullptr && !vecRT->empty())
    pContext->OMGetRenderTargets(1, &oldRT, nullptr);

  UINT cPasses, iPass;
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
  pContext->IASetInputLayout(m_inputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  for (iPass = 0; iPass < cPasses; iPass++)
  {
    if (vecRT != nullptr && vecRT->size() > iPass)
      pContext->OMSetRenderTargets(1, &vecRT->at(iPass), nullptr);

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

  if (oldRT != nullptr)
  {
    pContext->OMSetRenderTargets(1, &oldRT, nullptr);
    oldRT->Release();
  }
  return true;
}

//==================================================================================

bool CYUV2RGBShader::Create(unsigned int sourceWidth, unsigned int sourceHeight, ERenderFormat fmt)
{
  CWinShader::CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

  m_sourceWidth = sourceWidth;
  m_sourceHeight = sourceHeight;
  m_format = fmt;

  unsigned int texWidth;

  DefinesMap defines;

  switch (fmt)
  {
  case RENDER_FMT_YUV420P:
  case RENDER_FMT_YUV420P10:
  case RENDER_FMT_YUV420P16:
    defines["XBMC_YV12"] = "";
    texWidth = sourceWidth;
    break;
  case RENDER_FMT_NV12:
    defines["XBMC_NV12"] = "";
    texWidth = sourceWidth;
    // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
    if (!g_Windowing.IsFormatSupport(DXGI_FORMAT_R8G8_UNORM, D3D11_FORMAT_SUPPORT_TEXTURE2D))
      defines["NV12_SNORM_UV"] = "";
    break;
  case RENDER_FMT_UYVY422:
    defines["XBMC_UYVY"] = "";
    texWidth = sourceWidth >> 1;
    break;
  case RENDER_FMT_YUYV422:
    defines["XBMC_YUY2"] = "";
    texWidth = sourceWidth >> 1;
    break;
  default:
    return false;
    break;
  }

  m_texSteps[0] = 1.0f/(float)texWidth;
  m_texSteps[1] = 1.0f/(float)sourceHeight;

  std::string effectString = "special://xbmc/system/shaders/yuv2rgb_d3d.fx";

  if(!LoadEffect(effectString, &defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
    return false;
  }

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

void CYUV2RGBShader::Render(CRect sourceRect, CPoint dest[],
                            float contrast,
                            float brightness,
                            unsigned int flags,
                            YUVBuffer* YUVbuf)
{
  PrepareParameters(sourceRect, dest,
                    contrast, brightness, flags);
  SetShaderParameters(YUVbuf);
  Execute(nullptr, 4);
}

CYUV2RGBShader::~CYUV2RGBShader()
{
}

void CYUV2RGBShader::PrepareParameters(CRect sourceRect,
                                       CPoint dest[],
                                       float contrast,
                                       float brightness,
                                       unsigned int flags)
{
  if (m_sourceRect != sourceRect 
    || m_dest[0] != dest[0] || m_dest[1] != dest[1] 
    || m_dest[2] != dest[2] || m_dest[3] != dest[3])
  {
    m_sourceRect = sourceRect;
    for (size_t i = 0; i < 4; ++i)
      m_dest[i] = dest[i];

    CUSTOMVERTEX* v;
    CWinShader::LockVertexBuffer((void**)&v);

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

  m_matrix.SetParameters(contrast * 0.02f,
                         brightness * 0.01f - 0.5f,
                         flags,
                         m_format);
}

void CYUV2RGBShader::SetShaderParameters(YUVBuffer* YUVbuf)
{
  m_effect.SetTechnique("YUV2RGB_T");
  SVideoPlane *planes = YUVbuf->planes;
  ID3D11ShaderResourceView* ppSRView[3] =
  {
    planes[0].texture.GetShaderResource(),
    planes[1].texture.GetShaderResource(),
    planes[2].texture.GetShaderResource(),
  };
  m_effect.SetResources("g_Texture", ppSRView, YUVbuf->GetActivePlanes());
  m_effect.SetMatrix("g_ColorMatrix", m_matrix.Matrix());
  m_effect.SetFloatArray("g_StepXY", m_texSteps, ARRAY_SIZE(m_texSteps));

  UINT numPorts = 1;
  D3D11_VIEWPORT viewPort;
  g_Windowing.Get3D11Context()->RSGetViewports(&numPorts, &viewPort);
  m_effect.SetFloatArray("g_viewPort", &viewPort.Width, 2);
}

//==================================================================================

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
bool CConvolutionShader1Pass::Create(ESCALINGMETHOD method)
{
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
                                CRect sourceRect,
                                CRect destRect,
                                bool useLimitRange)
{
  PrepareParameters(sourceWidth, sourceHeight, sourceRect, destRect);
  float texSteps[] = { 1.0f/(float)sourceWidth, 1.0f/(float)sourceHeight};
  SetShaderParameters(sourceTexture, &texSteps[0], ARRAY_SIZE(texSteps), useLimitRange);
  Execute(nullptr, 4);
}

void CConvolutionShader1Pass::PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                                           CRect sourceRect,
                                           CRect destRect)
{
  if(m_sourceWidth != sourceWidth || m_sourceHeight != sourceHeight
  || m_sourceRect != sourceRect || m_destRect != destRect)
  {
    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceRect = sourceRect;
    m_destRect = destRect;

    CUSTOMVERTEX* v;
    CWinShader::LockVertexBuffer((void**)&v);

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
    (useLimitRange ? 16.f : 0.f) / 255.f,
    (useLimitRange ? (235.f - 16.f) : 255.f) / 255.f,
  };
  m_effect.SetFloatArray("g_colorRange", colorRange, _countof(colorRange));
}

//==================================================================================

CConvolutionShaderSeparable::CConvolutionShaderSeparable() : CConvolutionShader()
{
  m_sourceWidth = -1;
  m_sourceHeight = -1;
  m_destWidth = -1;
  m_destHeight = -1;
}

bool CConvolutionShaderSeparable::Create(ESCALINGMETHOD method)
{
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
                                CRect sourceRect,
                                CRect destRect,
                                bool useLimitRange)
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

  Execute(nullptr, 4);

  // we changed view port, so we need to restore our real viewport.
  g_Windowing.RestoreViewPort();
}

CConvolutionShaderSeparable::~CConvolutionShaderSeparable()
{
  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
  SAFE_RELEASE(m_oldRenderTarget);
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
  ID3D11RenderTargetView* intermediateRT = m_IntermediateTarget.GetRenderTarget();
  g_Windowing.Get3D11Context()->ClearRenderTargetView(intermediateRT, color);
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
    (useLimitRange ? 16.f : 0.f) / 255.f,
    (useLimitRange ? (235.f - 16.f) : 255.f) / 255.f,
  };
  m_effect.SetFloatArray("g_colorRange", colorRange, _countof(colorRange));
}

void CConvolutionShaderSeparable::SetStepParams(UINT iPass)
{
  CD3D11_VIEWPORT viewPort(.0f, .0f, .0f, .0f);
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();

  if (iPass == 0)
  {
    // store old RT
    pContext->OMGetRenderTargets(1, &m_oldRenderTarget, nullptr);
    // setting new RT
    ID3D11RenderTargetView* newRT = m_IntermediateTarget.GetRenderTarget();
    pContext->OMSetRenderTargets(1, &newRT, nullptr);
    // new viewport
    viewPort = CD3D11_VIEWPORT(0.0f, 0.0f, 
                               static_cast<float>(m_IntermediateTarget.GetWidth()), 
                               static_cast<float>(m_IntermediateTarget.GetHeight()));
    // reset scissor
    g_Windowing.ResetScissors();
  }
  else if (iPass == 1)
  {
    if (m_oldRenderTarget)
    {
      // get dimention of old render target
      ID3D11Resource* rtResource = nullptr;
      m_oldRenderTarget->GetResource(&rtResource);
      ID3D11Texture2D* rtTexture = nullptr;
      if (SUCCEEDED(rtResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtTexture))))
      {
        D3D11_TEXTURE2D_DESC rtDescr = {};
        rtTexture->GetDesc(&rtDescr);
        viewPort = CD3D11_VIEWPORT(0.0f, 0.0f,
                                   static_cast<float>(rtDescr.Width),
                                   static_cast<float>(rtDescr.Height));
      }
      SAFE_RELEASE(rtTexture);
      SAFE_RELEASE(rtResource);
    }
    else
    {
      // current RT is null so try to restore viewport
      CRect winViewPort;
      g_Windowing.GetViewPort(winViewPort);
      viewPort = CD3D11_VIEWPORT(winViewPort.x1, winViewPort.y1, winViewPort.Width(), winViewPort.Height());
    }
    pContext->OMSetRenderTargets(1, &m_oldRenderTarget, nullptr);
    SAFE_RELEASE(m_oldRenderTarget);
    // at the second pass m_IntermediateTarget is a source of data
    m_effect.SetTexture("g_Texture", m_IntermediateTarget);
    // restore scissor
    g_Windowing.SetScissors(g_graphicsContext.StereoCorrection(g_graphicsContext.GetScissors()));
  }
  // seting view port
  pContext->RSSetViewports(1, &viewPort);
  // pass viewport dimention to the shaders
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

#endif
