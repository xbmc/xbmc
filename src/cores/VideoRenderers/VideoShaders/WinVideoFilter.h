#pragma once

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

#include "../../guilib/Geometry.h"
#include "../WinRenderer.h"
#include "../RenderFormats.h"


class CYUV2RGBMatrix
{
public:
  CYUV2RGBMatrix();
  void SetParameters(float contrast, float blacklevel, unsigned int flags, ERenderFormat format);
  D3DXMATRIX* Matrix();

private:
  bool         m_NeedRecalc;
  float        m_contrast;
  float        m_blacklevel;
  unsigned int m_flags;
  ERenderFormat m_format;
  D3DXMATRIX   m_mat;
};

class CWinShader
{
protected:
  CWinShader() :
    m_vbsize(0),
    m_FVF(0),
    m_vertsize(0),
    m_primitivesCount(0)
  {}
  virtual ~CWinShader();
  virtual bool CreateVertexBuffer(DWORD FVF, unsigned int vertCount, unsigned int vertSize, unsigned int primitivesCount);
  virtual bool LockVertexBuffer(void **data);
  virtual bool UnlockVertexBuffer();
  virtual bool LoadEffect(const std::string& filename, DefinesMap* defines);
  virtual bool Execute(std::vector<LPDIRECT3DSURFACE9> *vecRT, unsigned int vertexIndexStep);

  CD3DEffect   m_effect;

private:
  CD3DVertexBuffer m_vb;
  unsigned int     m_vbsize;
  DWORD            m_FVF;
  unsigned int     m_vertsize;
  unsigned int     m_primitivesCount;
};

class CYUV2RGBShader : public CWinShader
{
public:
  virtual bool Create(unsigned int sourceWidth, unsigned int sourceHeight, ERenderFormat fmt);
  virtual void Render(CRect sourceRect,
                      CRect destRect,
                      float contrast,
                      float brightness,
                      unsigned int flags,
                      YUVBuffer* YUVbuf);
  CYUV2RGBShader() : 
    m_sourceWidth (0),
    m_sourceHeight(0),
    m_format      (RENDER_FMT_NONE)    
    {
      memset(&m_texSteps,0,sizeof(m_texSteps));
    }
  virtual ~CYUV2RGBShader();

protected:
  virtual void PrepareParameters(CRect sourceRect,
                                 CRect destRect,
                                 float contrast,
                                 float brightness,
                                 unsigned int flags);
  virtual void SetShaderParameters(YUVBuffer* YUVbuf);
  virtual bool UploadToGPU(YUVBuffer* YUVbuf);

private:
  CYUV2RGBMatrix m_matrix;
  unsigned int   m_sourceWidth, m_sourceHeight;
  CRect          m_sourceRect, m_destRect;
  ERenderFormat  m_format;
  CD3DTexture    m_YUVPlanes[3];
  float          m_texSteps[2];

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      FLOAT tu, tv;   // Y Texture coordinates
      FLOAT tu2, tv2; // U Texture coordinates
      FLOAT tu3, tv3; // V Texture coordinates
  };
};

class CConvolutionShader : public CWinShader
{
public:
  virtual bool Create(ESCALINGMETHOD method) = 0;
  virtual void Render(CD3DTexture &sourceTexture,
                               unsigned int sourceWidth, unsigned int sourceHeight,
                               unsigned int destWidth, unsigned int destHeight,
                               CRect sourceRect,
                               CRect destRect) = 0;
  virtual ~CConvolutionShader();

protected:
  virtual bool ChooseKernelD3DFormat();
  virtual bool CreateHQKernel(ESCALINGMETHOD method);

  CD3DTexture   m_HQKernelTexture;
  D3DFORMAT     m_KernelFormat;
  bool          m_floattex;
  bool          m_rgba;

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      FLOAT tu, tv;
  };
};

class CConvolutionShader1Pass : public CConvolutionShader
{
public:
  virtual bool Create(ESCALINGMETHOD method);
  virtual void Render(CD3DTexture &sourceTexture,
                               unsigned int sourceWidth, unsigned int sourceHeight,
                               unsigned int destWidth, unsigned int destHeight,
                               CRect sourceRect,
                               CRect destRect);
    CConvolutionShader1Pass() : m_sourceWidth (0), m_sourceHeight(0) {}

protected:
  virtual void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                               CRect sourceRect,
                               CRect destRect);
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount);


private:
  unsigned int  m_sourceWidth, m_sourceHeight;
  CRect         m_sourceRect, m_destRect;
};

class CConvolutionShaderSeparable : public CConvolutionShader
{
public:
  CConvolutionShaderSeparable();
  virtual bool Create(ESCALINGMETHOD method);
  virtual void Render(CD3DTexture &sourceTexture,
                               unsigned int sourceWidth, unsigned int sourceHeight,
                               unsigned int destWidth, unsigned int destHeight,
                               CRect sourceRect,
                               CRect destRect);
  virtual ~CConvolutionShaderSeparable();

protected:
  virtual bool ChooseIntermediateD3DFormat();
  virtual bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height);
  virtual bool ClearIntermediateRenderTarget();
  virtual void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                               unsigned int destWidth, unsigned int destHeight,
                               CRect sourceRect,
                               CRect destRect);
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps1, int texStepsCount1, float* texSteps2, int texStepsCount2);

private:
  CD3DTexture   m_IntermediateTarget;
  D3DFORMAT     m_IntermediateFormat;
  unsigned int  m_sourceWidth, m_sourceHeight;
  unsigned int  m_destWidth, m_destHeight;
  CRect         m_sourceRect, m_destRect;
};

class CTestShader : public CWinShader
{
public:
  virtual bool Create();
};

#endif
