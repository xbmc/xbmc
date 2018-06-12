/*
 *      Copyright (C) 2007-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <vector>

#include "system_gl.h"

#include "FrameBufferObject.h"
#include "guilib/Shader.h"
#include "cores/VideoSettings.h"
#include "RenderFlags.h"
#include "RenderInfo.h"
#include "windowing/GraphicContext.h"
#include "BaseRenderer.h"
#include "ColorManager.h"
#include "threads/Event.h"
#include "VideoShaders/ShaderFormats.h"

extern "C" {
#include "libavutil/mastering_display_metadata.h"
}

class CRenderCapture;
class CRenderSystemGL;

class CBaseTexture;
namespace Shaders { class BaseYUV2RGBGLSLShader; }
namespace Shaders { class BaseVideoFilterShader; }

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

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

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

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
  void AddVideoPicture(const VideoPicture &picture, int index, double currentClock) override;
  void UnInit() override;
  void Flush() override;
  void SetBufferSize(int numBuffers) override { m_NumYV12Buffers = numBuffers; }
  void ReleaseBuffer(int idx) override;
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  void Update() override;
  bool RenderCapture(CRenderCapture* capture) override;
  CRenderInfo GetRenderInfo() override;
  bool ConfigChanged(const VideoPicture &picture) override;

  // Feature support
  bool SupportsMultiPassRendering() override;
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

protected:
  bool Render(unsigned int flags, int renderBuffer);
  void ClearBackBuffer();
  void DrawBlackBars();

  bool ValidateRenderer();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
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

  // hooks for HwDec Renderered
  virtual bool LoadShadersHook() { return false; };
  virtual bool RenderHook(int idx) { return false; };
  virtual void AfterRenderHook(int idx) {};

  struct
  {
    CFrameBufferObject fbo;
    float width, height;
  } m_fbo;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;

  bool m_bConfigured;
  bool m_bValidated;
  GLenum m_textureTarget;
  int m_renderMethod;
  RenderQuality m_renderQuality;
  CRenderSystemGL *m_renderSystem;

  // Raw data used by renderer
  int m_currentField;
  int m_reloadShaders;

  struct YUVPLANE
  {
    GLuint id;
    GLuint pbo;

    CRect  rect;

    float  width;
    float  height;

    unsigned texwidth;
    unsigned texheight;

    //pixels per texel
    unsigned pixpertex_x;
    unsigned pixpertex_y;
  };

  struct CPictureBuffer
  {
    CPictureBuffer();
   ~CPictureBuffer();

    YUVPLANE fields[MAX_FIELDS][YuvImage::MAX_PLANES];
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

  void LoadPlane(YUVPLANE& plane, int type,
                 unsigned width,  unsigned height,
                 int stride, int bpp, void* data);

  void GetPlaneTextureSize(YUVPLANE& plane);

  Shaders::BaseYUV2RGBGLSLShader *m_pYUVShader;
  Shaders::BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;
  bool m_useDithering;
  unsigned int m_ditherDepth;
  bool m_fullRange;
  AVColorPrimaries m_srcPrimaries;
  bool m_toneMap = false;

  // clear colour for "black" bars
  float m_clearColour;

  void BindPbo(CPictureBuffer& buff);
  void UnBindPbo(CPictureBuffer& buff);
  bool m_pboSupported;
  bool m_pboUsed;

  bool  m_nonLinStretch;
  bool  m_nonLinStretchGui;
  float m_pixelRatio;

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


