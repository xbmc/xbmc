#ifndef LINUXRENDERERGL_RENDERER
#define LINUXRENDERERGL_RENDERER

#ifdef HAS_SDL_OPENGL

#include "../../../guilib/Surface.h"
#include "../../../guilib/FrameBufferObject.h"
#include "../../../guilib/Shader.h"
#include "../ffmpeg/DllSwScale.h"
#include "../ffmpeg/DllAvCodec.h"
#include "VideoShaders/YUV2RGBShader.h"
#include "VideoShaders/VideoFilterShader.h"
#include "../../settings/VideoSettings.h"

using namespace Surface;
using namespace Shaders;

#define NUM_BUFFERS 3

#define MAX_PLANES 3
#define MAX_FIELDS 3

#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

typedef struct YV12Image
{
  BYTE *   plane[MAX_PLANES];
  unsigned stride[MAX_PLANES];
  unsigned width;
  unsigned height;
  unsigned flags;
  float texcoord_x;
  float texcoord_y;

  unsigned cshift_x; /* this is the chroma shift used */
  unsigned cshift_y;
} YV12Image;

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */
#define IMAGE_FLAG_READY     0x16 /* image is ready to be uploaded to texture memory */
#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)


#define RENDER_FLAG_EVEN        0x01
#define RENDER_FLAG_ODD         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_EVEN | RENDER_FLAG_ODD)
#define RENDER_FLAG_FIELDMASK   0x03
#define RENDER_FLAG_LAST        0x04

#define RENDER_FLAG_NOOSD       0x04 /* don't draw any osd */
#define RENDER_FLAG_NOOSDALPHA  0x08 /* don't allow alpha when osd is drawn */

/* these two flags will be used if we need to render same image twice (bob deinterlacing) */
#define RENDER_FLAG_NOLOCK      0x10   /* don't attempt to lock texture before rendering */
#define RENDER_FLAG_NOUNLOCK    0x20   /* don't unlock texture after rendering */

/* this defines what color translation coefficients */
#define CONF_FLAGS_YUVCOEF_MASK(a) ((a) & 0x07)
#define CONF_FLAGS_YUVCOEF_BT709 0x01
#define CONF_FLAGS_YUVCOEF_BT601 0x02
#define CONF_FLAGS_YUVCOEF_240M  0x03
#define CONF_FLAGS_YUVCOEF_EBU   0x04

#define CONF_FLAGS_YUV_FULLRANGE 0x08
#define CONF_FLAGS_FULLSCREEN    0x10

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
  FS_ODD,
  FS_EVEN,
  FS_BOTH
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
  RENDER_POT=0x16
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
#define FIELD_ODD 1
#define FIELD_EVEN 2

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;
extern YUVCOEF yuv_coef_bt601;
extern YUVCOEF yuv_coef_bt709;
extern YUVCOEF yuv_coef_ebu;
extern YUVCOEF yuv_coef_smtp240m;

class CLinuxRendererGL
{
public:
  CLinuxRendererGL();  
  virtual ~CLinuxRendererGL();

  virtual void GetVideoRect(RECT &rs, RECT &rd);
  virtual float GetAspectRatio();
  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};
  virtual void SetViewMode(int iViewMode);

  void CreateThumbnail(SDL_Surface * surface, unsigned int width, unsigned int height);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void         DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */

  void AutoCrop(bool bCrop);
  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  RESOLUTION GetResolution();  

protected:
  virtual void Render(DWORD flags);
  virtual void CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fUserPixelRatio, float fZoomAmount);
  void CalculateFrameAspectRatio(int desired_width, int desired_height);
  void ChooseBestResolution(float fps);
  virtual void ManageDisplay();
  void CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride);
  virtual void ManageTextures();
  void DeleteOSDTextures(int index);
  void Setup_Y8A8Render();
  void RenderOSD();
  void DeleteYV12Texture(int index);
  void ClearYV12Texture(int index);
  virtual bool CreateYV12Texture(int index, bool clear=true);
  void CopyYV12Texture(int dest);
  int  NextYV12Texture();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int renderMethod=FIELD_FULL);
  void LoadTextures(int source);
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();

  // renderers
  //void RenderLowMem(DWORD flags);     // low mem renderer
  void RenderMultiPass(DWORD flags);  // multi pass glsl renderer
  void RenderSinglePass(DWORD flags); // single pass glsl renderer
  void RenderSoftware(DWORD flags);   // single pass s/w yuv2rgb renderer

  CFrameBufferObject m_fbo;
  CSurface *m_pBuffer;

  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;

  float m_fSourceFrameRatio; // the frame aspect ratio of the source (corrected for pixel ratio)
  RESOLUTION m_iResolution;    // the resolution we're running in
  float m_fps;        // fps of movie
  RECT rd;          // destination rect
  RECT rs;          // source rect
  unsigned int m_iSourceWidth;    // width
  unsigned int m_iSourceHeight;   // height

  bool m_bConfigured;
  GLenum m_textureTarget;
  unsigned short m_renderMethod;
  RenderQuality m_renderQuality;

  // OSD stuff
  GLuint m_pOSDYTexture[NUM_BUFFERS];
  GLuint m_pOSDATexture[NUM_BUFFERS];
  GLubyte* m_pOSDYBuffer;
  GLubyte* m_pOSDABuffer;

  float m_OSDWidth;
  float m_OSDHeight;
  DRAWRECT m_OSDRect;
  int m_iOSDRenderBuffer;
  int m_iOSDTextureWidth;
  int m_iOSDTextureHeight[NUM_BUFFERS];
  int m_NumOSDBuffers;
  bool m_OSDRendered;

  // Raw data used by renderer
  YV12Image m_image[NUM_BUFFERS];
  int m_currentField;
  int m_reloadShaders;

  typedef GLuint YUVPLANES[MAX_PLANES];
  typedef YUVPLANES          YUVFIELDS[MAX_FIELDS];
  typedef YUVFIELDS          YUVBUFFERS[NUM_BUFFERS];

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFERS m_YUVTexture;

  //BaseYUV2RGBGLSLShader     *m_pYUVShaderGLSL;
  //BaseYUV2RGBARBShader      *m_pYUVShaderARB;
  CShaderProgram        *m_pYUVShader;
  BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;

//  /*
  GLuint m_fragmentShader;
  GLuint m_vertexShader;
//  */
  GLint m_yTex;
  GLint m_uTex;
  GLint m_vTex;
//  /*
  GLint m_brightness;
  GLint m_contrast;
  GLint m_stepX;
  GLint m_stepY;
  GLint m_shaderField;
//  */

  // clear colour for "black" bars
  DWORD m_clearColour;

  // software scale libraries (fallback if required gl version is not available)
  DllAvUtil   m_dllAvUtil;
  DllAvCodec  m_dllAvCodec;
  DllSwScale  m_dllSwScale;
  BYTE	     *m_rgbBuffer;  // if software scale is used, this will hold the result image
  int	      m_rgbBufferSize;

  static void TextureCallback(DWORD dwContext);

  HANDLE m_eventTexturesDone[NUM_BUFFERS];
  HANDLE m_eventOSDDone[NUM_BUFFERS];

};


inline int NP2( unsigned x ) {
#if defined(_LINUX) 
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
