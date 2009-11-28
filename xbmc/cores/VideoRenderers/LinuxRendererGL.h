#ifndef LINUXRENDERERGL_RENDERER
#define LINUXRENDERERGL_RENDERER

#ifdef HAS_GL

#include "../../../guilib/FrameBufferObject.h"
#include "../../../guilib/Shader.h"
#include "../ffmpeg/DllSwScale.h"
#include "../ffmpeg/DllAvCodec.h"
#include "../../settings/VideoSettings.h"
#include "RenderFlags.h"
#include "GraphicContext.h"
#include "BaseRenderer.h"

class CBaseTexture;
namespace Shaders { class BaseYUV2RGBShader; }
namespace Shaders { class BaseVideoFilterShader; }

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
  FS_ODD,
  FS_EVEN
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
  RENDER_POT=0x10
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

class CLinuxRendererGL : public CBaseRenderer
{
public:
  CLinuxRendererGL();  
  virtual ~CLinuxRendererGL();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};

  void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual bool IsConfigured() { return m_bConfigured; }
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  // Feature support
  virtual bool SupportsBrightness();
  virtual bool SupportsContrast();
  virtual bool SupportsGamma();
  virtual bool SupportsMultiPassRendering();
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);

protected:
  virtual void Render(DWORD flags, int renderBuffer);

  void ChooseUpscalingMethod();
  bool IsSoftwareUpscaling();
  void InitializeSoftwareUpscaling();

  virtual void ManageTextures();
  void DeleteYV12Texture(int index);
  void ClearYV12Texture(int index);
  virtual bool CreateYV12Texture(int index, bool clear=true);
  void CopyYV12Texture(int dest);
  int  NextYV12Texture();
  virtual bool ValidateRenderTarget();
  virtual void LoadShaders(int field=FIELD_FULL);
  void LoadTextures(int source);
  void SetTextureFilter(GLenum method);
  void UpdateVideoFilter();

  // renderers
  void RenderMultiPass(int renderBuffer, int field);  // multi pass glsl renderer
  void RenderSinglePass(int renderBuffer, int field); // single pass glsl renderer
  void RenderSoftware(int renderBuffer, int field);   // single pass s/w yuv2rgb renderer
  void RenderVDPAU(int renderBuffer, int field);      // render using vdpau hardware

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
  bool m_StrictBinding;

  // Software upscaling.
  int m_upscalingWidth;
  int m_upscalingHeight;
  YV12Image m_imScaled;
  bool m_isSoftwareUpscaling;

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

    unsigned flipindex;
  };

  typedef YUVPLANE           YUVPLANES[MAX_PLANES];
  typedef YUVPLANES          YUVFIELDS[MAX_FIELDS];

  struct YUVBUFFER
  {
    YUVFIELDS fields;
    YV12Image image;
    unsigned  flipindex; /* used to decide if this has been uploaded */
    GLuint    pbo[MAX_PLANES];
  };

  typedef YUVBUFFER          YUVBUFFERS[NUM_BUFFERS];

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFERS m_buffers;

  void LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                , unsigned width,  unsigned height
                , int stride, void* data );

  Shaders::BaseYUV2RGBShader     *m_pYUVShader;
  Shaders::BaseVideoFilterShader *m_pVideoFilterShader;
  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;

  // clear colour for "black" bars
  float m_clearColour;

  // software scale libraries (fallback if required gl version is not available)
  DllAvUtil    m_dllAvUtil;
  DllAvCodec   m_dllAvCodec;
  DllSwScale   m_dllSwScale;
  BYTE	      *m_rgbBuffer;  // if software scale is used, this will hold the result image
  unsigned int m_rgbBufferSize;

  HANDLE m_eventTexturesDone[NUM_BUFFERS];

  CRect m_crop;

  void BindPbo(YUVBUFFER& buff, int plane);
  void UnBindPbo(YUVBUFFER& buff, int plane);
  bool m_pboused;
};


inline int NP2( unsigned x ) {
#if defined(_LINUX) && !defined(__POWERPC__) && !defined(__PPC__)
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
