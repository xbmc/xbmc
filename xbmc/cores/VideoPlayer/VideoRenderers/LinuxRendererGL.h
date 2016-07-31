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

#pragma once

#include "system.h"

#ifdef HAS_GL
#include <vector>

#include "system_gl.h"

#include "FrameBufferObject.h"
#include "guilib/Shader.h"
#include "settings/VideoSettings.h"
#include "RenderFlags.h"
#include "RenderFormats.h"
#include "guilib/GraphicContext.h"
#include "BaseRenderer.h"
#include "ColorManager.h"

#include "threads/Event.h"

class CRenderCapture;

class CBaseTexture;
namespace Shaders { class BaseYUV2RGBShader; }
namespace Shaders { class BaseVideoFilterShader; }

#undef ALIGN
#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#define NOSOURCE   -2
#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */
#define IMAGE_FLAG_READY     0x16 /* image is ready to be uploaded to texture memory */
#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)

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
  RENDER_GLSL=0x01,
  RENDER_ARB=0x02,
  RENDER_VDPAU=0x08,
  RENDER_POT=0x10,
  RENDER_VAAPI=0x20,
  RENDER_CVREF = 0x40,
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

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;
extern YUVCOEF yuv_coef_bt601;
extern YUVCOEF yuv_coef_bt709;
extern YUVCOEF yuv_coef_ebu;
extern YUVCOEF yuv_coef_smtp240m;

class CLinuxRendererGL : public CBaseRenderer
{
public:
  CLinuxRendererGL();
  virtual ~CLinuxRendererGL();

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_formatl, unsigned int orientation);
  virtual bool IsConfigured() { return m_bConfigured; }
  virtual int GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void ReleaseImage(int source, bool preserve = false);
  virtual void FlipPage(int source);
  virtual void PreInit();
  virtual void UnInit();
  virtual void Reset(); /* resets renderer after seek for example */
  virtual void Flush();
  virtual void SetBufferSize(int numBuffers) { m_NumYV12Buffers = numBuffers; }
  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  virtual void Update();
  virtual bool RenderCapture(CRenderCapture* capture);
  virtual EINTERLACEMETHOD AutoInterlaceMethod();
  virtual CRenderInfo GetRenderInfo();

  // Feature support
  virtual bool SupportsMultiPassRendering();
  virtual bool Supports(ERENDERFEATURE feature);
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);

protected:
  virtual void Render(DWORD flags, int renderBuffer);
  void ClearBackBuffer();
  void DrawBlackBars();

  bool ValidateRenderer();
  int  NextYV12Texture();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();

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
  void RenderSoftware(int renderBuffer, int field);   // single pass s/w yuv2rgb renderer
  void RenderRGB(int renderBuffer, int field);      // render using vdpau/vaapi hardware
  void RenderProgressiveWeave(int renderBuffer, int field); // render using vdpau hardware

  // hooks for HwDec Renderered
  virtual bool LoadShadersHook() { return false; };
  virtual bool RenderHook(int idx) { return false; };

  struct
  {
    CFrameBufferObject fbo;
    float width, height;
  } m_fbo;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;
  int m_iLastRenderBuffer;

  bool m_bConfigured;
  bool m_bValidated;
  std::vector<ERenderFormat> m_formats;
  bool m_bImageReady;
  GLenum m_textureTarget;
  int m_renderMethod;
  RenderQuality m_renderQuality;
  unsigned int m_flipindex; // just a counter to keep track of if a image has been uploaded
  
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

    unsigned flipindex;
  };

  typedef YUVPLANE           YUVPLANES[MAX_PLANES];
  typedef YUVPLANES          YUVFIELDS[MAX_FIELDS];

  struct YUVBUFFER
  {
    YUVBUFFER();
   ~YUVBUFFER();

    YUVFIELDS fields;
    YV12Image image;
    unsigned  flipindex; /* used to decide if this has been uploaded */
    GLuint    pbo[MAX_PLANES];

    void *hwDec;
  };

  typedef YUVBUFFER          YUVBUFFERS[NUM_BUFFERS];

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFERS m_buffers;

  void LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                , unsigned width,  unsigned height
                , int stride, int bpp, void* data, GLuint* pbo = NULL );

  void GetPlaneTextureSize(YUVPLANE& plane);

  Shaders::BaseYUV2RGBShader *m_pYUVShader;
  Shaders::BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;
  bool m_useDithering;
  unsigned int m_ditherDepth;
  bool m_fullRange;

  // clear colour for "black" bars
  float m_clearColour;

  // software scale library (fallback if required gl version is not available)
  BYTE              *m_rgbBuffer;  // if software scale is used, this will hold the result image
  unsigned int       m_rgbBufferSize;
  GLuint             m_rgbPbo;
  struct SwsContext *m_context;

  void BindPbo(YUVBUFFER& buff);
  void UnBindPbo(YUVBUFFER& buff);
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


inline int NP2( unsigned x ) {
#if defined(TARGET_POSIX) && !defined(__POWERPC__) && !defined(__PPC__) && !defined(__arm__) && !defined(__aarch64__) && !defined(__mips__)
  // If there are any issues compiling this, just append a ' && 0'
  // to the above to make it '#if defined(TARGET_POSIX) && 0'

  // Linux assembly is AT&T Unix style, not Intel style
  unsigned y;
  __asm__("dec %%ecx \n"
          "movl $1, %%eax \n"
          "bsr %%ecx,%%ecx \n"
          "inc %%ecx \n"
          "shl %%cl, %%eax \n"
          "movl %%eax, %0 \n"
          :"=r"(y)
          :"c"(x)
          :"%eax");
  return y;
#else
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
#endif
}
#endif
