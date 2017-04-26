#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "BaseRenderer.h"
#include "ColorManager.h"
#include "HwDecRender/DXVAHD.h"
#include "guilib/D3DResource.h"
#include "RenderCapture.h"
#include "WinVideoBuffer.h"
#include "settings/Settings.h"

#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */

#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)

class CBaseTexture;
class CYUV2RGBShader;
class CConvolutionShader;
class COutputShader;

class DllAvUtil;
class DllAvCodec;
class DllSwScale;

struct VideoPicture;

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

enum RenderMethod
{
  RENDER_INVALID = 0x00,
  RENDER_PS      = 0x01,
  RENDER_SW      = 0x02,
  RENDER_DXVA    = 0x03,
};

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2
#define PLANE_UV 1
#define PLANE_DXVA 0

#define FIELD_FULL 0
#define FIELD_TOP 1
#define FIELD_BOT 2

class CWinRenderer : public CBaseRenderer
{
public:
  CWinRenderer();
  ~CWinRenderer();

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  void Update() override;
  bool RenderCapture(CRenderCapture* capture) override;

  // Player functions
  bool Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation) override;
  void AddVideoPicture(const VideoPicture &picture, int index, double currentClock) override;
  void FlipPage(int source) override;
  void UnInit() override;
  void Reset() override; /* resets renderer after seek for example */
  bool IsConfigured() override { return m_bConfigured; }
  void Flush() override;
  CRenderInfo GetRenderInfo() override;
  void RenderUpdate(bool clear, unsigned int flags = 0, unsigned int alpha = 255) override;
  void SetBufferSize(int numBuffers) override { m_neededBuffers = numBuffers; }
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;

  // Feature support
  bool SupportsMultiPassRendering() override { return false; }
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

  bool WantsDoublePass() override;
  bool ConfigChanged(const VideoPicture& picture) override;

  static bool HandlesVideoBuffer(CVideoBuffer *buffer);

protected:
  void PreInit();
  void Render(DWORD flags);
  void RenderSW();
  void RenderHW(DWORD flags);
  void RenderPS();
  void RenderHQ();
  void ManageTextures();
  void DeleteYV12Texture(int index);
  bool CreateYV12Texture(int index);
  int NextYV12Texture() const;
  void SelectRenderMethod();
  void UpdateVideoFilter();
  void SelectSWVideoFilter();
  void SelectPSVideoFilter();
  void UpdatePSVideoFilter();
  void ColorManagmentUpdate();
  bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height, bool dynamic);
  EBufferFormat SelectBufferFormat(const ERenderFormat format, const RenderMethod method) const;

  bool LoadCLUT();

  bool m_bConfigured;
  bool m_bUseHQScaler;
  bool m_bFilterInitialized;
  bool m_cmsOn{ false };
  bool m_clutLoaded{ true };
  bool m_useDithering;

  unsigned int m_destWidth;
  unsigned int m_destHeight;
  unsigned int m_frameIdx;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;
  int m_neededBuffers;
  int m_iRequestedMethod;
  int m_cmsToken{ -1 };
  int m_CLUTSize{ 0 };
  int m_ditherDepth;

  DXGI_FORMAT m_dxva_format;
  RenderMethod m_renderMethod;
  EBufferFormat m_bufferFormat;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;

  std::vector<ERenderFormat> m_formats;

  SWinVideoBuffer *m_VideoBuffers[NUM_BUFFERS];
  DXVA::CProcessorHD *m_processor;
  struct SwsContext *m_sw_scale_ctx;
  CYUV2RGBShader* m_colorShader;
  CConvolutionShader* m_scalerShader;
  std::unique_ptr<COutputShader> m_outputShader;
  CRenderCapture* m_capture = nullptr;
  std::unique_ptr<CColorManager> m_colorManager;
  ID3D11ShaderResourceView *m_pCLUTView{ nullptr };

  CD3DTexture m_IntermediateTarget;
};
