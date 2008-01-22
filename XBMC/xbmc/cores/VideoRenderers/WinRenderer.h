#pragma once
#ifndef _LINUX

//#define MP_DIRECTRENDERING

#ifdef MP_DIRECTRENDERING
#define NUM_BUFFERS 3
#else
#define NUM_BUFFERS 2
#endif

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

  unsigned cshift_x; /* this is the chroma shift used */
  unsigned cshift_y;
} YV12Image;

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */

#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)


#define RENDER_FLAG_EVEN        0x01
#define RENDER_FLAG_ODD         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_EVEN | RENDER_FLAG_ODD)
#define RENDER_FLAG_FIELDMASK   0x03

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
  FS_BOTH,
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

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;
extern YUVCOEF yuv_coef_bt601;
extern YUVCOEF yuv_coef_bt709;
extern YUVCOEF yuv_coef_ebu;
extern YUVCOEF yuv_coef_smtp240m;

class CWinRenderer
{
public:
  CWinRenderer(LPDIRECT3DDEVICE8 pDevice);
  ~CWinRenderer();

  virtual void GetVideoRect(RECT &rs, RECT &rd);
  virtual float GetAspectRatio();
  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};
  virtual void SetViewMode(int iViewMode);
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);

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
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
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
  bool CreateYV12Texture(int index);
  void CopyYV12Texture(int dest);
  int  NextYV12Texture();

  // low memory renderer (default PixelShaderRenderer)
  void RenderLowMem(DWORD flags);
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

  // OSD stuff
  LPDIRECT3DTEXTURE8 m_pOSDYTexture[NUM_BUFFERS];
  LPDIRECT3DTEXTURE8 m_pOSDATexture[NUM_BUFFERS];
  float m_OSDWidth;
  float m_OSDHeight;
  DRAWRECT m_OSDRect;
  int m_iOSDRenderBuffer;
  int m_iOSDTextureWidth;
  int m_iOSDTextureHeight[NUM_BUFFERS];
  int m_NumOSDBuffers;
  bool m_OSDRendered;

  typedef LPDIRECT3DTEXTURE8 YUVPLANES[MAX_PLANES];
  typedef YUVPLANES          YUVBUFFERS[NUM_BUFFERS];

  #define PLANE_Y 0
  #define PLANE_U 1
  #define PLANE_V 2

  #define FIELD_FULL 0
  #define FIELD_ODD 1
  #define FIELD_EVEN 2

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  YUVBUFFERS m_YUVTexture;

  // render device
  LPDIRECT3DDEVICE8 m_pD3DDevice;

  // pixel shader (low memory shader used in all renderers while in GUI)
  DWORD m_hLowMemShader;

  // clear colour for "black" bars
  DWORD m_clearColour;
};


class CPixelShaderRenderer : public CWinRenderer
{
public:
  CPixelShaderRenderer(LPDIRECT3DDEVICE8 pDevice);
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

protected:
  virtual void Render(DWORD flags);
};

#else
#include "LinuxRenderer.h"
#endif

