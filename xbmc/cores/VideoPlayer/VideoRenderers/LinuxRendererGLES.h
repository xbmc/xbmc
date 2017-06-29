#ifndef LINUXRENDERERGLES_RENDERER
#define LINUXRENDERERGLES_RENDERER

/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if HAS_GLES == 2
#include <vector>

#include "system_gl.h"

#include "FrameBufferObject.h"
#include "xbmc/guilib/Shader.h"
#include "settings/VideoSettings.h"
#include "RenderFlags.h"
#include "RenderInfo.h"
#include "guilib/GraphicContext.h"
#include "BaseRenderer.h"
#include "xbmc/cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"

class CRenderCapture;

class CBaseTexture;
namespace Shaders { class BaseYUV2RGBShader; }
namespace Shaders { class BaseVideoFilterShader; }

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

struct YUVCOEF
{
  float r_up, r_vp;
  float g_up, g_vp;
  float b_up, b_vp;
};

enum RenderMethod
{
  RENDER_GLSL   = 0x001,
  RENDER_SW     = 0x004,
  RENDER_POT    = 0x010,
  RENDER_OMXEGL = 0x040,
  RENDER_CVREF  = 0x080,
  RENDER_BYPASS = 0x100,
  RENDER_MEDIACODEC = 0x400,
  RENDER_MEDIACODECSURFACE = 0x800,
  RENDER_IMXMAP = 0x1000
};

enum RenderQuality
{
  RQ_LOW=1,
  RQ_SINGLEPASS,
  RQ_MULTIPASS,
  RQ_SOFTWARE
};

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

#define FIELD_FULL 0
#define FIELD_TOP 1
#define FIELD_BOT 2

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;
extern YUVCOEF yuv_coef_bt601;
extern YUVCOEF yuv_coef_bt709;
extern YUVCOEF yuv_coef_ebu;
extern YUVCOEF yuv_coef_smtp240m;

class CEvent;

class CLinuxRendererGLES : public CBaseRenderer
{
public:
  CLinuxRendererGLES();
  virtual ~CLinuxRendererGLES();

  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  virtual bool Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation) override;
  virtual bool IsConfigured() override { return m_bConfigured; }
  virtual void AddVideoPicture(const VideoPicture &picture, int index) override;
  virtual void FlipPage(int source) override;
  virtual void UnInit() override;
  virtual void Reset() override;
  virtual void Flush() override;
  virtual void ReorderDrawPoints() override;
  virtual void SetBufferSize(int numBuffers) override { m_NumYV12Buffers = numBuffers; }
  virtual bool IsGuiLayer() override;
  virtual void ReleaseBuffer(int idx) override;
  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255) override;
  virtual void Update() override;
  virtual bool RenderCapture(CRenderCapture* capture) override;
  virtual CRenderInfo GetRenderInfo() override;
  virtual bool ConfigChanged(const VideoPicture &picture) override;

  // Feature support
  virtual bool SupportsMultiPassRendering() override;
  virtual bool Supports(ERENDERFEATURE feature) override;
  virtual bool Supports(ESCALINGMETHOD method) override;

protected:
  virtual void Render(DWORD flags, int index);
  virtual void RenderUpdateVideo(bool clear, DWORD flags = 0, DWORD alpha = 255);

  int  NextYV12Texture();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
  virtual void ReleaseShaders();
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();

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

  void UploadBYPASSTexture(int index);
  void DeleteBYPASSTexture(int index);
  bool CreateBYPASSTexture(int index);

  void CalculateTextureSourceRects(int source, int num_planes);

  // renderers
  void RenderMultiPass(int index, int field);     // multi pass glsl renderer
  void RenderSinglePass(int index, int field);    // single pass glsl renderer
  
  // hooks for HwDec Renderered
  virtual bool LoadShadersHook() { return false; };
  virtual bool RenderHook(int idx) { return false; };
  virtual void AfterRenderHook(int idx) {};

  CFrameBufferObject m_fbo;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;
  int m_iLastRenderBuffer;

  bool m_bConfigured;
  bool m_bValidated;
  GLenum m_textureTarget;
  int m_renderMethod;
  int m_oldRenderMethod;
  RenderQuality m_renderQuality;
  bool m_StrictBinding;

  // Raw data used by renderer
  int m_currentField;
  int m_reloadShaders;

  struct YUVPLANE
  {
    GLuint id;
    CRect  rect;

    float  width;
    float  height;

    unsigned texwidth;
    unsigned texheight;

    //pixels per texel
    unsigned pixpertex_x;
    unsigned pixpertex_y;
  };

  struct YUVBUFFER
  {
    YUVBUFFER();
   ~YUVBUFFER();

    YUVPLANE fields[MAX_FIELDS][YuvImage::MAX_PLANES];
    YuvImage image;
    GLuint pbo[3]; // one pbo for 3 planes

    CVideoBuffer *videoBuffer;
    bool loaded;
  };

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFER m_buffers[NUM_BUFFERS];

  void LoadPlane(YUVPLANE& plane, int type,
                 unsigned width,  unsigned height,
                 int stride, int bpp, void* data);

  Shaders::BaseYUV2RGBShader     *m_pYUVProgShader;
  Shaders::BaseYUV2RGBShader     *m_pYUVBobShader;
  Shaders::BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;

  // clear colour for "black" bars
  float m_clearColour;
};


inline int NP2( unsigned x )
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}
#endif

#endif
