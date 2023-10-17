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

#include <array>
#include <vector>

#include <DirectXMath.h>
#include <wrl/client.h>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/mastering_display_metadata.h>
}
class CRenderBuffer;

using namespace DirectX;

class CWinShader
{
protected:
  CWinShader() = default;
  virtual ~CWinShader() = default;

  virtual bool CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize);
  virtual bool LockVertexBuffer(void **data);
  virtual bool UnlockVertexBuffer();
  virtual bool LoadEffect(const std::string& filename, DefinesMap* defines);
  virtual bool Execute(const std::vector<CD3DTexture*>& targets, unsigned int vertexIndexStep);
  virtual void SetStepParams(unsigned stepIndex) { }
  virtual bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC *layout, unsigned numElements);

  CD3DEffect m_effect;
  CD3DTexture* m_target = nullptr;

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
  explicit COutputShader() = default;
  ~COutputShader() = default;

  void ApplyEffectParameters(CD3DEffect &effect, unsigned sourceWidth, unsigned sourceHeight);
  void GetDefines(DefinesMap &map) const;
  bool Create(bool useLUT,
              bool useDithering,
              int ditherDepth,
              bool toneMapping,
              ETONEMAPMETHOD toneMethod,
              bool HLGtoPQ);
  void Render(CD3DTexture& sourceTexture, CRect sourceRect, const CPoint points[4]
            , CD3DTexture& target, unsigned range = 0, float contrast = 0.5f, float brightness = 0.5f);
  void Render(CD3DTexture& sourceTexture, CRect sourceRect, CRect destRect
            , CD3DTexture& target, unsigned range = 0, float contrast = 0.5f, float brightness = 0.5f);
  void SetLUT(int lutSize, ID3D11ShaderResourceView *pLUTView);
  void SetDisplayMetadata(bool hasDisplayMetadata, AVMasteringDisplayMetadata displayMetadata,
                          bool hasLightMetadata, AVContentLightMetadata lightMetadata);
  void SetToneMapParam(ETONEMAPMETHOD method, float param);
  std::string GetDebugInfo();

  static bool CreateLUTView(int lutSize, uint16_t* lutData, bool isRGB, ID3D11ShaderResourceView** ppLUTView);

private:
  struct Vertex
  {
    float x, y, z;
    float tu, tv;
  };

  bool HasLUT() const { return m_lutSize && m_pLUTView; }
  void PrepareParameters(unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4]);
  void SetShaderParameters(CD3DTexture &sourceTexture, unsigned range, float contrast, float brightness);
  void CreateDitherView();

  bool m_useLut = false;
  bool m_useDithering = false;
  bool m_toneMapping = false;
  bool m_useHLGtoPQ = false;

  bool m_hasDisplayMetadata = false;
  bool m_hasLightMetadata = false;
  unsigned m_sourceWidth = 0;
  unsigned m_sourceHeight = 0;
  int m_lutSize = 0;
  int m_ditherDepth = 0;
  ETONEMAPMETHOD m_toneMappingMethod = VS_TONEMAPMETHOD_OFF;
  float m_toneMappingParam = 1.0f;
  float m_toneMappingDebug = .0f;

  CRect m_sourceRect = {};
  CPoint m_destPoints[4] = {};
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pDitherView = nullptr;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pLUTView = nullptr;

  AVMasteringDisplayMetadata m_displayMetadata = {};
  AVContentLightMetadata m_lightMetadata = {};
};

class CYUV2RGBShader : public CWinShader
{
public:
  explicit CYUV2RGBShader() = default;
  ~CYUV2RGBShader() = default;

  bool Create(AVPixelFormat fmt,
              AVColorPrimaries dstPrimaries,
              AVColorPrimaries srcPrimaries,
              const std::shared_ptr<COutputShader>& pOutShader = nullptr);
  void Render(CRect sourceRect, CPoint dest[], CRenderBuffer* videoBuffer, CD3DTexture& target);
  void SetParams(float contrast, float black, bool limited);
  void SetColParams(AVColorSpace colSpace, int bits, bool limited, int texBits);

protected:
  void PrepareParameters(CRenderBuffer* videoBuffer, CRect sourceRect, CPoint dest[]);
  void SetShaderParameters(CRenderBuffer* videoBuffer);

private:
  struct Vertex {
    float x, y, z;
    float tu, tv;   // Y Texture coordinates
    float tu2, tv2; // U and V Textures coordinates
  };

  unsigned int m_sourceWidth = 0;
  unsigned int m_sourceHeight = 0;
  CRect m_sourceRect = {};
  CPoint m_dest[4] = {};
  AVPixelFormat m_format = AV_PIX_FMT_NONE;
  std::array<float, 2> m_texSteps = {};
  std::shared_ptr<COutputShader> m_pOutShader = nullptr;
  CConvertMatrix m_convMatrix;
  bool m_colorConversion{false};
};

class CConvolutionShader : public CWinShader
{
public:
  virtual ~CConvolutionShader() = default;
  virtual bool Create(ESCALINGMETHOD method, const std::shared_ptr<COutputShader>& pOutShader = nullptr) = 0;
  virtual void Render(CD3DTexture& sourceTexture, CD3DTexture& target,
                      CRect sourceRect, CRect destRect, bool useLimitRange) = 0;
protected:
  struct Vertex {
    float x, y, z;
    float tu, tv;
  };

  CConvolutionShader() = default;

  virtual bool ChooseKernelD3DFormat();
  virtual bool CreateHQKernel(ESCALINGMETHOD method);
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount, bool useLimitRange) = 0;

  bool m_floattex = false;
  bool m_rgba = false;
  DXGI_FORMAT m_KernelFormat = DXGI_FORMAT_UNKNOWN;
  CD3DTexture m_HQKernelTexture;
  std::shared_ptr<COutputShader> m_pOutShader = nullptr;
};

class CConvolutionShader1Pass : public CConvolutionShader
{
public:
  explicit CConvolutionShader1Pass() = default;

  bool Create(ESCALINGMETHOD method, const std::shared_ptr<COutputShader>& pOutputShader = nullptr) override;
  void Render(CD3DTexture& sourceTexture, CD3DTexture& target,
              CRect sourceRect, CRect destRect, bool useLimitRange) override;

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

  bool Create(ESCALINGMETHOD method, const std::shared_ptr<COutputShader>& pOutputShader = nullptr) override;
  void Render(CD3DTexture& sourceTexture, CD3DTexture& target,
              CRect sourceRect, CRect destRect, bool useLimitRange) override;

protected:
  bool ChooseIntermediateD3DFormat();
  bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height);
  bool ClearIntermediateRenderTarget();
  void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
                         unsigned int destWidth, unsigned int destHeight,
                         CRect sourceRect, CRect destRect);
  void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps,
                           int texStepsCount, bool useLimitRange) override;
  void SetStepParams(unsigned iPass) override;

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
  virtual bool Create();
};
