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

#include "system_gl.h"

#include "xbmc/guilib/FrameBufferObject.h"
#include "xbmc/guilib/Shader.h"
#include "settings/VideoSettings.h"
#include "RenderFlags.h"
#include "RenderFormats.h"
#include "guilib/GraphicContext.h"
#include "BaseRenderer.h"
#include "xbmc/cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"

class CRenderCapture;

class CBaseTexture;
namespace Shaders { class BaseYUV2RGBShader; }
namespace Shaders { class BaseVideoFilterShader; }
typedef std::vector<int>     Features;


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
  RENDER_GLSL   = 0x001,
  RENDER_SW     = 0x004,
  RENDER_POT    = 0x010,
  RENDER_OMXEGL = 0x040,
  RENDER_CVREF  = 0x080,
  RENDER_BYPASS = 0x100,
  RENDER_EGLIMG = 0x200,
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

  virtual void Update();

  virtual bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_formatunsigned, unsigned int orientation);
  virtual bool IsConfigured() { return m_bConfigured; }
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual void         FlipPage(int source);
  virtual void         PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual void         Flush();
  virtual void         ReorderDrawPoints();
  virtual void         ReleaseBuffer(int idx){ };
  virtual void         SetBufferSize(int numBuffers) { m_NumYV12Buffers = numBuffers; }
  virtual bool         IsGuiLayer();

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  virtual bool RenderUpdateCheckForEmptyField() { return true; }

  // Feature support
  virtual bool SupportsMultiPassRendering();
  virtual bool Supports(ERENDERFEATURE feature);
  virtual bool Supports(EDEINTERLACEMODE mode);
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);

  virtual EINTERLACEMETHOD AutoInterlaceMethod();

  virtual CRenderInfo GetRenderInfo();

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

  void UploadYV12Texture(int index);
  void DeleteYV12Texture(int index);
  bool CreateYV12Texture(int index);
  virtual bool SkipUploadYV12(int index) { return false; }

  void UploadNV12Texture(int index);
  void DeleteNV12Texture(int index);
  bool CreateNV12Texture(int index);

  void UploadBYPASSTexture(int index);
  void DeleteBYPASSTexture(int index);
  bool CreateBYPASSTexture(int index);

  void CalculateTextureSourceRects(int source, int num_planes);

  // renderers
  void RenderMultiPass(int index, int field);     // multi pass glsl renderer
  void RenderSinglePass(int index, int field);    // single pass glsl renderer
  void RenderSoftware(int index, int field);      // single pass s/w yuv2rgb renderer
  
  // hooks for HwDec Renderered
  virtual bool LoadShadersHook() { return false; };
  virtual bool RenderHook(int idx) { return false; };
  virtual bool RenderUpdateVideoHook(bool clear, DWORD flags, DWORD alpha) { return false; };
  virtual int  GetImageHook(YV12Image *image, int source = AUTOSOURCE, bool readonly = false) { return NOSOURCE; }

  CFrameBufferObject m_fbo;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;
  int m_iLastRenderBuffer;

  bool m_bConfigured;
  bool m_bValidated;
  std::vector<ERenderFormat> m_formats;
  bool m_bImageReady;
  GLenum m_textureTarget;
  unsigned short m_renderMethod;
  unsigned short m_oldRenderMethod;
  RenderQuality m_renderQuality;
  unsigned int m_flipindex; // just a counter to keep track of if a image has been uploaded
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
    void *hwDec;
  };

  typedef YUVBUFFER          YUVBUFFERS[NUM_BUFFERS];

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFERS m_buffers;

  void LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                , unsigned width,  unsigned height
                , unsigned int stride, int bpp, void* data );

  Shaders::BaseYUV2RGBShader     *m_pYUVProgShader;
  Shaders::BaseYUV2RGBShader     *m_pYUVBobShader;
  Shaders::BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;

  Features m_renderFeatures;
  Features m_deinterlaceMethods;
  Features m_deinterlaceModes;
  Features m_scalingMethods;

  // clear colour for "black" bars
  float m_clearColour;

  // software scale libraries (fallback if required gl version is not available)
  struct SwsContext *m_sw_context;
  BYTE	      *m_rgbBuffer;  // if software scale is used, this will hold the result image
  unsigned int m_rgbBufferSize;
  float        m_textureMatrix[16];
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
