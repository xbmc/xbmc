#ifndef LINUXRENDERERGL_RENDERER
#define LINUXRENDERERGL_RENDERER

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

#ifdef HAS_GL

#include "guilib/FrameBufferObject.h"
#include "guilib/Shader.h"
#include "settings/VideoSettings.h"
#include "RenderFlags.h"
#include "guilib/GraphicContext.h"
#include "BaseRenderer.h"

#include "threads/Event.h"

class CRenderCapture;

class CVDPAU;
class CBaseTexture;
namespace Shaders { class BaseYUV2RGBShader; }
namespace Shaders { class BaseVideoFilterShader; }
namespace VAAPI   { struct CHolder; }

#define NUM_BUFFERS 3


#undef ALIGN
#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

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

enum EFIELDSYNC
{
  FS_NONE,
  FS_TOP,
  FS_BOT
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
  RENDER_SW=0x04,
  RENDER_VDPAU=0x08,
  RENDER_POT=0x10,
  RENDER_VAAPI=0x20,
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

class DllSwScale;

class CLinuxRendererGL : public CBaseRenderer
{
public:
  CLinuxRendererGL();
  virtual ~CLinuxRendererGL();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};

  bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, unsigned int format);
  virtual bool IsConfigured() { return m_bConfigured; }
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual void         Flush();

#ifdef HAVE_LIBVDPAU
  virtual void         AddProcessor(CVDPAU* vdpau);
#endif
#ifdef HAVE_LIBVA
  virtual void         AddProcessor(VAAPI::CHolder& holder);
#endif

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  // Feature support
  virtual bool SupportsMultiPassRendering();
  virtual bool Supports(ERENDERFEATURE feature);
  virtual bool Supports(EDEINTERLACEMODE mode);
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);

  virtual EINTERLACEMETHOD AutoInterlaceMethod();

protected:
  virtual void Render(DWORD flags, int renderBuffer);
  void         ClearBackBuffer();
  void         DrawBlackBars();

  bool ValidateRenderer();
  virtual void ManageTextures();
  int  NextYV12Texture();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();

  // textures
  void (CLinuxRendererGL::*m_textureUpload)(int index);
  void (CLinuxRendererGL::*m_textureDelete)(int index);
  bool (CLinuxRendererGL::*m_textureCreate)(int index);

  void UploadYV12Texture(int index);
  void DeleteYV12Texture(int index);
  bool CreateYV12Texture(int index);

  void UploadNV12Texture(int index);
  void DeleteNV12Texture(int index);
  bool CreateNV12Texture(int index);
  
  void UploadVDPAUTexture(int index);
  void DeleteVDPAUTexture(int index);
  bool CreateVDPAUTexture(int index);

  void UploadVAAPITexture(int index);
  void DeleteVAAPITexture(int index);
  bool CreateVAAPITexture(int index);

  void UploadYUV422PackedTexture(int index);
  void DeleteYUV422PackedTexture(int index);
  bool CreateYUV422PackedTexture(int index);

  void UploadRGBTexture(int index);
  void ToRGBFrame(YV12Image* im, unsigned flipIndexPlane, unsigned flipIndexBuf);
  void ToRGBFields(YV12Image* im, unsigned flipIndexPlaneTop, unsigned flipIndexPlaneBot, unsigned flipIndexBuf);
  void SetupRGBBuffer();

  void CalculateTextureSourceRects(int source, int num_planes);

  // renderers
  void RenderMultiPass(int renderBuffer, int field);  // multi pass glsl renderer
  void RenderSinglePass(int renderBuffer, int field); // single pass glsl renderer
  void RenderSoftware(int renderBuffer, int field);   // single pass s/w yuv2rgb renderer
  void RenderVDPAU(int renderBuffer, int field);      // render using vdpau hardware
  void RenderVAAPI(int renderBuffer, int field);      // render using vdpau hardware

  CFrameBufferObject m_fbo;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;
  int m_iLastRenderBuffer;

  bool m_bConfigured;
  bool m_bValidated;
  bool m_bImageReady;
  unsigned m_iFlags;
  GLenum m_textureTarget;
  unsigned short m_renderMethod;
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

#ifdef HAVE_LIBVDPAU
    CVDPAU*   vdpau;
#endif
#ifdef HAVE_LIBVA
    VAAPI::CHolder& vaapi;
#endif
  };

  typedef YUVBUFFER          YUVBUFFERS[NUM_BUFFERS];

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFERS m_buffers;

  void LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                , unsigned width,  unsigned height
                , int stride, void* data, GLuint* pbo = NULL );


  Shaders::BaseYUV2RGBShader     *m_pYUVShader;
  Shaders::BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;

  // clear colour for "black" bars
  float m_clearColour;

  // software scale library (fallback if required gl version is not available)
  DllSwScale        *m_dllSwScale;
  BYTE              *m_rgbBuffer;  // if software scale is used, this will hold the result image
  unsigned int       m_rgbBufferSize;
  GLuint             m_rgbPbo;
  struct SwsContext *m_context;

  CEvent* m_eventTexturesDone[NUM_BUFFERS];

  void BindPbo(YUVBUFFER& buff);
  void UnBindPbo(YUVBUFFER& buff);
  bool m_pboSupported;
  bool m_pboUsed;

  bool  m_nonLinStretch;
  bool  m_nonLinStretchGui;
  float m_pixelRatio;
};


inline int NP2( unsigned x ) {
#if defined(_LINUX) && !defined(__POWERPC__) && !defined(__PPC__) && !defined(__arm__)
  // If there are any issues compiling this, just append a ' && 0'
  // to the above to make it '#if defined(_LINUX) && 0'

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

#endif
