/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

#include "system_gl.h"

#include "BaseRenderer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoSettings.h"
#include "FrameBufferObject.h"
#include "guilib/Shader.h"
#include "RenderFlags.h"
#include "RenderInfo.h"
#include "windowing/GraphicContext.h"

extern "C" {
#include <libavutil/mastering_display_metadata.h>
}

class CRenderCapture;
class CRenderSystemGLES;

class CTexture;
namespace Shaders
{
namespace GLES
{
class BaseYUV2RGBGLSLShader;
class BaseVideoFilterShader;
}
} // namespace Shaders

enum RenderMethod
{
  RENDER_GLSL = 0x01,
  RENDER_CUSTOM = 0x02,
};

enum RenderQuality
{
  RQ_LOW = 1,
  RQ_SINGLEPASS,
  RQ_MULTIPASS,
  RQ_SOFTWARE
};

class CEvent;

class CLinuxRendererGLES : public CBaseRenderer
{
public:
  CLinuxRendererGLES();
  ~CLinuxRendererGLES() override;

  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool IsConfigured() override { return m_bConfigured; }
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void UnInit() override;
  bool Flush(bool saveBuffers) override;
  void SetBufferSize(int numBuffers) override { m_NumYV12Buffers = numBuffers; }
  bool IsGuiLayer() override;
  void ReleaseBuffer(int idx) override;
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  void Update() override;
  bool RenderCapture(int index, CRenderCapture* capture) override;
  CRenderInfo GetRenderInfo() override;
  bool ConfigChanged(const VideoPicture& picture) override;

  // Feature support
  bool SupportsMultiPassRendering() override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

  CRenderCapture* GetRenderCapture() override;

protected:
  static const int FIELD_FULL{0};
  static const int FIELD_TOP{1};
  static const int FIELD_BOT{2};

  virtual bool Render(unsigned int flags, int index);
  virtual void RenderUpdateVideo(bool clear, unsigned int flags = 0, unsigned int alpha = 255);

  int NextYV12Texture();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
  virtual void ReleaseShaders();
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();
  AVColorPrimaries GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned int width, unsigned int height);

  // textures
  virtual bool UploadTexture(int index);
  virtual void DeleteTexture(int index);
  virtual bool CreateTexture(int index);

  bool UploadYV12Texture(int index);
  void DeleteYV12Texture(int index);
  bool CreateYV12Texture(int index);
  virtual bool SkipUploadYV12(int index) { return false; }

  bool UploadNV12Texture(int index);
  void DeleteNV12Texture(int index);
  bool CreateNV12Texture(int index);

  void CalculateTextureSourceRects(int source, int num_planes);

  // renderers
  void RenderToFBO(int index, int field);
  void RenderFromFBO();
  void RenderSinglePass(int index, int field); // single pass glsl renderer

  // hooks for HwDec Renderered
  virtual bool LoadShadersHook() { return false; }
  virtual bool RenderHook(int idx) { return false; }
  virtual void AfterRenderHook(int idx) {}

  struct
  {
    CFrameBufferObject fbo;
    float width{0.0};
    float height{0.0};
  } m_fbo;

  int m_iYV12RenderBuffer{0};
  int m_NumYV12Buffers{0};

  bool m_bConfigured{false};
  bool m_bValidated{false};
  GLenum m_textureTarget = GL_TEXTURE_2D;
  int m_renderMethod{RENDER_GLSL};
  RenderQuality m_renderQuality{RQ_SINGLEPASS};

  // Raw data used by renderer
  int m_currentField{FIELD_FULL};
  int m_reloadShaders{0};
  CRenderSystemGLES *m_renderSystem{nullptr};
  GLenum m_pixelStoreKey{0};

  struct CYuvPlane
  {
    GLuint id{0};
    CRect rect{0, 0, 0, 0};

    float width{0.0};
    float height{0.0};

    unsigned texwidth{0};
    unsigned texheight{0};

    //pixels per texel
    unsigned pixpertex_x{0};
    unsigned pixpertex_y{0};
  };

  struct CPictureBuffer
  {
    CYuvPlane fields[MAX_FIELDS][YuvImage::MAX_PLANES];
    YuvImage image;

    CVideoBuffer *videoBuffer{nullptr};
    bool loaded{false};

    AVColorPrimaries m_srcPrimaries;
    AVColorSpace m_srcColSpace;
    int m_srcBits{8};
    int m_srcTextureBits{8};
    bool m_srcFullRange;

    bool hasDisplayMetadata{false};
    AVMasteringDisplayMetadata displayMetadata;
    bool hasLightMetadata{false};
    AVContentLightMetadata lightMetadata;
  };

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  CPictureBuffer m_buffers[NUM_BUFFERS];

  void LoadPlane(CYuvPlane& plane, int type,
                 unsigned width,  unsigned height,
                 int stride, int bpp, void* data);

  Shaders::GLES::BaseYUV2RGBGLSLShader* m_pYUVProgShader{nullptr};
  Shaders::GLES::BaseYUV2RGBGLSLShader* m_pYUVBobShader{nullptr};
  Shaders::GLES::BaseVideoFilterShader* m_pVideoFilterShader{nullptr};
  ESCALINGMETHOD m_scalingMethod{VS_SCALINGMETHOD_LINEAR};
  ESCALINGMETHOD m_scalingMethodGui{VS_SCALINGMETHOD_MAX};
  bool m_fullRange;
  AVColorPrimaries m_srcPrimaries;
  bool m_toneMap = false;
  ETONEMAPMETHOD m_toneMapMethod = VS_TONEMAPMETHOD_OFF;
  bool m_passthroughHDR = false;
  unsigned char* m_planeBuffer = nullptr;
  size_t m_planeBufferSize = 0;

  // clear colour for "black" bars
  float m_clearColour{0.0f};
  CRect m_viewRect;

private:
  void ClearBackBuffer();
  void ClearBackBufferQuad();
  void DrawBlackBars();
};
