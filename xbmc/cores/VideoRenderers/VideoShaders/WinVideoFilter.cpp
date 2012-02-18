/*
 *      Copyright (C) 2007-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DX

#include "WinVideoFilter.h"
#include "windowing/WindowingFactory.h"
#include "../../../utils/log.h"
#include "../../../FileSystem/File.h"
#include <map>
#include "ConvolutionKernels.h"
#include "YUV2RGBShader.h"
#include "win32/WIN32Util.h"

CYUV2RGBMatrix::CYUV2RGBMatrix()
{
  m_NeedRecalc = true;
}

void CYUV2RGBMatrix::SetParameters(float contrast, float blacklevel, unsigned int flags)
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
}

D3DXMATRIX* CYUV2RGBMatrix::Matrix()
{
  if (m_NeedRecalc)
  {
    TransformMatrix matrix;
    CalculateYUVMatrix(matrix, m_flags, m_blacklevel, m_contrast);

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
    m_mat._44 = 0.0f;
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
}

bool CWinShader::CreateVertexBuffer(DWORD FVF, unsigned int vertCount, unsigned int vertSize, unsigned int primitivesCount)
{
  if (!m_vb.Create(vertCount * vertSize, D3DUSAGE_WRITEONLY, FVF, g_Windowing.DefaultD3DPool()))
    return false;
  m_vbsize = vertCount * vertSize;
  m_FVF = FVF;
  m_vertsize = vertSize;
  m_primitivesCount = primitivesCount;
  return true;
}

bool CWinShader::LockVertexBuffer(void **data)
{
  if (!m_vb.Lock(0, m_vbsize, data, 0))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock vertex buffer");
    return false;
  }
  return true;
}

bool CWinShader::UnlockVertexBuffer()
{
  if (!m_vb.Unlock())
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock vertex buffer");
    return false;
  }
  return true;
}

bool CWinShader::LoadEffect(CStdString filename, DefinesMap* defines)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - loading shader %s", filename.c_str());

  XFILE::CFileStream file;
  if(!file.Open(filename))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to open file %s", filename.c_str());
    return false;
  }

  CStdString pStrEffect;
  getline(file, pStrEffect, '\0');

  if (!m_effect.Create(pStrEffect, defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__" %s failed", pStrEffect.c_str());
    return false;
  }

  return true;
}

bool CWinShader::Execute(std::vector<LPDIRECT3DSURFACE9> *vecRT, unsigned int vertexIndexStep)
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  LPDIRECT3DSURFACE9 oldRT = 0;
  // The render target will be overriden: save the caller's original RT
  if (vecRT != NULL && vecRT->size() > 0)
    pD3DDevice->GetRenderTarget(0, &oldRT);

  pD3DDevice->SetFVF(m_FVF);
  pD3DDevice->SetStreamSource(0, m_vb.Get(), 0, m_vertsize);

  UINT cPasses, iPass;
  if (!m_effect.Begin( &cPasses, 0 ))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - failed to begin d3d effect");
    return false;
  }

  for( iPass = 0; iPass < cPasses; iPass++ )
  {
    if (!m_effect.BeginPass( iPass ))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - failed to begin d3d effect pass");
      break;
    }

    if (vecRT != NULL && vecRT->size() > iPass)
      pD3DDevice->SetRenderTarget(0, (*vecRT)[iPass]);

    HRESULT hr = pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, iPass * vertexIndexStep, m_primitivesCount);
    if (FAILED(hr))
      CLog::Log(LOGERROR, __FUNCTION__" - failed DrawPrimitive %08X", hr);

    if (!m_effect.EndPass())
      CLog::Log(LOGERROR, __FUNCTION__" - failed to end d3d effect pass");
  }
  if (!m_effect.End())
    CLog::Log(LOGERROR, __FUNCTION__" - failed to end d3d effect");

  if (oldRT != 0)
  {
    pD3DDevice->SetRenderTarget(0, oldRT);
    oldRT->Release();
  }

  return true;
}

//==================================================================================

bool CYUV2RGBShader::Create(unsigned int sourceWidth, unsigned int sourceHeight, BufferFormat fmt)
{
  CWinShader::CreateVertexBuffer(D3DFVF_XYZRHW | D3DFVF_TEX3, 4, sizeof(CUSTOMVERTEX), 2);

  m_sourceWidth = sourceWidth;
  m_sourceHeight = sourceHeight;

  unsigned int texWidth;

  DefinesMap defines;

  if (fmt == YV12)
  {
    defines["XBMC_YV12"] = "";
    texWidth = sourceWidth;

    if(!m_YUVPlanes[0].Create(texWidth    , m_sourceHeight    , 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT)
    || !m_YUVPlanes[1].Create(texWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT)
    || !m_YUVPlanes[2].Create(texWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create YV12 planes.");
      return false;
    }
  }
  else if (fmt == NV12)
  {
    defines["XBMC_NV12"] = "";
    texWidth = sourceWidth;

    if(!m_YUVPlanes[0].Create(texWidth    , m_sourceHeight    , 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT)
    || !m_YUVPlanes[1].Create(texWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_A8L8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create NV12 planes.");
      return false;
    }
  }
  else if (fmt == YUY2)
  {
    defines["XBMC_YUY2"] = "";
    texWidth = sourceWidth >> 1;

    if(!m_YUVPlanes[0].Create(texWidth    , m_sourceHeight    , 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create YUY2 planes.");
      return false;
    }
  }
  else if (fmt == UYVY)
  {
    defines["XBMC_UYVY"] = "";
    texWidth = sourceWidth >> 1;

    if(!m_YUVPlanes[0].Create(texWidth    , m_sourceHeight    , 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create UYVY planes.");
      return false;
    }
  }
  else
    return false;

  m_texSteps[0] = 1.0f/(float)texWidth;
  m_texSteps[1] = 1.0f/(float)sourceHeight;

  CStdString effectString = "special://xbmc/system/shaders/yuv2rgb_d3d.fx";

  if(!LoadEffect(effectString, &defines))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
    return false;
  }

  return true;
}

void CYUV2RGBShader::Render(CRect sourceRect, CRect destRect,
                            float contrast,
                            float brightness,
                            unsigned int flags,
                            YUVBuffer* YUVbuf)
{
  PrepareParameters(sourceRect, destRect,
                    contrast, brightness, flags);
  UploadToGPU(YUVbuf);
  SetShaderParameters(YUVbuf);
  Execute(NULL,4);
}

CYUV2RGBShader::~CYUV2RGBShader()
{
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if (m_YUVPlanes[i].Get())
      m_YUVPlanes[i].Release();
  }
}

void CYUV2RGBShader::PrepareParameters(CRect sourceRect,
                                       CRect destRect,
                                       float contrast,
                                       float brightness,
                                       unsigned int flags)
{
  //See RGB renderer for comment on this
  #define CHROMAOFFSET_HORIZ 0.25f

  if (m_sourceRect != sourceRect || m_destRect != destRect)
  {
    m_sourceRect = sourceRect;
    m_destRect = destRect;

    CUSTOMVERTEX* v;
    CWinShader::LockVertexBuffer((void**)&v);

    v[0].x = destRect.x1;
    v[0].y = destRect.y1;
    v[0].tu = sourceRect.x1 / m_sourceWidth;
    v[0].tv = sourceRect.y1 / m_sourceHeight;
    v[0].tu2 = v[0].tu3 = (sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1);
    v[0].tv2 = v[0].tv3 = (sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1);

    v[1].x = destRect.x2;
    v[1].y = destRect.y1;
    v[1].tu = sourceRect.x2 / m_sourceWidth;
    v[1].tv = sourceRect.y1 / m_sourceHeight;
    v[1].tu2 = v[1].tu3 = (sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1);
    v[1].tv2 = v[1].tv3 = (sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1);

    v[2].x = destRect.x2;
    v[2].y = destRect.y2;
    v[2].tu = sourceRect.x2 / m_sourceWidth;
    v[2].tv = sourceRect.y2 / m_sourceHeight;
    v[2].tu2 = v[2].tu3 = (sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1);
    v[2].tv2 = v[2].tv3 = (sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1);

    v[3].x = destRect.x1;
    v[3].y = destRect.y2;
    v[3].tu = sourceRect.x1 / m_sourceWidth;
    v[3].tv = sourceRect.y2 / m_sourceHeight;
    v[3].tu2 = v[3].tu3 = (sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1);
    v[3].tv2 = v[3].tv3 = (sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1);

    // -0.5 offset to compensate for D3D rasterization
    // set z and rhw
    for(int i = 0; i < 4; i++)
    {
      v[i].x -= 0.5;
      v[i].y -= 0.5;
      v[i].z = 0.0f;
      v[i].rhw = 1.0f;
    }
    CWinShader::UnlockVertexBuffer();
  }

  m_matrix.SetParameters(contrast * 0.02f,
                         brightness * 0.01f - 0.5f,
                         flags);
}

void CYUV2RGBShader::SetShaderParameters(YUVBuffer* YUVbuf)
{
  m_effect.SetMatrix("g_ColorMatrix", m_matrix.Matrix());
  m_effect.SetTechnique("YUV2RGB_T");
  m_effect.SetTexture("g_YTexture", m_YUVPlanes[0]);
  if (YUVbuf->GetActivePlanes() > 1)
    m_effect.SetTexture("g_UTexture", m_YUVPlanes[1]);
  if (YUVbuf->GetActivePlanes() > 2)
    m_effect.SetTexture("g_VTexture", m_YUVPlanes[2]);
  m_effect.SetFloatArray("g_StepXY", m_texSteps, sizeof(m_texSteps)/sizeof(m_texSteps[0]));
}

bool CYUV2RGBShader::UploadToGPU(YUVBuffer* YUVbuf)
{
  const POINT point = { 0, 0 };

  for (unsigned int i = 0; i<YUVbuf->GetActivePlanes(); i++)
  {
    const RECT rect = { 0, 0, YUVbuf->planes[i].texture.GetWidth(), YUVbuf->planes[i].texture.GetHeight() };
    IDirect3DSurface9 *src, *dest;
    if(FAILED(YUVbuf->planes[i].texture.Get()->GetSurfaceLevel(0, &src)))
      CLog::Log(LOGERROR, __FUNCTION__": Failed to retrieve level 0 surface for source YUV plane %d", i);
    if (FAILED(m_YUVPlanes[i].Get()->GetSurfaceLevel(0, &dest)))
      CLog::Log(LOGERROR, __FUNCTION__": Failed to retrieve level 0 surface for destination YUV plane %d", i);

    if (FAILED(g_Windowing.Get3DDevice()->UpdateSurface(src, &rect, dest, &point)))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to copy plane %d from sysmem to vidmem.", i);
      src->Release();
      dest->Release();
      return false;
    }
    src->Release();
    dest->Release();
  }
  return true;
}

//==================================================================================

CConvolutionShader::~CConvolutionShader()
{
  if(m_HQKernelTexture.Get())
    m_HQKernelTexture.Release();
}

bool CConvolutionShader::ChooseKernelD3DFormat()
{
  if (g_Windowing.IsTextureFormatOk(D3DFMT_A16B16G16R16F, 0))
  {
    m_KernelFormat = D3DFMT_A16B16G16R16F;
    m_floattex = true;
    m_rgba = true;
  }
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_A8B8G8R8, 0))
  {
    m_KernelFormat = D3DFMT_A8B8G8R8;
    m_floattex = false;
    m_rgba = true;
  }
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_A8R8G8B8, 0))
  {
    m_KernelFormat = D3DFMT_A8R8G8B8;
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

  if (!m_HQKernelTexture.Create(kern.GetSize(), 1, 1, g_Windowing.DefaultD3DUsage(), m_KernelFormat, g_Windowing.DefaultD3DPool()))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create kernel texture.");
    return false;
  }

  void *kernelVals;
  int kernelValsSize;

  if (m_floattex)
  {
    float *rawVals = kern.GetFloatPixels();
    D3DXFLOAT16* float16Vals = new D3DXFLOAT16[kern.GetSize()*4];

    for(int i = 0; i < kern.GetSize()*4; i++)
      float16Vals[i] = rawVals[i];
    kernelVals = float16Vals;
    kernelValsSize = sizeof(D3DXFLOAT16)*kern.GetSize()*4;
  }
  else
  {
    kernelVals = kern.GetUint8Pixels();
    kernelValsSize = sizeof(uint8_t)*kern.GetSize()*4;
  }

  D3DLOCKED_RECT lr;
  if (!m_HQKernelTexture.LockRect(0, &lr, NULL, D3DLOCK_DISCARD))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to lock kernel texture.");
  memcpy(lr.pBits, kernelVals, kernelValsSize);
  if (!m_HQKernelTexture.UnlockRect(0))
    CLog::Log(LOGERROR, __FUNCTION__": Failed to unlock kernel texture.");

  if (m_floattex)
    delete[] kernelVals;

  return true;
}
//==================================================================================
bool CConvolutionShader1Pass::Create(ESCALINGMETHOD method)
{
  CStdString effectString;
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

  CWinShader::CreateVertexBuffer(D3DFVF_XYZRHW | D3DFVF_TEX1, 4, sizeof(CUSTOMVERTEX), 2);

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

  return true;
}

void CConvolutionShader1Pass::Render(CD3DTexture &sourceTexture,
                                unsigned int sourceWidth, unsigned int sourceHeight,
                                unsigned int destWidth, unsigned int destHeight,
                                CRect sourceRect,
                                CRect destRect)
{
  PrepareParameters(sourceWidth, sourceHeight, sourceRect, destRect);
  float texSteps[] = { 1.0f/(float)sourceWidth, 1.0f/(float)sourceHeight};
  SetShaderParameters(sourceTexture, &texSteps[0], sizeof(texSteps)/sizeof(texSteps[0]));
  Execute(NULL,4);
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
    v[0].tu = sourceRect.x1 / sourceWidth;
    v[0].tv = sourceRect.y1 / sourceHeight;

    v[1].x = destRect.x2;
    v[1].y = destRect.y1;
    v[1].tu = sourceRect.x2 / sourceWidth;
    v[1].tv = sourceRect.y1 / sourceHeight;

    v[2].x = destRect.x2;
    v[2].y = destRect.y2;
    v[2].tu = sourceRect.x2 / sourceWidth;
    v[2].tv = sourceRect.y2 / sourceHeight;

    v[3].x = destRect.x1;
    v[3].y = destRect.y2;
    v[3].tu = sourceRect.x1 / sourceWidth;
    v[3].tv = sourceRect.y2 / sourceHeight;

    // -0.5 offset to compensate for D3D rasterization
    // set z and rhw
    for(int i = 0; i < 4; i++)
    {
      v[i].x -= 0.5;
      v[i].y -= 0.5;
      v[i].z = 0.0f;
      v[i].rhw = 1.0f;
    }

    CWinShader::UnlockVertexBuffer();
  }
}

void CConvolutionShader1Pass::SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount)
{
  m_effect.SetTechnique( "SCALER_T" );
  m_effect.SetTexture( "g_Texture",  sourceTexture ) ;
  m_effect.SetTexture( "g_KernelTexture", m_HQKernelTexture );
  m_effect.SetFloatArray("g_StepXY", texSteps, texStepsCount);
}

//==================================================================================

CConvolutionShaderSeparable::CConvolutionShaderSeparable()
{
  m_sourceWidth = -1;
  m_sourceHeight = -1;
  m_destWidth = -1;
  m_destHeight = -1;
}

bool CConvolutionShaderSeparable::Create(ESCALINGMETHOD method)
{
  CStdString effectString;
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

  CWinShader::CreateVertexBuffer(D3DFVF_XYZRHW | D3DFVF_TEX1, 8, sizeof(CUSTOMVERTEX), 2);

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

  return true;
}

void CConvolutionShaderSeparable::Render(CD3DTexture &sourceTexture,
                                unsigned int sourceWidth, unsigned int sourceHeight,
                                unsigned int destWidth, unsigned int destHeight,
                                CRect sourceRect,
                                CRect destRect)
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if(m_destWidth != destWidth || m_sourceHeight != sourceHeight)
    CreateIntermediateRenderTarget(destWidth, sourceHeight);

  PrepareParameters(sourceWidth, sourceHeight, destWidth, destHeight, sourceRect, destRect);
  float texSteps1[] = { 1.0f/(float)sourceWidth, 1.0f/(float)sourceHeight};
  float texSteps2[] = { 1.0f/(float)destWidth, 1.0f/(float)(sourceHeight)};
  SetShaderParameters(sourceTexture, &texSteps1[0], sizeof(texSteps1)/sizeof(texSteps1[0]), &texSteps2[0], sizeof(texSteps2)/sizeof(texSteps2[0]));

  // This part should be cleaned up, but how?
  std::vector<LPDIRECT3DSURFACE9> rts;
  LPDIRECT3DSURFACE9 intRT, currentRT;
  m_IntermediateTarget.GetSurfaceLevel(0, &intRT);
  pD3DDevice->GetRenderTarget(0, &currentRT);
  rts.push_back(intRT);
  rts.push_back(currentRT);
  Execute(&rts, 4);
  intRT->Release();
  currentRT->Release();
}

CConvolutionShaderSeparable::~CConvolutionShaderSeparable()
{
  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
}

bool CConvolutionShaderSeparable::ChooseIntermediateD3DFormat()
{
  DWORD usage = D3DUSAGE_RENDERTARGET;

  // Need a float texture, as the output of the first pass can contain negative values.
  if      (g_Windowing.IsTextureFormatOk(D3DFMT_A16B16G16R16F, usage)) m_IntermediateFormat = D3DFMT_A16B16G16R16F;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_A32B32G32R32F, usage)) m_IntermediateFormat = D3DFMT_A32B32G32R32F;
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

  if(!m_IntermediateTarget.Create(width, height, 1, D3DUSAGE_RENDERTARGET, m_IntermediateFormat, D3DPOOL_DEFAULT))
  {
    CLog::Log(LOGERROR, __FUNCTION__": render target creation failed.");
    return false;
  }
  return true;
}

bool CConvolutionShaderSeparable::ClearIntermediateRenderTarget()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  LPDIRECT3DSURFACE9 currentRT;
  pD3DDevice->GetRenderTarget(0, &currentRT);

  LPDIRECT3DSURFACE9 intermediateRT;
  m_IntermediateTarget.GetSurfaceLevel(0, &intermediateRT);

  pD3DDevice->SetRenderTarget(0, intermediateRT);

  pD3DDevice->Clear(0L, NULL, D3DCLEAR_TARGET, 0L, 1.0f, 0L);

  pD3DDevice->SetRenderTarget(0, currentRT);
  currentRT->Release();
  intermediateRT->Release();

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
    // fixme better: clearing the whole render target when changing the source/dest rect is not optimal.
    // Problem is that the edges of the final picture may retain content when the rects change.
    // For example when changing zoom value, the edges can retain content from the previous zoom value.
    // Playing with coordinates was unsuccessful so far, this is a quick fix for release.
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
    CRect tgtRect(0, 0, destWidth, destHeight);
    CWIN32Util::CropSource(sourceRect, destRect, tgtRect);

    // Manipulate the coordinates to work only on the active parts of the textures,
    // and therefore avoid the need to clear surfaces/render targets

    // Pass 1:
    // Horizontal dimension: crop/zoom, so that it is completely done with the convolution shader. Scaling to display width in pass1 and
    // cropping/zooming in pass 2 would use bilinear in pass2, which we don't want.
    // Vertical dimension: crop using sourceRect to save memory bandwidth for high zoom values, but don't stretch/shrink in any way in this pass.
    
    v[0].x = 0;
    v[0].y = 0;
    v[0].tu = sourceRect.x1 / sourceWidth;
    v[0].tv = sourceRect.y1 / sourceHeight;

    v[1].x = destRect.x2 - destRect.x1;
    v[1].y = 0;
    v[1].tu = sourceRect.x2 / sourceWidth;
    v[1].tv = sourceRect.y1 / sourceHeight;

    v[2].x = destRect.x2 - destRect.x1;
    v[2].y = sourceRect.y2 - sourceRect.y1;
    v[2].tu = sourceRect.x2 / sourceWidth;
    v[2].tv = sourceRect.y2 / sourceHeight;

    v[3].x = 0;
    v[3].y = sourceRect.y2 - sourceRect.y1;
    v[3].tu = sourceRect.x1 / sourceWidth;
    v[3].tv = sourceRect.y2 / sourceHeight;

    // Pass 2: pass the horizontal data untouched, resize vertical dimension for final result.

    v[4].x = destRect.x1;
    v[4].y = destRect.y1;
    v[4].tu = 0;
    v[4].tv = 0;

    v[5].x = destRect.x2;
    v[5].y = destRect.y1;
    v[5].tu = (destRect.x2 - destRect.x1) / destWidth;
    v[5].tv = 0;

    v[6].x = destRect.x2;
    v[6].y = destRect.y2;
    v[6].tu = (destRect.x2 - destRect.x1) / destWidth;
    v[6].tv = (sourceRect.y2 - sourceRect.y1) / sourceHeight;

    v[7].x = destRect.x1;
    v[7].y = destRect.y2;
    v[7].tu = 0;
    v[7].tv = (sourceRect.y2 - sourceRect.y1) / sourceHeight;

    // -0.5 offset to compensate for D3D rasterization
    // set z and rhw
    for(int i = 0; i < 8; i++)
    {
      v[i].x -= 0.5;
      v[i].y -= 0.5;
      v[i].z = 0.0f;
      v[i].rhw = 1.0f;
    }

    CWinShader::UnlockVertexBuffer();
  }
}

void CConvolutionShaderSeparable::SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps1, int texStepsCount1, float* texSteps2, int texStepsCount2)
{
  m_effect.SetTechnique( "SCALER_T" );
  m_effect.SetTexture( "g_Texture",  sourceTexture ) ;
  m_effect.SetTexture( "g_KernelTexture", m_HQKernelTexture );
  m_effect.SetTexture( "g_IntermediateTexture",  m_IntermediateTarget ) ;
  m_effect.SetFloatArray("g_StepXY_P0", texSteps1, texStepsCount1);
  m_effect.SetFloatArray("g_StepXY_P1", texSteps2, texStepsCount2);
}


//==========================================================

bool CTestShader::Create()
{
  CStdString effectString = "special://xbmc/system/shaders/testshader.fx";

  if(!LoadEffect(effectString, NULL))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
    return false;
  }
  return true;
}

#endif
