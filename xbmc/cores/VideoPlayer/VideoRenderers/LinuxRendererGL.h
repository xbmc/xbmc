/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

#include "system_gl.h"

#include "FrameBufferObject.h"
#include "cores/VideoSettings.h"
#include "RenderInfo.h"
#include "BaseRenderer.h"
#include "ColorManager.h"
#include "utils/Geometry.h"

extern "C" {
#include <libavutil/mastering_display_metadata.h>
}

class CRenderCapture;
class CRenderSystemGL;

class CTexture;
namespace Shaders
{
namespace GL
{
class BaseYUV2RGBGLSLShader;
class BaseVideoFilterShader;
}
} // namespace Shaders

enum RenderMethod
{
  RENDER_GLSL=0x01,
  RENDER_CUSTOM=0x02
};

enum RenderQuality
{
  RQ_LOW=1,
  RQ_SINGLEPASS,
  RQ_MULTIPASS,
};

#define FIELD_FULL 0
#define FIELD_TOP 1
#define FIELD_BOT 2

class CLinuxRendererGL : public CBaseRenderer
{
public:
  CLinuxRendererGL();
  ~CLinuxRendererGL() override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;
  bool IsConfigured() override { return m_bConfigured; }
  void AddVideoPicture(const VideoPicture &picture, int index) override;
  void UnInit() override;
  bool Flush(bool saveBuffers) override;
  void SetBufferSize(int numBuffers) override { m_NumYV12Buffers = numBuffers; }
  void ReleaseBuffer(int idx) override;
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  void Update() override;
  bool RenderCapture(int index, CRenderCapture* capture) override;
  CRenderInfo GetRenderInfo() override;
  bool ConfigChanged(const VideoPicture &picture) override;

  // Feature support
  bool SupportsMultiPassRendering() override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

  CRenderCapture* GetRenderCapture() override;

protected:

  bool Render(unsigned int flags, int renderBuffer);
  void ClearBackBuffer();
  void ClearBackBufferQuad();
  void DrawBlackBars();

  bool ValidateRenderer();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();
  void CheckVideoParameters(int index);
  AVColorPrimaries GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned int width, unsigned int height);

  // textures
  virtual bool UploadTexture(int index);
  virtual void DeleteTexture(int index);
  virtual bool CreateTexture(int index);

  bool UploadYV12Texture(int index);
  void DeleteYV12Texture(int index);
  bool CreateYV12Texture(int index);

  bool UploadNV12Texture(int index);
  void DeleteNV12Texture(int index);
  bool CreateNV12Texture(int index);

  bool UploadYUV422PackedTexture(int index);
  void DeleteYUV422PackedTexture(int index);
  bool CreateYUV422PackedTexture(int index);

  void CalculateTextureSourceRects(int source, int num_planes);

  // renderers
  void RenderToFBO(int renderBuffer, int field, bool weave = false);
  void RenderFromFBO();
  void RenderSinglePass(int renderBuffer, int field); // single pass glsl renderer
  void RenderRGB(int renderBuffer, int field);      // render using vdpau/vaapi hardware
  void RenderProgressiveWeave(int renderBuffer, int field); // render using vdpau hardware

  struct CYuvPlane;
  struct CPictureBuffer;

  void BindPbo(CPictureBuffer& buff);
  void UnBindPbo(CPictureBuffer& buff);
  void LoadPlane(CYuvPlane& plane, int type,
                 unsigned width,  unsigned height,
                 int stride, int bpp, void* data);
  void GetPlaneTextureSize(CYuvPlane& plane);
  GLint GetInternalFormat(GLint format, int bpp);

  // hooks for HwDec Renderer
  virtual bool LoadShadersHook() { return false; }
  virtual bool RenderHook(int idx) { return false; }
  virtual void AfterRenderHook(int idx) {}
  virtual bool CanSaveBuffers() { return true; }

  struct
  {
    CFrameBufferObject fbo;
    float width, height;
  } m_fbo;

  int m_iYV12RenderBuffer = 0;
  int m_NumYV12Buffers = 0;

  bool m_bConfigured = false;
  bool m_bValidated = false;
  GLenum m_textureTarget = GL_TEXTURE_2D;
  int m_renderMethod = RENDER_GLSL;
  RenderQuality m_renderQuality = RQ_SINGLEPASS;
  CRenderSystemGL *m_renderSystem = nullptr;

  // Raw data used by renderer
  int m_currentField = FIELD_FULL;
  int m_reloadShaders = 0;

  struct CYuvPlane
  {
    GLuint id;
    GLuint pbo;
    CRect rect;
    float width;
    float height;
    unsigned texwidth;
    unsigned texheight;
    //pixels per texel
    unsigned pixpertex_x;
    unsigned pixpertex_y;
  };

  struct CPictureBuffer
  {
    CPictureBuffer();
    ~CPictureBuffer() = default;

    CYuvPlane fields[MAX_FIELDS][YuvImage::MAX_PLANES];
    YuvImage image;
    GLuint pbo[3]; // one pbo for 3 planes

    CVideoBuffer *videoBuffer;
    bool loaded;

    AVColorPrimaries m_srcPrimaries;
    AVColorSpace m_srcColSpace;
    int m_srcBits = 8;
    int m_srcTextureBits = 8;
    bool m_srcFullRange;

    bool hasDisplayMetadata = false;
    AVMasteringDisplayMetadata displayMetadata;
    bool hasLightMetadata = false;
    AVContentLightMetadata lightMetadata;
  };

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  CPictureBuffer m_buffers[NUM_BUFFERS];

  Shaders::GL::BaseYUV2RGBGLSLShader* m_pYUVShader = nullptr;
  Shaders::GL::BaseVideoFilterShader* m_pVideoFilterShader = nullptr;
  ESCALINGMETHOD m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  ESCALINGMETHOD m_scalingMethodGui = VS_SCALINGMETHOD_MAX;
  bool m_useDithering;
  unsigned int m_ditherDepth;
  bool m_fullRange;
  AVColorPrimaries m_srcPrimaries;
  bool m_toneMap = false;
  ETONEMAPMETHOD m_toneMapMethod = VS_TONEMAPMETHOD_OFF;
  float m_clearColour = 0.0f;
  bool m_pboSupported = true;
  bool m_pboUsed = false;
  bool m_nonLinStretch = false;
  bool m_nonLinStretchGui = false;
  float m_pixelRatio = 0.0f;
  CRect m_viewRect;

  // color management
  std::unique_ptr<CColorManager> m_ColorManager;
  GLuint m_tCLUTTex;
  uint16_t *m_CLUT;
  int m_CLUTsize;
  int m_cmsToken;
  bool m_cmsOn;

  bool LoadCLUT();
  void DeleteCLUT();
};
