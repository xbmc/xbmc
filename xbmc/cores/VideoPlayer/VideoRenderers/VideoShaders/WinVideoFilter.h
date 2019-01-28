/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayer.h"
#include "cores/VideoPlayer/VideoRenderers/VideoShaders/ConversionMatrix.h"
#include "guilib/D3DResource.h"
#include "utils/Geometry.h"

#include <vector>
#include <wrl/client.h>

extern "C" {
#include "libavutil/pixfmt.h"
#include "libavutil/mastering_display_metadata.h"
}

enum EBufferFormat;
class CRenderBuffer;

class CWinShader
{
protected:
  CWinShader() = default;
  virtual ~CWinShader() = default;
  bool CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize);
  bool LockVertexBuffer(void **data);
  bool UnlockVertexBuffer();
  bool LoadEffect(const std::string& filename, DefinesMap* defines);
  bool Execute(const std::vector<CD3DTexture*> &targets, unsigned int vertexIndexStep);
  bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC *layout, unsigned numElements);
  virtual void SetStepParams(UINT stepIndex) { }

  CD3DEffect m_effect;
  CD3DTexture* m_target;

private:
  void SetTarget(CD3DTexture* target);
  CD3DBuffer m_vb;
  CD3DBuffer m_ib;
  unsigned int m_vbsize = 0;
  unsigned int m_vertsize = 0;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout = nullptr;
};

class COutputShader : public CWinShader
{
public:
  ~COutputShader() = default;

  void ApplyEffectParameters(CD3DEffect &effect, unsigned sourceWidth, unsigned sourceHeight);
  void GetDefines(DefinesMap &map) const;
  bool Create(bool useCLUT, bool useDithering, int ditherDepth, bool toneMapping);
  void Render(CD3DTexture& sourceTexture, unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4]
            , CD3DTexture& target, unsigned range = 0, float contrast = 0.5f, float brightness = 0.5f);
  void Render(CD3DTexture& sourceTexture, unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, CRect destRect
            , CD3DTexture& target, unsigned range = 0, float contrast = 0.5f, float brightness = 0.5f);
  void SetCLUT(int clutSize, ID3D11ShaderResourceView *pCLUTView);
  void SetDisplayMetadata(bool hasDisplayMetadata, AVMasteringDisplayMetadata displayMetadata,
                          bool hasLightMetadata, AVContentLightMetadata lightMetadata);
  void SetToneMapParam(float param) { m_toneMappingParam = param; }

  static bool CreateCLUTView(int clutSize, uint16_t* clutData, bool isRGB, ID3D11ShaderResourceView** ppCLUTView);

private:
  struct CUSTOMVERTEX {
    FLOAT x, y, z;
    FLOAT tu, tv;
  };

  bool HasCLUT() const { return m_clutSize && m_pCLUTView; }
  void PrepareParameters(unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4]);
  void SetShaderParameters(CD3DTexture &sourceTexture, unsigned range, float contrast, float brightness);
  void CreateDitherView();

  // CLUT fields
  bool m_useCLUT = false;
  unsigned m_sourceWidth = 0;
  unsigned m_sourceHeight = 0;
  CRect m_sourceRect = { 0.f, 0.f, 0.f, 0.f };
  CPoint m_destPoints[4] =
  {
    { 0.f, 0.f },
    { 0.f, 0.f },
    { 0.f, 0.f },
    { 0.f, 0.f },
  };
  int m_clutSize = 0;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pCLUTView = nullptr;

  // dithering fields
  bool m_useDithering = false;
  int m_ditherDepth = 0;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDitherView = nullptr;

  // tone mapping fileds
  bool m_toneMapping = false;
  float m_toneMappingParam = 1.0f;
  bool m_hasDisplayMetadata = false;
  bool m_hasLightMetadata = false;
  AVMasteringDisplayMetadata m_displayMetadata = {};
  AVContentLightMetadata m_lightMetadata = {};
};

class CYUV2RGBShader : public CWinShader
{
public:
  explicit CYUV2RGBShader() = default;
  ~CYUV2RGBShader() = default;

  bool Create(EBufferFormat fmt, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries, std::shared_ptr<COutputShader> pOutShader = nullptr);
  void Render(CRect sourceRect, CPoint dest[], CRenderBuffer* videoBuffer, CD3DTexture& target);
  void SetParams(float contrast, float black, bool limited) const;
  void SetColParams(AVColorSpace colSpace, int bits, bool limited, int textuteBits) const;

protected:
  void PrepareParameters(CRenderBuffer* videoBuffer, CRect sourceRect, CPoint dest[]);
  void SetShaderParameters(CRenderBuffer* videoBuffer);

private:
  unsigned int m_sourceWidth = 0;
  unsigned int m_sourceHeight = 0;
  CRect m_sourceRect = {};
  CPoint m_dest[4];
  EBufferFormat m_format = static_cast<EBufferFormat>(0);
  float m_texSteps[2] = {0.f, 0.f};
  std::shared_ptr<COutputShader> m_pOutShader;
  std::shared_ptr<CConvertMatrix> m_pConvMatrix;

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT tu, tv;   // Y Texture coordinates
      FLOAT tu2, tv2; // U and V Textures coordinates
  };
};

class CConvolutionShader : public CWinShader
{
public:
  CConvolutionShader() = default;
  ~CConvolutionShader() = default;

  virtual bool Create(ESCALINGMETHOD method, std::shared_ptr<COutputShader> pOutShader = nullptr) = 0;
  virtual void Render(CD3DTexture &sourceTexture,
                      unsigned int sourceWidth, unsigned int sourceHeight,
                      unsigned int destWidth, unsigned int destHeight,
                      CRect sourceRect, CRect destRect, bool useLimitRange,
                      CD3DTexture& target) = 0;

protected:
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount, bool useLimitRange) = 0;
  bool ChooseKernelD3DFormat();
  bool CreateHQKernel(ESCALINGMETHOD method);

  CD3DTexture m_HQKernelTexture;
  DXGI_FORMAT m_KernelFormat = DXGI_FORMAT_UNKNOWN;
  bool m_floattex = false;
  bool m_rgba = false;
  std::shared_ptr<COutputShader> m_pOutShader;

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT tu, tv;
  };
};

class CConvolutionShader1Pass : public CConvolutionShader
{
public:
  explicit CConvolutionShader1Pass() = default;

  bool Create(ESCALINGMETHOD method, std::shared_ptr<COutputShader> pOutShader = nullptr) override;
  void Render(CD3DTexture& sourceTexture,
              unsigned int sourceWidth, unsigned int sourceHeight,
              unsigned int destWidth, unsigned int destHeight,
              CRect sourceRect, CRect destRect, bool useLimitRange,
              CD3DTexture& target) override;

protected:
  void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                         CRect sourceRect, CRect destRect);
  void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps,
                           int texStepsCount, bool useLimitRange) override;


private:
  unsigned int m_sourceWidth = 0;
  unsigned int m_sourceHeight = 0;
  CRect m_sourceRect = {};
  CRect m_destRect = {};
};

class CConvolutionShaderSeparable : public CConvolutionShader
{
public:
  explicit CConvolutionShaderSeparable() = default;
  ~CConvolutionShaderSeparable() = default;

  bool Create(ESCALINGMETHOD method, std::shared_ptr<COutputShader> pOutShader = nullptr) override;
  void Render(CD3DTexture& sourceTexture,
              unsigned int sourceWidth, unsigned int sourceHeight,
              unsigned int destWidth, unsigned int destHeight,
              CRect sourceRect, CRect destRect, bool useLimitRange,
              CD3DTexture& target) override;

protected:
  bool ChooseIntermediateD3DFormat();
  bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height);
  bool ClearIntermediateRenderTarget();
  void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                         unsigned int destWidth, unsigned int destHeight,
                         CRect sourceRect, CRect destRect);
  void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps,
                           int texStepsCount, bool useLimitRange) override;
  void SetStepParams(UINT iPass) override;

private:
  CD3DTexture m_IntermediateTarget;
  DXGI_FORMAT m_IntermediateFormat = DXGI_FORMAT_UNKNOWN;
  unsigned int m_sourceWidth = 0;
  unsigned int m_sourceHeight = 0;
  unsigned int m_destWidth = 0;
  unsigned int m_destHeight = 0;
  CRect m_sourceRect = {};
  CRect m_destRect = {};
};

class CTestShader : public CWinShader
{
public:
  bool Create();
};
