#pragma once

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

#include "../../guilib/Geometry.h"
#include "../WinRenderer.h"

/*
class CBaseVideoFilter
{
};
*/

class CYUV2RGBMatrix
{
public:
  CYUV2RGBMatrix();
  void SetParameters(float contrast, float blacklevel, unsigned int flags);
  D3DXMATRIX* Matrix();

private:
  bool         m_NeedRecalc;
  float        m_contrast;
  float        m_blacklevel;
  unsigned int m_flags;
  D3DXMATRIX   m_mat;
};

class CWinShader
{
protected:
  CWinShader() {}

public:
  void Release(); // for user code only, like the SAFE_RELEASE() construct

protected:
  virtual bool CreateVertexBuffer(DWORD FVF, unsigned int vertCount, unsigned int vertSize, unsigned int primitivesCount);
  virtual bool LockVertexBuffer(void **data);
  virtual bool UnlockVertexBuffer();
  virtual void ReleaseInternal();
  virtual bool LoadEffect(CStdString filename, DefinesMap* defines);
  virtual bool Execute();

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
  virtual bool Create(bool singlepass, unsigned int sourceWidth, unsigned int sourceHeight, BufferFormat fmt);
  virtual void Render(CRect sourceRect,
                      CRect destRect,
                      float contrast,
                      float brightness,
                      unsigned int flags,
                      YUVBuffer* YUVbuf);

protected:
  virtual void PrepareParameters(CRect sourceRect,
                                 CRect destRect,
                                 float contrast,
                                 float brightness,
                                 unsigned int flags);
  virtual void SetShaderParameters(YUVBuffer* YUVbuf);
  virtual void ReleaseInternal();
  virtual bool UploadToGPU(YUVBuffer* YUVbuf);

private:
  CYUV2RGBMatrix m_matrix;
  unsigned int   m_sourceWidth, m_sourceHeight;
  CRect          m_sourceRect, m_destRect;
  CD3DTexture    m_YUVPlanes[3];

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
  virtual bool Create(ESCALINGMETHOD method);
  virtual void Render(CD3DTexture &sourceTexture,
                               unsigned int sourceWidth, unsigned int sourceHeight,
                               CRect sourceRect,
                               CRect destRect);

protected:
  virtual bool KernelTexFormat();
  virtual bool CreateHQKernel(ESCALINGMETHOD method);
  virtual void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                               CRect sourceRect,
                               CRect destRect);
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount);
  virtual void ReleaseInternal();

private:
  CD3DTexture   m_HQKernelTexture;
  D3DFORMAT     m_format;
  bool          m_floattex;
  bool          m_rgba;
  unsigned int  m_sourceWidth, m_sourceHeight;
  CRect         m_sourceRect, m_destRect;

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      FLOAT tu, tv;
  };
};

class CTestShader : public CWinShader
{
public:
  virtual bool Create();
};

#endif
